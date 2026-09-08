#include "PrototypePlayer.h"
#include "IshibashiriBoss.h"
#include "PrototypeGameMode.h"
#include "PrimitiveAppearance.h"
#include "ModelAppearance.h"
#include "GrabComponent.h"
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
    GrabComponent = CreateDefaultSubobject<UGrabComponent>(TEXT("GrabComponent"));

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

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Warrior(TEXT("/Game/Characters/FreeModels/Warrior/SK_Warrior.SK_Warrior"));
    GetMesh()->SetSkeletalMesh(Warrior.Object);
    GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
    GetMesh()->SetRelativeScale3D(FVector(0.62f));
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Body->SetVisibility(!Warrior.Succeeded());
    Sword->SetVisibility(!Warrior.Succeeded());
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Idle(TEXT("/Game/Characters/FreeModels/Warrior/AN_Warrior_Idle.AN_Warrior_Idle"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Run(TEXT("/Game/Characters/FreeModels/Warrior/AN_Warrior_Run.AN_Warrior_Run"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> AttackClip(TEXT("/Game/Characters/FreeModels/Warrior/AN_Warrior_Attack.AN_Warrior_Attack"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> DodgeClip(TEXT("/Game/Characters/FreeModels/Warrior/AN_Warrior_Dodge.AN_Warrior_Dodge"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Hit(TEXT("/Game/Characters/FreeModels/Warrior/AN_Warrior_Hit.AN_Warrior_Hit"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Death(TEXT("/Game/Characters/FreeModels/Warrior/AN_Warrior_Death.AN_Warrior_Death"));
    IdleAnimation = Idle.Object;
    RunAnimation = Run.Object;
    AttackAnimation = AttackClip.Object;
    DodgeAnimation = DodgeClip.Object;
    HitAnimation = Hit.Object;
    DeathAnimation = Death.Object;
}

void APrototypePlayer::BeginPlay()
{
    Super::BeginPlay();
    Health = MaxHealth;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    BodyMaterial = Body->CreateDynamicMaterialInstance(0);
    SetPrimitiveColor(BodyMaterial, FLinearColor(0.08f, 0.5f, 0.8f));
    for (int32 Index = 0; Index < GetMesh()->GetNumMaterials(); ++Index)
        if (UMaterialInstanceDynamic* Material = GetMesh()->CreateDynamicMaterialInstance(Index)) ModelMaterials.Add(Material);
    UpdateModelVisuals();
}

void APrototypePlayer::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
    Super::CalcCamera(DeltaTime, OutResult);
    const FVector PlayerLocation = GetActorLocation();
    const APrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    const AIshibashiriBoss* Boss = Mode ? Mode->GetBoss() : nullptr;
    // Check the regular spring-arm view, even while using the raised view. A small
    // hysteresis prevents rapid switching at the edge of a wall or the boar mesh.
    const float SafeDistance = MinimumCameraDistance + (bUsingRaisedCamera ? 60.f : 0.f);
    const float BossMargin = bUsingRaisedCamera ? 120.f : 70.f;
    bUsingRaisedCamera = FVector::Dist(PlayerLocation, OutResult.Location) < SafeDistance
        || (Boss && Boss->GetComponentsBoundingBox(true).ExpandBy(BossMargin).IsInside(OutResult.Location));
    if (!bUsingRaisedCamera) return;

    // The arena has open sky. Lifting straight up keeps the camera inside its
    // walls and above both characters, including when a charge overlaps them.
    // CalcCamera also runs after victory/defeat, when combat ticking has stopped.
    const FVector Pivot = PlayerLocation + FVector(0.f, 0.f, 90.f);
    const FVector RaisedLocation = PlayerLocation + FVector(0.f, 0.f, RaisedCameraHeight);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RaisedCombatCamera), false, this);
    if (Boss) Params.AddIgnoredActor(Boss);
    FHitResult Hit;
    const bool bBlocked = GetWorld()->SweepSingleByChannel(Hit, Pivot, RaisedLocation,
        FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(12.f), Params);
    OutResult.Location = bBlocked ? Hit.Location : RaisedLocation;
    const float Yaw = Controller ? Controller->GetControlRotation().Yaw : GetActorRotation().Yaw;
    const FVector Focus = PlayerLocation + FRotator(0.f, Yaw, 0.f).Vector() * 250.f;
    OutResult.Rotation = (Focus - OutResult.Location).Rotation();
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
    if (CanAct() && Controller && !IsDodging() && !IsGrabbing())
        AddMovementInput(FRotationMatrix(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::X), Value);
}

void APrototypePlayer::MoveRight(float Value)
{
    RightInput = Value;
    if (CanAct() && Controller && !IsDodging() && !IsGrabbing())
        AddMovementInput(FRotationMatrix(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::Y), Value);
}

void APrototypePlayer::Turn(float Value) { AddControllerYawInput(Value); }
void APrototypePlayer::LookUp(float Value) { AddControllerPitchInput(Value); }
void APrototypePlayer::TurnRate(float Value) { AddControllerYawInput(Value * GamepadCameraYawSpeed * GetWorld()->GetDeltaSeconds()); }
void APrototypePlayer::LookUpRate(float Value) { AddControllerPitchInput(Value * GamepadCameraPitchSpeed * GetWorld()->GetDeltaSeconds()); }
void APrototypePlayer::TryJump() { if (CanAct() && !IsDodging() && !IsAttacking() && !IsGrabbing()) Jump(); }

bool APrototypePlayer::IsGrabbing() const { return GrabComponent && GrabComponent->IsGrabbing(); }

void APrototypePlayer::BeginGrab()
{
    if (!CanAct() || IsDodging() || IsGrabbing()) return;
    const APrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (Mode && GrabComponent->TryGrab(Mode->GetBoss(), GrabDistance)) ShowFeedback(TEXT("GRABBING"));
}

void APrototypePlayer::ReleaseGrab()
{
    if (!IsGrabbing()) return;
    GrabComponent->Release();
    ShowFeedback(TEXT("RELEASED"));
}

void APrototypePlayer::Dodge()
{
    if (!CanAct() || IsDodging() || IsGrabbing() || DodgeCooldownRemaining > 0.f || GetCharacterMovement()->IsFalling()) return;
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
    if (!CanAct() || IsDodging() || IsGrabbing() || AttackCooldownRemaining > 0.f) return;
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
    if (!CanAct()) { UpdateModelVisuals(); return; }
    DodgeCooldownRemaining = FMath::Max(0.f, DodgeCooldownRemaining - DeltaSeconds);
    AttackCooldownRemaining = FMath::Max(0.f, AttackCooldownRemaining - DeltaSeconds);
    HurtInvulnerabilityRemaining = FMath::Max(0.f, HurtInvulnerabilityRemaining - DeltaSeconds);
    FeedbackRemaining = FMath::Max(0.f, FeedbackRemaining - DeltaSeconds);
    if (FeedbackRemaining == 0.f) Feedback.Empty();

    // Axis callbacks only record input. Applying both axes together here avoids
    // CharacterMovement and climbing receiving the same WASD input and keeps
    // diagonal climbing at the configured maximum speed.
    if (IsGrabbing()) GrabComponent->Climb(ForwardInput, RightInput, DeltaSeconds);

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
        TraceAttack();
        AttackRemaining = FMath::Max(0.f, AttackRemaining - DeltaSeconds);
    }
    const float Swing = IsAttacking() ? FMath::Lerp(-65.f, 65.f, 1.f - AttackRemaining / AttackDuration) : 0.f;
    Sword->SetRelativeRotation(FRotator(0.f, Swing, 0.f));
    GetCharacterMovement()->bOrientRotationToMovement = !IsAttacking() && !IsDodging();
    if (BodyMaterial)
    {
        const bool bFlash = HurtInvulnerabilityRemaining > 0.f && FMath::Sin(GetWorld()->GetTimeSeconds() * 35.f) > 0.f;
        SetPrimitiveColor(BodyMaterial, IsDodging() ? FLinearColor::White
            : (bFlash ? FLinearColor(1.f, 0.15f, 0.1f) : FLinearColor(0.08f, 0.5f, 0.8f)));
    }
    UpdateModelVisuals();
}

bool APrototypePlayer::HasImportedVisuals() const
{
    return GetMesh()->GetSkeletalMeshAsset() && IdleAnimation && RunAnimation && AttackAnimation
        && DodgeAnimation && HitAnimation && DeathAnimation && ModelMaterials.Num() == 2
        && !Body->IsVisible() && !Sword->IsVisible();
}

void APrototypePlayer::UpdateModelVisuals()
{
    if (Health <= 0) PlayModelClip(GetMesh(), DeathAnimation, false);
    else if (IsDodging()) PlayModelClip(GetMesh(), DodgeAnimation, false, DodgeAnimation ? DodgeAnimation->GetPlayLength() / DodgeDuration : 1.f);
    else if (IsAttacking()) PlayModelClip(GetMesh(), AttackAnimation, false, AttackAnimation ? AttackAnimation->GetPlayLength() / AttackDuration : 1.f);
    else if (HurtInvulnerabilityRemaining > HurtInvulnerabilityDuration - 0.25f)
        PlayModelClip(GetMesh(), HitAnimation, false, 2.f);
    else if (GetVelocity().SizeSquared2D() > 100.f)
        PlayModelClip(GetMesh(), RunAnimation, true, FMath::Clamp(GetVelocity().Size2D() / WalkSpeed, 0.5f, 1.4f));
    else PlayModelClip(GetMesh(), IdleAnimation, true);
    const bool bFlash = Health > 0 && HurtInvulnerabilityRemaining > 0.f && FMath::Sin(GetWorld()->GetTimeSeconds() * 35.f) > 0.f;
    const FLinearColor Tint = bFlash ? FLinearColor(2.f, 0.25f, 0.2f)
        : IsDodging() ? FLinearColor(1.6f, 1.8f, 2.f) : FLinearColor::White;
    for (UMaterialInstanceDynamic* Material : ModelMaterials) Material->SetVectorParameterValue(TEXT("Tint"), Tint);
}

void APrototypePlayer::ShowFeedback(const FString& Text)
{
    Feedback = Text;
    FeedbackRemaining = 1.1f;
}

void APrototypePlayer::StopCombat()
{
    ReleaseGrab();
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
    Health = MaxHealth;
    DodgeCooldownRemaining = AttackCooldownRemaining = HurtInvulnerabilityRemaining = 0.f;
    ForwardInput = RightInput = FeedbackRemaining = 0.f;
    bAttackConnected = false;
    Feedback.Empty();
    bUsingRaisedCamera = false;
    ConsumeMovementInputVector();
    SetActorTransform(Spawn, false, nullptr, ETeleportType::TeleportPhysics);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    GetCharacterMovement()->bOrientRotationToMovement = true;
    Sword->SetRelativeRotation(FRotator::ZeroRotator);
    if (Controller) Controller->SetControlRotation(FRotator(-18.f, Spawn.Rotator().Yaw, 0.f));
    SetPrimitiveColor(BodyMaterial, FLinearColor(0.08f, 0.5f, 0.8f));
    GetMesh()->PlayAnimation(IdleAnimation, true);
    UpdateModelVisuals();
}

void APrototypePlayer::Retry()
{
    if (APrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>()) Mode->RetryEncounter();
}
