#include "FuchimatoiPlayer.h"
#include "FuchimatoiBoss.h"
#include "FuchimatoiGameMode.h"
#include "FuchimatoiRouteAnchor.h"
#include "FuchimatoiSimulationComponent.h"
#include "GrabComponent.h"
#include "StaminaComponent.h"
#include "PlayerSenseComponent.h"
#include "NushiProgressComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "PrimitiveAppearance.h"
#include "ShirotsuraVisualComponent.h"
#include "UObject/ConstructorHelpers.h"

AFuchimatoiPlayer::AFuchimatoiPlayer()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;
    GetCapsuleComponent()->InitCapsuleSize(34, 88);
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0, 720, 0);
    GetCharacterMovement()->MaxWalkSpeed = 480;
    GetCharacterMovement()->JumpZVelocity = 520;
    Grab = CreateDefaultSubobject<UGrabComponent>(TEXT("SharedGrab"));
    Stamina = CreateDefaultSubobject<UStaminaComponent>(TEXT("SharedStamina"));
    Sense = CreateDefaultSubobject<UPlayerSenseComponent>(TEXT("PlayerSense"));
    Arm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
    Arm->SetupAttachment(RootComponent);
    Arm->TargetArmLength = 1050;
    Arm->TargetOffset = FVector(0, 0, 110);
    Arm->bUsePawnControlRotation = true;
    Arm->bDoCollisionTest = true;
    Arm->ProbeSize = 18;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Arm);
    Camera->FieldOfView = 80;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    PrimitiveBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrimitiveClimber"));
    PrimitiveBody->SetupAttachment(RootComponent);
    PrimitiveBody->SetStaticMesh(Sphere.Object);
    PrimitiveBody->SetMaterial(0, Material.Object);
    PrimitiveBody->SetRelativeScale3D(FVector(.65, .65, 1.35));
    PrimitiveBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    UStaticMeshComponent* Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClimberHead"));
    Head->SetupAttachment(RootComponent);
    Head->SetStaticMesh(Sphere.Object);
    Head->SetMaterial(0, Material.Object);
    Head->SetRelativeLocation(FVector(0, 0, 65));
    Head->SetRelativeScale3D(FVector(.45));
    Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ShirotsuraVisual = CreateDefaultSubobject<UShirotsuraVisualComponent>(TEXT("ShirotsuraVisual"));
    ShirotsuraVisual->Configure(GetMesh(), PrimitiveBody, nullptr, Head, ECampaignState::Fuchimatoi);
}

void AFuchimatoiPlayer::ConfigureBoss(AFuchimatoiBoss* InBoss)
{
    Boss = InBoss;
    AddTickPrerequisiteComponent(Boss->FindComponentByClass<UFuchimatoiSimulationComponent>());
    Grab->AddTickPrerequisiteActor(this);
    TArray<UStaticMeshComponent*> Parts;
    GetComponents(Parts);
    for (UStaticMeshComponent* Part : Parts) SetPrimitiveColor(Part->CreateDynamicMaterialInstance(0), FLinearColor(.93, .76, .34));
    Sense->ClearBoundaryTargets();
    for (int32 I = 0; I < Boss->GetKakonCount(); ++I) Sense->RegisterBoundaryTarget(Boss->GetKakon(I), I == 0);
}

bool AFuchimatoiPlayer::CanAct() const
{
    const AFuchimatoiGameMode* Mode = GetWorld()->GetAuthGameMode<AFuchimatoiGameMode>();
    return Mode && Mode->IsEncounterActive();
}

bool AFuchimatoiPlayer::IsMounted() const { return Grab->IsGrabbing(); }

int32 AFuchimatoiPlayer::GetGuidanceNode() const
{
    if (Destination != INDEX_NONE) return Destination;
    return Boss ? FMath::Clamp(Node + (ForwardInput < -.4f ? -1 : 1), 0, Boss->IsCoilingComplete() ? 9 : 3) : INDEX_NONE;
}

bool AFuchimatoiPlayer::IsOnRecoveryGround() const
{
    // Arena floor is Z=0. Standing on an elevated rock is not ground recovery.
    return !IsMounted() && GetCharacterMovement()->IsMovingOnGround() && GetActorLocation().Z < 200.f;
}

