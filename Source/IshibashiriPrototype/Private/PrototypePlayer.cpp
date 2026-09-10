#include "PrototypePlayer.h"
#include "IshibashiriBoss.h"
#include "PrototypeGameMode.h"
#include "PrimitiveAppearance.h"
#include "ColossusClimbingComponent.h"
#include "GrabComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

APrototypePlayer::APrototypePlayer()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(42.f, 88.f);
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 900.f, 0.f);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->JumpZVelocity = 500.f;
    GetCharacterMovement()->AirControl = 0.3f;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->TargetArmLength = 740.f;
    SpringArm->SocketOffset = FVector(0.f, 70.f, 110.f);
    SpringArm->bUsePawnControlRotation = true;
    SpringArm->bEnableCameraLag = false;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);
    Camera->FieldOfView = 95.f;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(RootComponent);
    Body->SetStaticMesh(Sphere.Object);
    Body->SetRelativeScale3D(FVector(0.68f, 0.68f, 1.65f));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetMaterial(0, Material.Object);

    Sword = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sword"));
    Sword->SetupAttachment(RootComponent);
    Sword->SetStaticMesh(Cube.Object);
    Sword->SetRelativeLocation(FVector(65.f, 48.f, 0.f));
    Sword->SetRelativeScale3D(FVector(1.3f, 0.07f, 0.12f));
    Sword->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GrabComponent = CreateDefaultSubobject<UGrabComponent>(TEXT("GrabComponent"));
    Climbing = CreateDefaultSubobject<UColossusClimbingComponent>(TEXT("ColossusClimbing"));
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Rigged(TEXT("/Game/Characters/Rigged/Shirotsura/SK_Shirotsura"));
    GetMesh()->SetSkeletalMesh(Rigged.Object);
    GetMesh()->SetRelativeLocation(FVector(0,0,-88));
    GetMesh()->SetRelativeRotation(FRotator(0,90,0));
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    if (Rigged.Succeeded()) { Body->SetVisibility(false); Sword->SetVisibility(false); }
    const TCHAR* Clips[] = {TEXT("Idle"),TEXT("Walk"),TEXT("Run"),TEXT("Slash"),TEXT("Dodge"),
        TEXT("Climb"),TEXT("Hang"),TEXT("Grip"),TEXT("Jump"),TEXT("Death")};
    for (const TCHAR* Clip : Clips)
        Animations.Add(LoadObject<UAnimSequence>(nullptr,*FString::Printf(TEXT("/Game/Characters/Rigged/Shirotsura/AN_Shirotsura_%s"),Clip)));
}

void APrototypePlayer::BeginPlay()
{
    Super::BeginPlay();
    Health = MaxHealth;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    BodyMaterial = Body->CreateDynamicMaterialInstance(0);
    SetPrimitiveColor(BodyMaterial, FLinearColor(0.08f, 0.5f, 0.8f));
    UpdateAnimation();
}

