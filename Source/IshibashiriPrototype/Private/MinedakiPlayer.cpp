#include "MinedakiPlayer.h"
#include "MinedakiBoss.h"
#include "MinedakiGameMode.h"
#include "GrabComponent.h"
#include "StaminaComponent.h"
#include "PlayerSenseComponent.h"
#include "NushiProgressComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraTypes.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "PrimitiveAppearance.h"
#include "ShirotsuraVisualComponent.h"

AMinedakiPlayer::AMinedakiPlayer()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;
    GetCapsuleComponent()->InitCapsuleSize(34, 88);
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->MaxWalkSpeed = 480;
    GetCharacterMovement()->bOrientRotationToMovement = true;
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
    PrimitiveBody->SetRelativeScale3D(FVector(.7, .7, 1.65));
    PrimitiveBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ShirotsuraVisual = CreateDefaultSubobject<UShirotsuraVisualComponent>(TEXT("ShirotsuraVisual"));
    ShirotsuraVisual->Configure(GetMesh(), PrimitiveBody);
}

void AMinedakiPlayer::ConfigureBoss(AMinedakiBoss* InBoss)
{
    Boss = InBoss;
    AddTickPrerequisiteActor(Boss);
    Grab->AddTickPrerequisiteActor(this);
    SetPrimitiveColor(PrimitiveBody->CreateDynamicMaterialInstance(0), FLinearColor(1, .72, .08));
    Sense->ClearBoundaryTargets();
    for (int32 I = 0; I < Boss->GetKakonCount(); ++I) Sense->RegisterBoundaryTarget(Boss->GetKakon(I), I == 0);
}

bool AMinedakiPlayer::CanAct() const { return Boss && Boss->GetNushiState() == ENushiState::Active && !bFallen; }

bool AMinedakiPlayer::IsMounted() const { return Grab->IsGrabbing(); }

void AMinedakiPlayer::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("MoveForward"), this, &AMinedakiPlayer::Forward);
    Input->BindAxis(TEXT("MoveRight"), this, &AMinedakiPlayer::Right);
    Input->BindAxis(TEXT("Turn"), this, &AMinedakiPlayer::Turn);
    Input->BindAxis(TEXT("LookUp"), this, &AMinedakiPlayer::Look);
    Input->BindAxis(TEXT("TurnRate"), this, &AMinedakiPlayer::TurnRate);
    Input->BindAxis(TEXT("LookUpRate"), this, &AMinedakiPlayer::LookRate);
    Input->BindAction(TEXT("Grab"), IE_Pressed, this, &AMinedakiPlayer::GrabPressed);
    Input->BindAction(TEXT("Grab"), IE_Released, this, &AMinedakiPlayer::GrabReleased);
    Input->BindAction(TEXT("Jump"), IE_Pressed, this, &AMinedakiPlayer::JumpPressed);
    Input->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
    Input->BindAction(TEXT("Attack"), IE_Pressed, this, &AMinedakiPlayer::AttackPressed);
    Input->BindAction(TEXT("Retry"), IE_Pressed, this, &AMinedakiPlayer::RetryPressed);
    Input->BindAction(TEXT("BoundarySense"), IE_Pressed, this, &AMinedakiPlayer::BoundarySensePressed);
    Input->BindAction(TEXT("BoundarySense"), IE_Released, this, &AMinedakiPlayer::BoundarySenseReleased);
    Input->BindAction(TEXT("ArmSense"), IE_Pressed, this, &AMinedakiPlayer::ArmSensePressed);
    Input->BindAction(TEXT("ArmSense"), IE_Released, this, &AMinedakiPlayer::ArmSenseReleased);
}

void AMinedakiPlayer::Forward(float Value)
{
    ForwardInput = Value;
    if (CanAct() && !IsMounted()) AddMovementInput(FRotator(0, GetControlRotation().Yaw, 0).Vector(), Value);
}

