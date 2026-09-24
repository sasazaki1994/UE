#include "MagatsunePlayer.h"
#include "MagatsuneBoss.h"
#include "MagatsuneGameMode.h"
#include "GrabComponent.h"
#include "StaminaComponent.h"
#include "PlayerSenseComponent.h"
#include "NushiProgressComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraTypes.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "PrimitiveAppearance.h"
#include "ShirotsuraVisualComponent.h"

AMagatsunePlayer::AMagatsunePlayer()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;
    GetCapsuleComponent()->InitCapsuleSize(34, 88);
    Grab = CreateDefaultSubobject<UGrabComponent>(TEXT("SharedGrab"));
    Stamina = CreateDefaultSubobject<UStaminaComponent>(TEXT("SharedStamina"));
    Sense = CreateDefaultSubobject<UPlayerSenseComponent>(TEXT("PlayerSense"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    PrimitiveBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Climber"));
    PrimitiveBody->SetupAttachment(RootComponent);
    PrimitiveBody->SetStaticMesh(Sphere.Object);
    PrimitiveBody->SetMaterial(0, Material.Object);
    PrimitiveBody->SetRelativeScale3D({.7, .7, 1.65});
    PrimitiveBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ShirotsuraVisual = CreateDefaultSubobject<UShirotsuraVisualComponent>(TEXT("ShirotsuraVisual"));
    ShirotsuraVisual->Configure(GetMesh(), PrimitiveBody, nullptr, nullptr, ECampaignState::Magatsune);
}

void AMagatsunePlayer::ConfigureBoss(AMagatsuneBoss* V)
{
    Boss = V;
    AddTickPrerequisiteActor(Boss);
    Grab->AddTickPrerequisiteActor(this);
    SetPrimitiveColor(FindComponentByClass<UStaticMeshComponent>()->CreateDynamicMaterialInstance(0), FLinearColor(1, .66, .12));
    Sense->ClearBoundaryTargets();
    for (int32 I = 0; I < Boss->GetKakonCount(); ++I) Sense->RegisterBoundaryTarget(Boss->GetKakon(I), I == 0);
}

bool AMagatsunePlayer::IsMounted() const { return Grab->IsGrabbing(); }

void AMagatsunePlayer::SetupPlayerInputComponent(UInputComponent* I)
{
    Super::SetupPlayerInputComponent(I);
    I->BindAxis(TEXT("MoveForward"), this, &AMagatsunePlayer::Forward);
    I->BindAxis(TEXT("MoveRight"), this, &AMagatsunePlayer::Right);
    I->BindAxis(TEXT("Turn"), this, &AMagatsunePlayer::Turn);
    I->BindAxis(TEXT("LookUp"), this, &AMagatsunePlayer::Look);
    I->BindAction(TEXT("Grab"), IE_Pressed, this, &AMagatsunePlayer::GrabPressed);
    I->BindAction(TEXT("Grab"), IE_Released, this, &AMagatsunePlayer::GrabReleased);
    I->BindAction(TEXT("Jump"), IE_Pressed, this, &AMagatsunePlayer::JumpPressed);
    I->BindAction(TEXT("Attack"), IE_Pressed, this, &AMagatsunePlayer::PurifyPressed);
    I->BindAction(TEXT("Retry"), IE_Pressed, this, &AMagatsunePlayer::RetryPressed);
    I->BindAction(TEXT("BoundarySense"), IE_Pressed, this, &AMagatsunePlayer::BoundarySensePressed);
    I->BindAction(TEXT("BoundarySense"), IE_Released, this, &AMagatsunePlayer::BoundarySenseReleased);
    I->BindAction(TEXT("ArmSense"), IE_Pressed, this, &AMagatsunePlayer::ArmSensePressed);
    I->BindAction(TEXT("ArmSense"), IE_Released, this, &AMagatsunePlayer::ArmSenseReleased);
}

void AMagatsunePlayer::Forward(float V)
{
    ForwardInput = V;
    if (Boss && !IsMounted() && !bFalling) AddMovementInput(FRotator(0, GetControlRotation().Yaw, 0).Vector(), V);
}

void AMagatsunePlayer::Right(float V)
{
    if (Boss && !IsMounted() && !bFalling)
        AddMovementInput(FRotationMatrix(FRotator(0, GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::Y), V);
}

void AMagatsunePlayer::Turn(float V) { AddControllerYawInput(V); }

void AMagatsunePlayer::Look(float V) { AddControllerPitchInput(V); }

void AMagatsunePlayer::GrabPressed()
{
    bGripHeld = true;
    if (!Boss || IsMounted() || bFalling || Stamina->GetCurrentStamina() < MinimumGrabStamina) return;
    ++Boss->Telemetry.GrabAttempts;
    const int32 Start = bRecovering ? Boss->GetRecoveryNode() : 0;
    const FVector Anchor = bRecovering ? Boss->GetRecoveryWorld() : Boss->GetRouteWorld(0);
    if (FVector::Dist(GetActorLocation(), Anchor) > GrabRange) return;
    if (!Grab->TryGrab(Boss->GetGrabFrame(), FVector::Dist(GetActorLocation(), Boss->GetGrabFrame()->GetActorLocation()) + 1)) return;
    Node = Start;
    Destination = INDEX_NONE;
    Grab->SetRelativeGrabTransform(FTransform(FRotator(0, 0, 0), Boss->GetRouteLocal(Start)));
    ++Boss->Telemetry.GrabSuccesses;
    if (bRecovering)
    {
        bRecovering = false;
        RecoveryGrace = 2;
        ++Boss->Telemetry.Recoveries;
        Boss->LogTelemetry(TEXT("Recovery_Regrab"));
    }
}

void AMagatsunePlayer::JumpPressed()
{
    if (IsMounted()) Fall();
    else Jump();
}

void AMagatsunePlayer::PurifyPressed()
{
    if (Boss && Boss->TryPurify())
    {
        Sense->NotifyPurifyAfterSense();
        ShirotsuraVisual->PlayOneShot(EShirotsuraVisualState::Slash);
    }
}

void AMagatsunePlayer::RetryPressed()
{
    if (auto* M = GetWorld()->GetAuthGameMode<AMagatsuneGameMode>()) M->RetryEncounter();
}

void AMagatsunePlayer::ResolveLargePulse()
{
    if (!IsMounted()) return;
    if (!IsClinging() || Stamina->GetCurrentStamina() <= PulseCost)
    {
        Fall(Stamina->GetCurrentStamina() <= PulseCost);
        return;
    }
    Stamina->ConsumeStamina(PulseCost);
}

void AMagatsunePlayer::Fall(bool Exhausted)
{
    if (!IsMounted()) return;
    Grab->Release();
    Node = Destination = INDEX_NONE;
    bGripHeld = false;
    bFalling = true;
    ++Boss->Telemetry.Falls;
    if (Exhausted) ++Boss->Telemetry.Exhaustions;
    LaunchCharacter({-250, 100, 100}, true, true);
    Boss->LogTelemetry(Exhausted ? TEXT("StaminaExhaustion_Fall") : TEXT("Fall"));
}

void AMagatsunePlayer::Tick(float Dt)
{
    Super::Tick(Dt);
    const EShirotsuraVisualState VisualState = IsMounted() ? (IsRouteMoving()       ? EShirotsuraVisualState::Climb
                                                                     : IsClinging() ? EShirotsuraVisualState::Grip
                                                                                    : EShirotsuraVisualState::Hang)
        : GetCharacterMovement()->IsFalling()              ? EShirotsuraVisualState::Jump
        : GetVelocity().Size2D() > 5                       ? EShirotsuraVisualState::Run
                                                           : EShirotsuraVisualState::Idle;
    ShirotsuraVisual->SetState(VisualState);
    ShirotsuraVisual->SetWeaponHidden(IsMounted());
    UpdateSenseFromBoss();
    if (!Boss || !FMath::IsFinite(Dt) || Dt <= 0) return;
    if (bFalling)
    {
        if (GetCharacterMovement()->IsMovingOnGround() || GetActorLocation().Z < -100)
        {
            bFalling = false;
            bRecovering = true;
            SetActorLocation(Boss->GetRecoveryWorld());
            GetCharacterMovement()->StopMovementImmediately();
            Boss->LogTelemetry(TEXT("Recovery"));
        }
        return;
    }
    if (!IsMounted())
    {
        Stamina->RestoreStamina((24 * Dt) * Sense->GetRecoveryMultiplier());
        return;
    }
    if (IsClinging()) Boss->Telemetry.ClingSeconds += Dt;
    RecoveryGrace = FMath::Max(0.f, RecoveryGrace - Dt);
    if (Boss->IsSafeNode(Node) && !IsRouteMoving()) Stamina->RestoreStamina((24 * Dt) * Sense->GetRecoveryMultiplier());
    else if (RecoveryGrace <= 0) Stamina->ConsumeStamina(((IsRouteMoving() ? 5.f : 2.f) + (IsClinging() ? 1.f : 0.f)) * Dt);
    if (Stamina->IsDepleted())
    {
        Fall(true);
        return;
    }
    if (Boss->IsLargePulse() || !Boss->IsRouteNodeEnabled(Node)) return;
    RouteDelay = FMath::Max(0.f, RouteDelay - Dt);
    if (!IsRouteMoving() && RouteDelay <= 0 && FMath::Abs(ForwardInput) > .4f)
    {
        int32 Next = Node + (ForwardInput > 0 ? 1 : -1);
        if (Boss->IsRouteNodeEnabled(Next))
        {
            Destination = Next;
            MoveProgress = 0;
        }
    }
    if (!IsRouteMoving()) return;
    const FVector A = Boss->GetRouteLocal(Node), B = Boss->GetRouteLocal(Destination);
    MoveProgress = FMath::Min(1.f, MoveProgress + RouteSpeed * Dt / FMath::Max(1.f, FVector::Dist(A, B)));
    Grab->SetRelativeGrabTransform(FTransform(FRotator::ZeroRotator, FMath::Lerp(A, B, MoveProgress)));
    if (MoveProgress >= 1)
    {
        Node = Destination;
        Destination = INDEX_NONE;
        RouteDelay = .2f;
    }
}

void AMagatsunePlayer::ResetForEncounter()
{
    Grab->Release();
    Node = Destination = INDEX_NONE;
    Sense->ResetSense();
    ShirotsuraVisual->ResetPresentation();
    ForwardInput = MoveProgress = RouteDelay = RecoveryGrace = 0;
    bGripHeld = bFalling = bRecovering = false;
    SmoothedCameraFocus = SmoothedCameraLocation = FVector::ZeroVector;
    CameraPresentationFOV = 82.f;
    bCameraInitialized = false;
    Stamina->ResetStamina();
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    SetActorTransform(SpawnTransform(), false, nullptr, ETeleportType::TeleportPhysics);
    if (Controller) Controller->SetControlRotation(FRotator(-12, 0, 0));
}

void AMagatsunePlayer::CalcCamera(float Dt, FMinimalViewInfo& V)
{
    const FVector PlayerFocus = IsMounted() ? GetActorLocation() : FVector(300, 0, 700);
    const float Transition = Boss ? Boss->GetTransitionAlpha() : 0.f;
    const float Cue = Boss ? FMath::Max(Boss->GetAnticipation(), Boss->GetPurificationPresentation()) : 0.f;
    const float FocusWeight = Boss ? FMath::Clamp(.18f * Transition + .10f * Cue + .12f * Boss->GetCalmPresentation(), 0.f, .30f) : 0.f;
    const FVector DesiredFocus = Boss ? FMath::Lerp(PlayerFocus, Boss->GetPresentationFocus(), FocusWeight) : PlayerFocus;
    if (!bCameraInitialized) { SmoothedCameraFocus = DesiredFocus; bCameraInitialized = true; }
    SmoothedCameraFocus = FMath::VInterpTo(SmoothedCameraFocus, DesiredFocus, Dt, 2.4f);
    const FRotator Orbit(FMath::Clamp(GetControlRotation().Pitch, -55.f, -5.f), GetControlRotation().Yaw, 0);
    const float Distance = (IsMounted() ? 1600.f : 2300.f) + 180.f * Transition + 100.f * Cue;
    const FVector DesiredLocation = SmoothedCameraFocus - Orbit.Vector() * Distance;
    if (SmoothedCameraLocation.IsNearlyZero()) SmoothedCameraLocation = DesiredLocation;
    SmoothedCameraLocation = FMath::VInterpTo(SmoothedCameraLocation, DesiredLocation, Dt, 3.2f);
    V.Location = SmoothedCameraLocation;
    V.Rotation = (SmoothedCameraFocus - V.Location).Rotation();
    const float Purify = Boss ? Boss->GetPurificationPresentation() : 0.f;
    const float Calm = Boss ? Boss->GetCalmPresentation() : 0.f;
    CameraPresentationFOV = FMath::FInterpTo(CameraPresentationFOV, 82.f + 2.f * Cue + Purify - Calm, Dt, 3.f);
    V.FOV = CameraPresentationFOV;
}

void AMagatsunePlayer::BoundarySensePressed()
{
    Sense->BeginBoundarySense();
    if (Boss) Boss->RefreshSenseGuidance();
}

void AMagatsunePlayer::BoundarySenseReleased()
{
    Sense->EndBoundarySense();
    if (Boss) Boss->RefreshSenseGuidance();
}

ECorruptionWarning AMagatsunePlayer::ComputeCorruptionWarning() const
{
    if (!Boss) return ECorruptionWarning::None;
    if (Boss->IsLargePulse()) return ECorruptionWarning::Danger;
    if (Boss->GetPhase() == EMagatsunePhase::RootRockRoute || Boss->GetPhase() == EMagatsunePhase::FinalRise)
        return ECorruptionWarning::Transition;
    return Boss->IsSafeNode(Node) ? ECorruptionWarning::Safe : ECorruptionWarning::None;
}

void AMagatsunePlayer::UpdateSenseFromBoss()
{
    if (Boss)
    {
        const int32 Current = Boss->GetNushiProgressComponent()->GetPurifiedCount();
        for (int32 I = 0; I < Boss->GetKakonCount(); ++I) Sense->SetBoundaryTargetAvailable(Boss->GetKakon(I), I == Current);
    }
    if (Sense->IsCorruptionSenseActive()) Sense->SetCorruptionWarning(ComputeCorruptionWarning());
}

void AMagatsunePlayer::ArmSensePressed()
{
    Sense->BeginCorruptionSense();
    Sense->SetCorruptionWarning(ComputeCorruptionWarning());
}

void AMagatsunePlayer::ArmSenseReleased() { Sense->EndCorruptionSense(); }