void APrototypePlayer::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
    Super::CalcCamera(DeltaTime, OutResult);
    UWorld* World = GetWorld();
    if (!World) return;

    const FVector PlayerLocation = GetActorLocation();
    const FVector Pivot = PlayerLocation + FVector(0.f, 0.f, 90.f);
    const APrototypeGameMode* Mode = World->GetAuthGameMode<APrototypeGameMode>();
    const AIshibashiriBoss* Boss = Mode ? Mode->GetBoss() : nullptr;
    if (Climbing->IsClimbing())
    {
        const FRotator View = Controller ? Controller->GetControlRotation() : GetActorRotation();
        FVector Location = PlayerLocation - View.Vector()*560.f + FVector(0,0,160);
        if (Boss && Boss->GetComponentsBoundingBox(true).ExpandBy(60).IsInside(Location))
        {
            const FVector Outward = (PlayerLocation-Boss->GetActorLocation()).GetSafeNormal2D();
            Location = PlayerLocation+Outward*650.f+FVector(0,0,340);
        }
        FCollisionQueryParams ClimbParams(SCENE_QUERY_STAT(ClimbCamera),false,this);
        if (Boss) ClimbParams.AddIgnoredActor(Boss);
        FHitResult CameraHit;
        if (GetWorld()->SweepSingleByChannel(CameraHit,PlayerLocation,Location,FQuat::Identity,ECC_Camera,
            FCollisionShape::MakeSphere(15),ClimbParams)) Location = CameraHit.Location;
        OutResult.Location = Location;
        OutResult.Rotation = (PlayerLocation+FVector(0,0,40)-Location).Rotation();
        bUsingRaisedCamera = false;
        bFrameBossWithCamera = false;
        CameraClearElapsed = 0.f;
        return;
    }

    // The boss deliberately ignores the Camera channel so it never shortens the
    // spring arm and causes an abrupt close-up. Detect it on the Visibility
    // channel instead, including when it is between the player and the camera
    // rather than directly surrounding the camera position.
    bool bBossObstructsView = false;
    if (Boss)
    {
        FHitResult SightHit;
        FCollisionQueryParams SightParams(SCENE_QUERY_STAT(CombatCameraSight), false, this);
        bBossObstructsView = World->LineTraceSingleByChannel(
            SightHit, Pivot, OutResult.Location, ECC_Visibility, SightParams)
            && SightHit.GetActor() == Boss;
    }

    // Check the regular spring-arm view, even while using the raised view. A small
    // hysteresis prevents rapid switching at the edge of a wall or the boar mesh.
    const float SafeDistance = MinimumCameraDistance + (bUsingRaisedCamera ? 60.f : 0.f);
    const float BossMargin = bUsingRaisedCamera ? 120.f : 70.f;
    bBossObstructsView |= Boss && Boss->GetComponentsBoundingBox(true).ExpandBy(BossMargin).IsInside(OutResult.Location);
    const bool bNeedsRaisedCamera = FVector::Dist(PlayerLocation, OutResult.Location) < SafeDistance
        || bBossObstructsView
        || (Boss && Boss->GetComponentsBoundingBox(true).ExpandBy(BossMargin).IsInside(OutResult.Location));
    if (!bUsingRaisedCamera && !bNeedsRaisedCamera) return;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(RaisedCombatCamera), false, this);
    if (Boss) Params.AddIgnoredActor(Boss);
    if (!bNeedsRaisedCamera)
    {
        CameraClearElapsed += FMath::Max(0.f, DeltaTime);
        const float Progress = FMath::Clamp((CameraClearElapsed - CameraClearDelay)
            / FMath::Max(0.01f, CameraReturnDuration), 0.f, 1.f);
        const float Blend = Progress * Progress * (3.f - 2.f * Progress);
        const FVector Candidate = FMath::Lerp(PlayerLocation + RaisedCameraOffset, OutResult.Location, Blend);
        FHitResult ReturnHit;
        const bool bWallBlocksReturn = World->SweepSingleByChannel(ReturnHit, Pivot, Candidate,
            FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(12.f), Params);
        FCollisionQueryParams SightParams(SCENE_QUERY_STAT(ReturnCameraSight), false, this);
        const bool bBossBlocksReturn = Boss && (
            Boss->GetComponentsBoundingBox(true).ExpandBy(12.f).IsInside(Candidate)
            || (World->LineTraceSingleByChannel(ReturnHit, Pivot, Candidate, ECC_Visibility, SightParams)
                && ReturnHit.GetActor() == Boss));
        if (!bWallBlocksReturn && !bBossBlocksReturn)
        {
            // Blend look-at points near the character along with position. A
            // quaternion-only blend can look away from the player halfway back.
            const FVector NormalFocus = OutResult.Location + OutResult.Rotation.Vector()
                * FVector::Dist(OutResult.Location, Pivot);
            const FVector Focus = FMath::Lerp(PlayerLocation + RaisedCameraFocusOffset, NormalFocus, Blend);
            OutResult.Location = Candidate;
            OutResult.Rotation = (Focus - Candidate).Rotation();
            if (Progress >= 1.f)
            {
                bUsingRaisedCamera = false;
                bFrameBossWithCamera = false;
                CameraClearElapsed = 0.f;
            }
            return;
        }
        // A clear destination alone is insufficient: the blended view must also
        // clear walls and the boss. Retreat immediately if that path is blocked.
    }

    bUsingRaisedCamera = true;
    CameraClearElapsed = 0.f;
    bFrameBossWithCamera |= bBossObstructsView;

    // Keep the raised view's yaw aligned with mouse aim. Center the pair when a
    // boss occludes the view, instead of turning the camera toward a boss behind
    // the player (which would reverse the apparent movement/attack direction).
    // CalcCamera also runs after victory/defeat, when combat ticking has stopped.
    const float Yaw = Controller ? Controller->GetControlRotation().Yaw : GetActorRotation().Yaw;
    const FVector Forward = FRotator(0.f, Yaw, 0.f).Vector();
    FVector Focus = bFrameBossWithCamera && Boss
        ? (PlayerLocation + Boss->GetActorLocation()) * 0.5f
        : PlayerLocation + Forward * 100.f;
    const float Height = bFrameBossWithCamera ? FMath::Max(RaisedCameraHeight, BossCameraHeight) : RaisedCameraHeight;
    FVector RaisedLocation = FVector(Focus.X, Focus.Y, PlayerLocation.Z + Height) - Forward * 160.f;
    if (bFrameBossWithCamera && Boss && Boss->HasImportedVisuals())
    {
        // Preserve the colossus ground camera: looking down from above its back
        // hides a player standing under the body. Route climbing has its own view.
        FVector Away = (PlayerLocation - Boss->GetActorLocation()).GetSafeNormal2D();
        if (Away.IsNearlyZero()) Away = -Forward;
        RaisedLocation = PlayerLocation + Away * 700.f + FVector(0.f, 0.f, 280.f);
        Focus = PlayerLocation + FVector(0.f, 0.f, 40.f);
    }
    FHitResult Hit;
    const bool bBlocked = World->SweepSingleByChannel(Hit, Pivot, RaisedLocation,
        FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(12.f), Params);
    OutResult.Location = bBlocked ? Hit.Location : RaisedLocation;
    OutResult.Rotation = (Focus - OutResult.Location).Rotation();
    RaisedCameraOffset = OutResult.Location - PlayerLocation;
    RaisedCameraFocusOffset = Focus - PlayerLocation;
}