void AMinedakiPlayer::Right(float Value)
{
    if (CanAct() && !IsMounted()) AddMovementInput(FRotationMatrix(FRotator(0, GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::Y), Value);
}

void AMinedakiPlayer::Turn(float Value) { AddControllerYawInput(Value); }

void AMinedakiPlayer::Look(float Value) { AddControllerPitchInput(Value); }

void AMinedakiPlayer::TurnRate(float Value) { Turn(Value * 55 * GetWorld()->GetDeltaSeconds()); }

void AMinedakiPlayer::LookRate(float Value) { Look(Value * 45 * GetWorld()->GetDeltaSeconds()); }

void AMinedakiPlayer::GrabPressed()
{
    bGripHeld = true;
    if (!CanAct() || IsMounted()) return;
    ++Boss->Telemetry.GrabAttempts;
    const int32 StartNode = bRecovering ? Boss->GetRecoveryNode() : 0;
    const FVector Anchor = bRecovering ? Boss->GetRecoveryAnchorWorld() : Boss->GetRouteWorld(0);
    if ((!bRecovering && Boss->GetActionState() != EMinedakiActionState::Grounded) || Stamina->GetCurrentStamina() < MinimumGrabStamina ||
        FVector::Dist(GetActorLocation(), Anchor) > GrabRange)
        return;
    // Range is validated against the leg; target is the unscaled parent body frame.
    if (!Grab->TryGrab(Boss->GetGrabFrame(), FVector::Dist(GetActorLocation(), Boss->GetGrabFrame()->GetActorLocation()) + 1)) return;
    Node = StartNode;
    Destination = INDEX_NONE;
    Progress = 0;
    RouteDelay = .2f;
    Grab->SetRelativeGrabTransform(FTransform(FRotator(0, 180, 0), Boss->GetRouteLocal(StartNode)));
    GetCharacterMovement()->bOrientRotationToMovement = false;
    ++Boss->Telemetry.GrabSuccesses;
    Boss->NotifyRouteNode(StartNode);
    if (bRecovering)
    {
        bRecovering = false;
        RecoveryGrace = 2.f;
        ++Boss->Telemetry.RecoverySuccesses;
        Boss->LogTelemetry(TEXT("RecoverySuccess_Regrab"));
    }
    else Boss->LogTelemetry(TEXT("LegGrab"));
}

void AMinedakiPlayer::JumpPressed()
{
    if (CanAct())
    {
        if (IsMounted()) Fall();
        else Jump();
    }
}

void AMinedakiPlayer::AttackPressed()
{
    if (CanAct() && Boss->TryPurifyKakon())
    {
        Sense->NotifyPurifyAfterSense();
        ShirotsuraVisual->PlayOneShot(EShirotsuraVisualState::Slash);
    }
}

void AMinedakiPlayer::RetryPressed()
{
    if (auto* Mode = GetWorld()->GetAuthGameMode<AMinedakiGameMode>()) Mode->RetryEncounter();
}

void AMinedakiPlayer::ResolveShake()
{
    if (!IsMounted()) return;
    if (!IsClinging() || Stamina->GetCurrentStamina() <= ShakeCost)
    {
        ++Boss->Telemetry.ShakeFailures;
        Fall(Stamina->GetCurrentStamina() <= ShakeCost);
        return;
    }
    Stamina->ConsumeStamina(ShakeCost);
    ++Boss->Telemetry.ShakeSuccesses;
}

void AMinedakiPlayer::Fall(bool bExhausted)
{
    if (!IsMounted()) return;
    Grab->Release();
    Node = Destination = INDEX_NONE;
    bGripHeld = false;
    bFallen = true;
    ++Boss->Telemetry.Falls;
    if (bExhausted) ++Boss->Telemetry.Exhaustions;
    SetActorRotation(FRotator(0, 180, 0));
    GetCharacterMovement()->bOrientRotationToMovement = true;
    LaunchCharacter(FVector(400, -200, 0), true, true);
    Boss->LogTelemetry(bExhausted ? TEXT("StaminaFall") : TEXT("Fall"));
}

void AMinedakiPlayer::Tick(float Dt)
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
    if (bFallen)
    {
        if (GetCharacterMovement()->IsMovingOnGround() || GetActorLocation().Z < -200)
        {
            bFallen = false;
            bRecovering = true;
            ++Boss->Telemetry.RecoveryStarts;
            Stamina->RestoreStamina((35.f) * Sense->GetRecoveryMultiplier());
            SetActorLocation(Boss->GetRecoveryAnchorWorld());
            GetCharacterMovement()->StopMovementImmediately();
            Boss->LogTelemetry(TEXT("RecoveryStarted_Anchor"));
        }
        return;
    }
    if (!CanAct()) return;
    if (!IsMounted())
    {
        Stamina->RestoreStamina((RestRestore * Dt) * Sense->GetRecoveryMultiplier());
        return;
    }
    if (IsClinging()) Boss->Telemetry.ClingSeconds += Dt;
    RecoveryGrace = FMath::Max(0.f, RecoveryGrace - Dt);
    const bool Wall = Boss->IsWallMoving() || Boss->IsBodyTransitioning();
    const bool Rest = (Node == 4 || Node == 7 || Node == 11) && !IsRouteMoving();
    if (Rest) Stamina->RestoreStamina((RestRestore * Dt) * Sense->GetRecoveryMultiplier());
    else if (RecoveryGrace <= 0)
        Stamina->ConsumeStamina((Wall ? WallDrain + (IsClinging() ? ClingExtraDrain : 0) : IsRouteMoving() ? ClimbDrain : GrabDrain) * Dt);
    if (Stamina->IsDepleted())
    {
        Fall(true);
        return;
    }
    if (Wall) return;
    RouteDelay = FMath::Max(0.f, RouteDelay - Dt);
    if (!IsRouteMoving() && RouteDelay <= 0 && FMath::Abs(ForwardInput) > .4f)
    {
        int32 Next = Node + (ForwardInput > 0 ? 1 : -1);
        if (Boss->IsRouteNodeEnabled(Next))
        {
            Destination = Next;
            Progress = 0;
        }
    }
    if (!IsRouteMoving()) return;
    const FVector From = Boss->GetRouteLocal(Node), To = Boss->GetRouteLocal(Destination);
    Progress = FMath::Min(1.f, Progress + RouteSpeed * Dt / FMath::Max(1.f, FVector::Dist(From, To)));
    Grab->SetRelativeGrabTransform(FTransform(FRotator(0, 180, 0), FMath::Lerp(From, To, Progress)));
    if (Progress >= 1)
    {
        Node = Destination;
        Destination = INDEX_NONE;
        RouteDelay = .22f;
        Boss->NotifyRouteNode(Node);
    }
}