void AFuchimatoiPlayer::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
    Super::CalcCamera(DeltaTime, OutResult);
    if (Boss)
    {
        const float Attack=Boss->GetAttackPresentation();
        const float Purify=Boss->GetPurificationPresentation();
        const float Calm=Boss->GetCalmPresentation();
        const FVector PlayerFocus=GetActorLocation()+FVector(0,0,35);
        const float BossWeight=IsMounted()?.18f:FMath::Lerp(.10f,.28f,Attack);
        const FVector DesiredFocus=FMath::Lerp(PlayerFocus,Boss->GetHeadWorldLocation(),BossWeight);
        if (SmoothedCameraFocus.IsNearlyZero()) SmoothedCameraFocus=DesiredFocus;
        SmoothedCameraFocus=FMath::VInterpTo(SmoothedCameraFocus,DesiredFocus,DeltaTime,2.6f);
        const float DesiredFOV=80.f+Attack*3.f+Purify*1.5f-Calm*1.f;
        CameraPresentationFOV=FMath::FInterpTo(CameraPresentationFOV,DesiredFOV,DeltaTime,3.5f);
        OutResult.FOV=CameraPresentationFOV;
        // Keep player agency: only bias pitch/yaw toward the creature and never
        // overwrite the controller or add oscillating camera shake.
        const FRotator Framed=(SmoothedCameraFocus-OutResult.Location).Rotation();
        OutResult.Rotation=FMath::RInterpTo(OutResult.Rotation,Framed,DeltaTime,Attack>0.f?1.8f:.55f);
    }
    // Supplement the spring arm with the same player-to-camera sphere retreat
    // used by the Ishibashiri climbing camera. Its offset pivot can miss a ledge.
    const FVector Focus = GetActorLocation() + FVector(0, 0, 35);
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(FuchimatoiCameraRetreat), false, this);
    if (GetWorld()->SweepSingleByChannel(
            Hit, Focus, OutResult.Location, FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(18.f), Params))
    {
        OutResult.Location = Hit.Location;
        OutResult.Rotation = (Focus - OutResult.Location).Rotation();
    }
}

void AFuchimatoiPlayer::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("MoveForward"), this, &AFuchimatoiPlayer::MoveForward);
    Input->BindAxis(TEXT("MoveRight"), this, &AFuchimatoiPlayer::MoveRight);
    Input->BindAxis(TEXT("Turn"), this, &AFuchimatoiPlayer::Turn);
    Input->BindAxis(TEXT("LookUp"), this, &AFuchimatoiPlayer::Look);
    Input->BindAxis(TEXT("TurnRate"), this, &AFuchimatoiPlayer::TurnRate);
    Input->BindAxis(TEXT("LookUpRate"), this, &AFuchimatoiPlayer::LookRate);
    Input->BindAction(TEXT("Grab"), IE_Pressed, this, &AFuchimatoiPlayer::GrabPressed);
    Input->BindAction(TEXT("Jump"), IE_Pressed, this, &AFuchimatoiPlayer::JumpPressed);
    Input->BindAction(TEXT("Dodge"), IE_Pressed, this, &AFuchimatoiPlayer::DodgePressed);
    Input->BindAction(TEXT("Attack"), IE_Pressed, this, &AFuchimatoiPlayer::AttackPressed);
    Input->BindAction(TEXT("Retry"), IE_Pressed, this, &AFuchimatoiPlayer::RetryPressed);
    Input->BindAction(TEXT("BoundarySense"), IE_Pressed, this, &AFuchimatoiPlayer::BoundarySensePressed);
    Input->BindAction(TEXT("BoundarySense"), IE_Released, this, &AFuchimatoiPlayer::BoundarySenseReleased);
    Input->BindAction(TEXT("ArmSense"), IE_Pressed, this, &AFuchimatoiPlayer::ArmSensePressed);
    Input->BindAction(TEXT("ArmSense"), IE_Released, this, &AFuchimatoiPlayer::ArmSenseReleased);
}