FVector APrototypePlayer::GetAttackIndicatorDirection() const
{
    // Turning the camera during a swing must not turn its already committed trace.
    return IsAttacking() ? AttackDirection
        : FRotator(0.f, Controller ? Controller->GetControlRotation().Yaw : GetActorRotation().Yaw, 0.f).Vector();
}

void APrototypePlayer::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("MoveForward"), this, &APrototypePlayer::MoveForward);
    Input->BindAxis(TEXT("MoveRight"), this, &APrototypePlayer::MoveRight);
    Input->BindAxis(TEXT("Turn"), this, &APrototypePlayer::Turn);
    Input->BindAxis(TEXT("LookUp"), this, &APrototypePlayer::LookUp);
    Input->BindAxis(TEXT("TurnRate"), this, &APrototypePlayer::TurnRate);
    Input->BindAxis(TEXT("LookUpRate"), this, &APrototypePlayer::LookUpRate);
    Input->BindAction(TEXT("Dodge"), IE_Pressed, this, &APrototypePlayer::Dodge);
    Input->BindAction(TEXT("Attack"), IE_Pressed, this, &APrototypePlayer::Attack);
    Input->BindAction(TEXT("Jump"), IE_Pressed, this, &APrototypePlayer::TryJump);
    Input->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
    Input->BindAction(TEXT("Retry"), IE_Pressed, this, &APrototypePlayer::Retry);
    Input->BindAction(TEXT("Grab"), IE_Pressed, this, &APrototypePlayer::BeginGrab);
    Input->BindAction(TEXT("Grab"), IE_Released, this, &APrototypePlayer::ReleaseGrab);
}

bool APrototypePlayer::CanAct() const
{
    const APrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    return Health > 0 && Mode && Mode->IsEncounterActive();
}