void AMinedakiPlayer::ResetForEncounter()
{
    Grab->Release();
    StopJumping();
    ConsumeMovementInputVector();
    Sense->ResetSense();
    ShirotsuraVisual->ResetPresentation();
    Node = Destination = INDEX_NONE;
    ForwardInput = Progress = RouteDelay = RecoveryGrace = 0;
    bGripHeld = bFallen = bRecovering = false;
    Stamina->ResetStamina();
    GetCharacterMovement()->ClearAccumulatedForces();
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    GetCharacterMovement()->bOrientRotationToMovement = true;
    SetActorTransform(SpawnTransform(), false, nullptr, ETeleportType::TeleportPhysics);
    if (Controller) Controller->SetControlRotation(FRotator(-18, 145, 0));
}

void AMinedakiPlayer::CalcCamera(float Dt, FMinimalViewInfo& View)
{
    const FVector Focus = IsMounted() ? GetActorLocation() + FVector(0, 0, 80) : FMath::Lerp(GetActorLocation(), FVector(0, 0, 1800), .6f);
    const float Distance = IsMounted() ? 2100.f : 3200.f;
    const FRotator Orbit(FMath::Clamp(GetControlRotation().Pitch, -60.f, -8.f), GetControlRotation().Yaw, 0);
    View.Location = Focus - Orbit.Vector() * Distance;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(MinedakiCamera), false, this);
    if (GetWorld()->SweepSingleByChannel(Hit, Focus, View.Location, FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(18), Params))
        View.Location = Hit.Location;
    View.Rotation = (Focus - View.Location).Rotation();
    View.FOV = 80;
}

ECorruptionWarning AMinedakiPlayer::ComputeCorruptionWarning() const
{
    if (!Boss) return ECorruptionWarning::None;
    if (Boss->GetActionState() == EMinedakiActionState::Shaking) return ECorruptionWarning::Danger;
    return Boss->IsBodyTransitioning() ? ECorruptionWarning::Transition : ECorruptionWarning::None;
}

void AMinedakiPlayer::UpdateSenseFromBoss()
{
    if (Boss)
    {
        const int32 Current = Boss->GetNushiProgressComponent()->GetPurifiedCount();
        for (int32 I = 0; I < Boss->GetKakonCount(); ++I) Sense->SetBoundaryTargetAvailable(Boss->GetKakon(I), I == Current);
    }
    if (Sense->IsCorruptionSenseActive()) Sense->SetCorruptionWarning(ComputeCorruptionWarning());
}

void AMinedakiPlayer::BoundarySensePressed()
{
    Sense->BeginBoundarySense();
    if (Boss) Boss->RefreshSenseGuidance();
}

void AMinedakiPlayer::BoundarySenseReleased()
{
    Sense->EndBoundarySense();
    if (Boss) Boss->RefreshSenseGuidance();
}

void AMinedakiPlayer::ArmSensePressed()
{
    Sense->BeginCorruptionSense();
    Sense->SetCorruptionWarning(ComputeCorruptionWarning());
}

void AMinedakiPlayer::ArmSenseReleased() { Sense->EndCorruptionSense(); }