void AFuchimatoiPlayer::MoveForward(float Value)
{
    ForwardInput = Value;
    if (CanAct() && !IsMounted() && !IsDodging()) AddMovementInput(FRotator(0, GetControlRotation().Yaw, 0).Vector(), Value);
}

void AFuchimatoiPlayer::MoveRight(float Value)
{
    RightInput = Value;
    if (CanAct() && !IsMounted() && !IsDodging())
        AddMovementInput(FRotationMatrix(FRotator(0, GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::Y), Value);
}

void AFuchimatoiPlayer::Turn(float Value)
{
    if (CanAct()) AddControllerYawInput(Value);
}

void AFuchimatoiPlayer::Look(float Value)
{
    if (CanAct()) AddControllerPitchInput(Value);
}

void AFuchimatoiPlayer::TurnRate(float Value) { Turn(Value * 55 * GetWorld()->GetDeltaSeconds()); }

void AFuchimatoiPlayer::LookRate(float Value) { Look(Value * 45 * GetWorld()->GetDeltaSeconds()); }

void AFuchimatoiPlayer::GrabPressed()
{
    if (!CanAct() || IsMounted() || !Boss) return;
    const bool bRecovery = Boss->CanRecover();
    AFuchimatoiRouteAnchor* Anchor = bRecovery ? Boss->GetRecoveryAnchor() : Boss->GetRouteAnchor(0);
    const bool bSuccess = !IsDodging() && (bRecovery || Boss->CanMount()) && Stamina->GetCurrentStamina() >= MinimumGrabStamina &&
        Grab->TryGrab(Anchor, bRecovery ? RecoveryGrabRange : GrabRange);
    Boss->RecordGrab(bSuccess, bRecovery);
    if (bSuccess)
    {
        bRecoveryApproach = bRecovery;
        Node = bRecovery ? Boss->RecoveryNode : 0;
        Destination = bRecovery ? Node : INDEX_NONE;
        RouteProgress = 0;
        RouteDelay = .2f;
        // Brief drain relief only, no health immunity. Minimum 25 is required.
        RecoveryStaminaGrace = bRecovery ? 3.f : 0.f;
        Grab->SetRelativeGrabTransform(FTransform::Identity);
        GetCharacterMovement()->bOrientRotationToMovement = false;
    }
}

void AFuchimatoiPlayer::DetachFromRoute()
{
    if (IsMounted() && Boss) Boss->RecordFall();
    Grab->Release();
    Node = Destination = INDEX_NONE;
    bRecoveryApproach = false;
    RecoveryStaminaGrace = 0;
    GetCharacterMovement()->bOrientRotationToMovement = true;
}

void AFuchimatoiPlayer::JumpPressed()
{
    if (!CanAct()) return;
    if (IsMounted())
    {
        DetachFromRoute();
        LaunchCharacter(FVector(0, -180, 260), true, true);
    }
    else Jump();
}

void AFuchimatoiPlayer::DodgePressed()
{
    if (!CanAct() || IsMounted() || IsDodging() || DodgeCooldown > 0 || GetCharacterMovement()->IsFalling()) return;
    const FRotationMatrix Rotation(FRotator(0, GetControlRotation().Yaw, 0));
    DodgeDirection = (Rotation.GetUnitAxis(EAxis::X) * ForwardInput + Rotation.GetUnitAxis(EAxis::Y) * RightInput).GetSafeNormal();
    if (DodgeDirection.IsNearlyZero()) DodgeDirection = GetActorRightVector();
    DodgeRemaining = .38f;
    DodgeCooldown = .85f;
    GetCharacterMovement()->Velocity = DodgeDirection * 1100.f;
}

void AFuchimatoiPlayer::AttackPressed()
{
    if (CanAct() && Boss && Boss->TryPurifyAtNode(Node))
    {
        Sense->NotifyPurifyAfterSense();
        ShirotsuraVisual->PlayOneShot(EShirotsuraVisualState::Slash);
    }
}

void AFuchimatoiPlayer::RetryPressed()
{
    if (AFuchimatoiGameMode* Mode = GetWorld()->GetAuthGameMode<AFuchimatoiGameMode>()) Mode->RetryEncounter();
}

bool AFuchimatoiPlayer::ReceiveBite()
{
    if (!CanAct() || IsDodging() || HitImmunity > 0) return false;
    Health = FMath::Max(0, Health - 1);
    HitImmunity = 1.f;
    if (Health == 0)
    {
        if (AFuchimatoiGameMode* Mode = GetWorld()->GetAuthGameMode<AFuchimatoiGameMode>()) Mode->Defeat();
    }
    else LaunchCharacter(FVector(0, -300, 180), true, true);
    return true;
}

void AFuchimatoiPlayer::AdvanceRoute(float Dt)
{
    if (Boss->IsCoiling() && !Boss->IsCoilingComplete())
    {
        Stamina->ConsumeStamina(6.f * Dt);
        if (Stamina->IsDepleted()) DetachFromRoute();
        return;
    }
    RouteDelay = FMath::Max(0.f, RouteDelay - Dt);
    AFuchimatoiRouteAnchor* Current = bRecoveryApproach ? Boss->GetRecoveryAnchor() : Boss->GetRouteAnchor(Node);
    if (!Current)
    {
        DetachFromRoute();
        return;
    }
    const float DrainDt = FMath::Max(0.f, Dt - RecoveryStaminaGrace);
    RecoveryStaminaGrace = FMath::Max(0.f, RecoveryStaminaGrace - Dt);
    if (Current->IsRock() && Destination == INDEX_NONE) Stamina->RestoreStamina((28.f * Dt) * Sense->GetRecoveryMultiplier());
    else Stamina->ConsumeStamina((IsRouteMoving() ? 7.f : 3.f) * DrainDt);
    if (Stamina->IsDepleted())
    {
        DetachFromRoute();
        return;
    }
    if (Destination == INDEX_NONE && RouteDelay <= 0 && FMath::Abs(ForwardInput) > .4f)
    {
        const int32 Next = Node + (ForwardInput > 0 ? 1 : -1);
        const int32 MaxNode = Boss->IsCoilingComplete() ? 9 : 3;
        if (Next >= 0 && Next <= MaxNode)
        {
            Destination = Next;
            RouteProgress = 0;
        }
    }
    if (Destination == INDEX_NONE) return;
    AFuchimatoiRouteAnchor* Next = Boss->GetRouteAnchor(Destination);
    const float Distance = FVector::Dist(Current->GetActorLocation(), Next->GetActorLocation());
    RouteProgress = FMath::Min(1.f, RouteProgress + RouteSpeed * Dt / FMath::Max(1.f, Distance));
    const FVector Position = FMath::Lerp(Current->GetActorLocation(), Next->GetActorLocation(), RouteProgress);
    const FTransform WorldPose((Next->GetActorLocation() - Current->GetActorLocation()).Rotation(), Position);
    Grab->SetRelativeGrabTransform(WorldPose.GetRelativeTransform(Current->GetActorTransform()));
    if (RouteProgress >= 1)
    {
        // Reuse the same grab component for the adjacent rock or serpent anchor.
        Grab->Release();
        if (Grab->TryGrab(Next, Distance + 100.f))
        {
            Grab->SetRelativeGrabTransform(FTransform::Identity);
            Node = Destination;
        }
        else
        {
            Boss->RecordFall();
            Node = INDEX_NONE;
            GetCharacterMovement()->bOrientRotationToMovement = true;
        }
        bRecoveryApproach = false;
        Destination = INDEX_NONE;
        RouteDelay = .18f;
    }
}

void AFuchimatoiPlayer::Tick(float Dt)
{
    Super::Tick(Dt);
    const EShirotsuraVisualState VisualState = IsDodging() ? EShirotsuraVisualState::Dodge
        : IsMounted()                         ? (IsRouteMoving() ? EShirotsuraVisualState::Climb : EShirotsuraVisualState::Hang)
        : GetCharacterMovement()->IsFalling() ? EShirotsuraVisualState::Jump
        : GetVelocity().Size2D() > 5          ? EShirotsuraVisualState::Run
                                              : EShirotsuraVisualState::Idle;
    ShirotsuraVisual->SetState(VisualState);
    ShirotsuraVisual->SetWeaponHidden(IsMounted());
    UpdateSenseFromBoss();
    if (!CanAct()) return;
    HitImmunity = FMath::Max(0.f, HitImmunity - Dt);
    DodgeCooldown = FMath::Max(0.f, DodgeCooldown - Dt);
    if (IsDodging())
    {
        GetCharacterMovement()->Velocity = DodgeDirection * 1100.f;
        DodgeRemaining = FMath::Max(0.f, DodgeRemaining - Dt);
        if (!IsDodging()) GetCharacterMovement()->StopMovementImmediately();
    }
    if (IsMounted()) AdvanceRoute(Dt);
    else Stamina->RestoreStamina((18.f * Dt) * Sense->GetRecoveryMultiplier());
    Arm->TargetArmLength = FMath::FInterpTo(Arm->TargetArmLength, IsMounted() ? 1400.f : 1050.f, Dt, 3.f);
    if (GetActorLocation().Z < -400)
    {
        if (AFuchimatoiGameMode* Mode = GetWorld()->GetAuthGameMode<AFuchimatoiGameMode>()) Mode->Defeat();
    }
}

void AFuchimatoiPlayer::StopEncounter()
{
    ForwardInput = RightInput = DodgeRemaining = 0;
    GetCharacterMovement()->StopMovementImmediately();
    ConsumeMovementInputVector();
}

void AFuchimatoiPlayer::ResetForEncounter(const FTransform& Spawn)
{
    Grab->Release();
    StopJumping();
    StopEncounter();
    Sense->ResetSense();
    ShirotsuraVisual->ResetPresentation();
    Destination = Node = INDEX_NONE;
    RouteProgress = RouteDelay = 0;
    bRecoveryApproach = false;
    RecoveryStaminaGrace = 0;
    DodgeCooldown = HitImmunity = 0;
    Health = 3;
    Stamina->ResetStamina();
    GetCharacterMovement()->ClearAccumulatedForces();
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    GetCharacterMovement()->bOrientRotationToMovement = true;
    SetActorTransform(Spawn, false, nullptr, ETeleportType::TeleportPhysics);
    // Side view keeps the spring arm clear of the bait rock when mounting the head.
    if (Controller) Controller->SetControlRotation(FRotator(-22, 110, 0));
    Arm->TargetArmLength = 1050;
    SmoothedCameraFocus = FVector::ZeroVector;
    CameraPresentationFOV = 80.f;
}

ECorruptionWarning AFuchimatoiPlayer::ComputeCorruptionWarning() const
{
    if (!Boss) return ECorruptionWarning::None;
    switch (Boss->GetActionState())
    {
    case EFuchimatoiActionState::BiteWindup:
    case EFuchimatoiActionState::BiteLunge: return ECorruptionWarning::Danger;
    case EFuchimatoiActionState::Coiling: return ECorruptionWarning::Transition;
    case EFuchimatoiActionState::Snagged: return ECorruptionWarning::Safe;
    case EFuchimatoiActionState::Submerged: return ECorruptionWarning::None;
    }
    return ECorruptionWarning::None;
}

void AFuchimatoiPlayer::UpdateSenseFromBoss()
{
    if (Boss)
    {
        const int32 Current = Boss->GetNushiProgressComponent()->GetPurifiedCount();
        for (int32 I = 0; I < Boss->GetKakonCount(); ++I) Sense->SetBoundaryTargetAvailable(Boss->GetKakon(I), I == Current);
    }
    if (Sense->IsCorruptionSenseActive()) Sense->SetCorruptionWarning(ComputeCorruptionWarning());
}

void AFuchimatoiPlayer::BoundarySensePressed() { Sense->BeginBoundarySense(); }

void AFuchimatoiPlayer::BoundarySenseReleased() { Sense->EndBoundarySense(); }

void AFuchimatoiPlayer::ArmSensePressed()
{
    Sense->BeginCorruptionSense();
    Sense->SetCorruptionWarning(ComputeCorruptionWarning());
}

void AFuchimatoiPlayer::ArmSenseReleased() { Sense->EndCorruptionSense(); }