void APrototypePlayer::MoveForward(float Value)
{
    ForwardInput = Value;
    Climbing->SetInput(ForwardInput,RightInput);
    if (CanAct() && Controller && !IsDodging() && !IsGrabbing())
        AddMovementInput(FRotationMatrix(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::X), Value);
}

void APrototypePlayer::MoveRight(float Value)
{
    RightInput = Value;
    Climbing->SetInput(ForwardInput,RightInput);
    if (CanAct() && Controller && !IsDodging() && !IsGrabbing())
        AddMovementInput(FRotationMatrix(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::Y), Value);
}

void APrototypePlayer::Turn(float Value) { AddControllerYawInput(Value); }
void APrototypePlayer::LookUp(float Value) { AddControllerPitchInput(Value); }
void APrototypePlayer::TryJump()
{
    if (!CanAct()) return;
    if (Climbing->IsClimbing()) { Climbing->Detach(); return; }
    if (!IsDodging() && !IsAttacking() && !IsGrabbing()) Jump();
}
void APrototypePlayer::TurnRate(float Value) { AddControllerYawInput(Value * GamepadCameraYawSpeed * GetWorld()->GetDeltaSeconds()); }
void APrototypePlayer::LookUpRate(float Value) { AddControllerPitchInput(Value * GamepadCameraPitchSpeed * GetWorld()->GetDeltaSeconds()); }
bool APrototypePlayer::IsGrabbing() const { return Climbing->IsClimbing() || GrabComponent->IsGrabbing(); }
void APrototypePlayer::BeginGrab()
{
    if (!CanAct()) return;
    if (Climbing->IsClimbing()) { Climbing->GrabPressed(); return; }
    if (GrabComponent->IsGrabbing() || IsDodging()) return;
    if (bUseRouteClimbing) { Climbing->GrabPressed(); return; }
    const APrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (Mode && GrabComponent->TryGrab(Mode->GetBoss(), GrabDistance)) ShowFeedback(TEXT("GRABBING"));
}
void APrototypePlayer::ReleaseGrab()
{
    Climbing->GrabReleased();
    if (GrabComponent->IsGrabbing()) { GrabComponent->Release(); ShowFeedback(TEXT("RELEASED")); }
}

void APrototypePlayer::Dodge()
{
    if (!CanAct() || IsGrabbing() || IsDodging() || DodgeCooldownRemaining > 0.f || GetCharacterMovement()->IsFalling()) return;
    const FRotator Yaw(0.f, Controller ? Controller->GetControlRotation().Yaw : GetActorRotation().Yaw, 0.f);
    DodgeDirection = (FRotationMatrix(Yaw).GetUnitAxis(EAxis::X) * ForwardInput
        + FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y) * RightInput).GetSafeNormal();
    if (DodgeDirection.IsNearlyZero()) DodgeDirection = GetActorForwardVector();
    // A dodge can cancel a swing, but keeps its attack cooldown.
    AttackRemaining = 0.f;
    DodgeRemaining = DodgeDuration;
    DodgeCooldownRemaining = DodgeCooldown;
    SetActorRotation(DodgeDirection.Rotation());
    GetCharacterMovement()->ClearAccumulatedForces();
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();
    ShowFeedback(TEXT("DODGE - invulnerable"));
}

void APrototypePlayer::Attack()
{
    if (!CanAct() || IsDodging() || GrabComponent->IsGrabbing() || AttackCooldownRemaining > 0.f) return;
    if (Climbing->IsClimbing())
    {
        if (!Climbing->IsResting()) return;
        AttackRemaining = AttackDuration; AttackCooldownRemaining = AttackCooldown;
        ShowFeedback(Climbing->TryPurify() ? TEXT("PURIFIED - corruption removed") : TEXT("No unpurified core in reach"));
        CurrentAnimation = INDEX_NONE;
        return;
    }
    AttackDirection = FRotator(0.f, Controller ? Controller->GetControlRotation().Yaw : GetActorRotation().Yaw, 0.f).Vector();
    SetActorRotation(AttackDirection.Rotation());
    AttackRemaining = AttackDuration;
    AttackCooldownRemaining = AttackCooldown;
    bAttackConnected = false;
    ShowFeedback(TEXT("SLASH"));
    TraceAttack();
}

void APrototypePlayer::TraceAttack()
{
    const FVector Start = GetActorLocation();
    const FVector End = Start + AttackDirection * AttackReach;
    const FColor Color = bAttackConnected ? FColor::Green : FColor::Cyan;
    DrawDebugCapsule(GetWorld(), (Start + End) * 0.5f, AttackReach * 0.5f + AttackRadius,
        AttackRadius, FQuat::FindBetweenNormals(FVector::UpVector, AttackDirection), Color, false, -1.f, 0, 2.f);
    if (bAttackConnected) return;

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(PlayerSword), false, this);
    if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility,
        FCollisionShape::MakeSphere(AttackRadius), Params))
    {
        if (AIshibashiriBoss* Boss = Cast<AIshibashiriBoss>(Hit.GetActor()))
        {
            bAttackConnected = true;
            ShowFeedback(Boss->TryReceiveCounter() ? TEXT("COUNTER! Boss HP -1") : TEXT("DEFLECTED - wait for a new recovery"));
        }
    }
}

bool APrototypePlayer::ReceiveChargeHit(const FVector& From)
{
    if (!CanAct() || IsInvulnerable()) return false;
    Climbing->Detach(false);
    ReleaseGrab();
    Health = FMath::Max(0, Health - 1);
    HurtInvulnerabilityRemaining = HurtInvulnerabilityDuration;
    AttackRemaining = 0.f;
    ShowFeedback(TEXT("HIT! Player HP -1"));
    if (Health == 0)
    {
        if (APrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>()) Mode->FinishEncounter(false);
    }
    else
    {
        const FVector Away = (GetActorLocation() - From).GetSafeNormal2D();
        LaunchCharacter(Away * 300.f + FVector(0.f, 0.f, 100.f), true, true);
    }
    return true;
}

void APrototypePlayer::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateAnimation();
    if (!CanAct()) return;
    DodgeCooldownRemaining = FMath::Max(0.f, DodgeCooldownRemaining - DeltaSeconds);
    AttackCooldownRemaining = FMath::Max(0.f, AttackCooldownRemaining - DeltaSeconds);
    HurtInvulnerabilityRemaining = FMath::Max(0.f, HurtInvulnerabilityRemaining - DeltaSeconds);
    FeedbackRemaining = FMath::Max(0.f, FeedbackRemaining - DeltaSeconds);
    if (FeedbackRemaining == 0.f) Feedback.Empty();
    if (GrabComponent->IsGrabbing()) GrabComponent->Climb(ForwardInput, RightInput, DeltaSeconds);

    if (IsDodging())
    {
        // Sweep, and clamp the last step: distance stays 420 cm regardless of frame rate.
        const float Step = FMath::Min(DeltaSeconds, DodgeRemaining);
        FHitResult Hit;
        SetActorLocation(GetActorLocation() + DodgeDirection * DodgeSpeed * Step, true, &Hit);
        DodgeRemaining = FMath::Max(0.f, DodgeRemaining - DeltaSeconds);
        if (!IsDodging()) GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }
    if (IsAttacking())
    {
        if (!Climbing->IsClimbing()) TraceAttack();
        AttackRemaining = FMath::Max(0.f, AttackRemaining - DeltaSeconds);
    }
    const float Swing = IsAttacking() ? FMath::Lerp(-65.f, 65.f, 1.f - AttackRemaining / AttackDuration) : 0.f;
    Sword->SetRelativeRotation(FRotator(0.f, Swing, 0.f));
    GetCharacterMovement()->bOrientRotationToMovement = !IsAttacking() && !IsDodging() && !IsGrabbing();
    if (BodyMaterial)
    {
        const bool bFlash = HurtInvulnerabilityRemaining > 0.f && FMath::Sin(GetWorld()->GetTimeSeconds() * 35.f) > 0.f;
        SetPrimitiveColor(BodyMaterial, IsDodging() ? FLinearColor::White
            : (bFlash ? FLinearColor(1.f, 0.15f, 0.1f) : FLinearColor(0.08f, 0.5f, 0.8f)));
    }
}

void APrototypePlayer::ShowFeedback(const FString& Text)
{
    Feedback = Text;
    FeedbackRemaining = 1.1f;
}

void APrototypePlayer::StopCombat()
{
    if (Climbing) Climbing->Detach(false);
    if (GrabComponent) GrabComponent->Release();
    StopJumping();
    DodgeRemaining = AttackRemaining = 0.f;
    // LaunchCharacter queues knockback for the next movement tick. Zeroing current
    // velocity alone would allow a hit immediately before Retry to move the new pawn state.
    GetCharacterMovement()->ClearAccumulatedForces();
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();
}

void APrototypePlayer::ResetForEncounter(const FTransform& Spawn)
{
    StopCombat();
    Climbing->Reset(); CurrentAnimation = INDEX_NONE;
    Health = MaxHealth;
    DodgeCooldownRemaining = AttackCooldownRemaining = HurtInvulnerabilityRemaining = 0.f;
    ForwardInput = RightInput = FeedbackRemaining = 0.f;
    bAttackConnected = false;
    Feedback.Empty();
    bUsingRaisedCamera = false;
    bFrameBossWithCamera = false;
    CameraClearElapsed = 0.f;
    RaisedCameraOffset = FVector::ZeroVector;
    RaisedCameraFocusOffset = FVector::ZeroVector;
    ConsumeMovementInputVector();
    SetActorTransform(Spawn, false, nullptr, ETeleportType::TeleportPhysics);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    GetCharacterMovement()->bOrientRotationToMovement = true;
    Sword->SetRelativeRotation(FRotator::ZeroRotator);
    if (Controller) Controller->SetControlRotation(FRotator(-18.f, Spawn.Rotator().Yaw, 0.f));
    SetPrimitiveColor(BodyMaterial, FLinearColor(0.08f, 0.5f, 0.8f));
}

void APrototypePlayer::UpdateAnimation()
{
    const bool Climb = Climbing && Climbing->IsClimbing();
    const bool HideWeapon = IsGrabbing() && !IsAttacking();
    if (HideWeapon != bWeaponHidden)
    {
        bWeaponHidden = HideWeapon;
        if (HideWeapon) GetMesh()->HideBoneByName(TEXT("weapon"),EPhysBodyOp::PBO_None);
        else GetMesh()->UnHideBoneByName(TEXT("weapon"));
    }
    int32 Next = Health <= 0 ? 9 : IsDodging() ? 4 : IsAttacking() ? 3
        : Climb ? (Climbing->GetBoss()->IsBucking() ? 7 : Climbing->IsMoving() ? 5 : Climbing->IsResting() ? 0 : 6)
        : GrabComponent->IsGrabbing() ? (FMath::Abs(ForwardInput)+FMath::Abs(RightInput) > .01f ? 5 : 6)
        : GetCharacterMovement()->IsFalling() ? 8 : GetVelocity().Size2D()>300.f ? 2 : GetVelocity().Size2D()>5.f ? 1 : 0;
    if (Next != CurrentAnimation && Animations.IsValidIndex(Next) && Animations[Next])
    {
        CurrentAnimation = Next;
        GetMesh()->PlayAnimation(Animations[Next],Next != 3 && Next != 4 && Next != 9);
        GetMesh()->SetPlayRate(Next == 4 ? .6f/DodgeDuration : 1.f);
    }
}

void APrototypePlayer::Retry()
{
    if (APrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>()) Mode->RetryEncounter();
}

bool APrototypePlayer::HasImportedVisuals() const
{
    if (!GetMesh()->GetSkeletalMeshAsset() || Body->IsVisible() || Sword->IsVisible() || Animations.Num() != 10) return false;
    for (const UAnimSequence* Clip : Animations) if (!Clip) return false;
    return GetMesh()->GetMaterial(0) != nullptr;
}
