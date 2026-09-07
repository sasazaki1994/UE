#include "IshibashiriBoss.h"
#include "PrototypeGameMode.h"
#include "PrototypePlayer.h"
#include "PrimitiveAppearance.h"
#include "ModelAppearance.h"
#include "Engine/SkeletalMesh.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AIshibashiriBoss::AIshibashiriBoss()
{
    PrimaryActorTick.bCanEverTick = true;
    // Character movement finishes before the boss checks charge contact.
    PrimaryActorTick.TickGroup = TG_PostPhysics;
    Collision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Collision"));
    SetRootComponent(Collision);
    Collision->InitCapsuleSize(180.f, 210.f);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    // Pawn contact is swept explicitly, so dodges never get trapped in the boss.
    Collision->SetGenerateOverlapEvents(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    auto Part = [this](const TCHAR* Name, UStaticMesh* Mesh, FVector Position, FVector Scale)
    {
        UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Component->SetupAttachment(RootComponent);
        Component->SetStaticMesh(Mesh);
        Component->SetRelativeLocation(Position);
        Component->SetRelativeScale3D(Scale);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetMaterial(0, Material.Object);
        return Component;
    };
    Body = Part(TEXT("Body"), Sphere.Object, FVector(0.f, 0.f, 10.f), FVector(4.3f, 3.3f, 3.2f));
    ColoredParts.Add(Body);
    ColoredParts.Add(Part(TEXT("Head"), Sphere.Object, FVector(140.f, 0.f, -15.f), FVector(2.f, 2.7f, 2.2f)));
    ColoredParts.Add(Part(TEXT("Snout"), Cube.Object, FVector(225.f, 0.f, -45.f), FVector(0.75f, 1.65f, 0.95f)));
    ColoredParts.Add(Part(TEXT("LeftEar"), Cone.Object, FVector(115.f, -90.f, 120.f), FVector(0.65f, 0.65f, 1.f)));
    ColoredParts.Add(Part(TEXT("RightEar"), Cone.Object, FVector(115.f, 90.f, 120.f), FVector(0.65f, 0.65f, 1.f)));
    ColoredParts.Add(Part(TEXT("FrontLeftLeg"), Cube.Object, FVector(110.f, -95.f, -140.f), FVector(0.65f, 0.65f, 1.3f)));
    ColoredParts.Add(Part(TEXT("FrontRightLeg"), Cube.Object, FVector(110.f, 95.f, -140.f), FVector(0.65f, 0.65f, 1.3f)));
    ColoredParts.Add(Part(TEXT("BackLeftLeg"), Cube.Object, FVector(-115.f, -95.f, -140.f), FVector(0.65f, 0.65f, 1.3f)));
    ColoredParts.Add(Part(TEXT("BackRightLeg"), Cube.Object, FVector(-115.f, 95.f, -140.f), FVector(0.65f, 0.65f, 1.3f)));
    Part(TEXT("LeftTusk"), Cone.Object, FVector(215.f, -115.f, -30.f), FVector(0.38f, 0.38f, 1.4f));
    Part(TEXT("RightTusk"), Cone.Object, FVector(215.f, 115.f, -30.f), FVector(0.38f, 0.38f, 1.4f));
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Boar(TEXT("/Game/Characters/FreeModels/Boar/SK_Boar.SK_Boar"));
    Model = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BoarModel"));
    Model->SetupAttachment(RootComponent);
    Model->SetSkeletalMesh(Boar.Object);
    Model->SetRelativeScale3D(FVector(1.6f));
    // The Boar faces +Y in Blender; the Warrior faces -Y.
    Model->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
    if (Boar.Succeeded())
    {
        // This asset's origin is in its torso, unlike the feet-origin Warrior.
        const FBoxSphereBounds Bounds = Boar.Object->GetBounds();
        Model->SetRelativeLocation(FVector(Bounds.Origin.X * 1.6f, 0.f,
            -210.f - (Bounds.Origin.Z - Bounds.BoxExtent.Z) * 1.6f));
    }
    Model->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Model->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Idle(TEXT("/Game/Characters/FreeModels/Boar/AN_Boar_Idle.AN_Boar_Idle"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Walk(TEXT("/Game/Characters/FreeModels/Boar/AN_Boar_Walk.AN_Boar_Walk"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> AttackClip(TEXT("/Game/Characters/FreeModels/Boar/AN_Boar_Attack.AN_Boar_Attack"));
    IdleAnimation = Idle.Object;
    WalkAnimation = Walk.Object;
    AttackAnimation = AttackClip.Object;
}

void AIshibashiriBoss::BeginPlay()
{
    Super::BeginPlay();
    Health = MaxHealth;
    BodyMaterial = Body->CreateDynamicMaterialInstance(0);
    if (BodyMaterial)
        for (UStaticMeshComponent* Part : ColoredParts) Part->SetMaterial(0, BodyMaterial);
    EnterState(EIshibashiriState::Chase);
    if (Model->GetSkeletalMeshAsset()) ModelMaterial = Model->CreateDynamicMaterialInstance(0);
    if (Model->GetSkeletalMeshAsset())
    {
        TArray<UStaticMeshComponent*> Primitives;
        GetComponents<UStaticMeshComponent>(Primitives);
        for (UStaticMeshComponent* Primitive : Primitives) Primitive->SetVisibility(false);
    }
}

void AIshibashiriBoss::ResetForEncounter(const FTransform& Spawn, APrototypePlayer* Player)
{
    Target = Player;
    Health = MaxHealth;
    bCounterUsed = bChargeHitPlayer = false;
    VisualTime = 0.f;
    ChargeDirection = Spawn.GetRotation().GetForwardVector();
    SetActorTransform(Spawn, false, nullptr, ETeleportType::TeleportPhysics);
    EnterState(EIshibashiriState::Chase);
    Model->PlayAnimation(WalkAnimation, true);
    if (Target) AddTickPrerequisiteComponent(Target->GetCharacterMovement());
    UpdateVisuals();
}

void AIshibashiriBoss::EnterState(EIshibashiriState NewState)
{
    State = NewState;
    switch (State)
    {
    case EIshibashiriState::Chase: StateTimeRemaining = ChaseDuration; break;
    case EIshibashiriState::Telegraph: StateTimeRemaining = TelegraphDuration; break;
    case EIshibashiriState::Charge:
        StateTimeRemaining = MaxChargeDuration;
        bChargeHitPlayer = false;
        // Capture the target only here; the entire charge uses this fixed direction.
        ChargeDirection = Target ? (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() : GetActorForwardVector();
        if (ChargeDirection.IsNearlyZero()) ChargeDirection = GetActorForwardVector();
        SetActorRotation(ChargeDirection.Rotation());
        break;
    case EIshibashiriState::Recover:
        StateTimeRemaining = RecoveryDuration;
        bCounterUsed = false;
        break;
    case EIshibashiriState::Calmed: StateTimeRemaining = 0.f; break;
    }
}

void AIshibashiriBoss::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const APrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (!Mode || !Mode->IsEncounterActive() || !IsValid(Target)) return;
    VisualTime += DeltaSeconds;
    // Bound movement steps and carry excess time into the next state at low frame rates.
    float Remaining = DeltaSeconds;
    while (Remaining > KINDA_SMALL_NUMBER && State != EIshibashiriState::Calmed)
    {
        float Step = FMath::Min(Remaining, 1.f / 60.f);
        if (StateTimeRemaining > 0.f) Step = FMath::Min(Step, StateTimeRemaining);
        Remaining -= Step;
        StateTimeRemaining = FMath::Max(0.f, StateTimeRemaining - Step);
        const FVector ToPlayer = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
        switch (State)
        {
        case EIshibashiriState::Chase:
        {
            if (!ToPlayer.IsNearlyZero()) SetActorRotation(ToPlayer.Rotation());
            const float Distance = FVector::Dist2D(Target->GetActorLocation(), GetActorLocation());
            if (Distance > 440.f)
            {
                FHitResult Hit;
                SetActorLocation(GetActorLocation() + ToPlayer * FMath::Min(ChaseSpeed * Step, Distance - 440.f), true, &Hit);
            }
            // The arena bounds distance; the timeout also avoids an endless chase at an obstruction.
            if (StateTimeRemaining <= 0.f && (Distance <= ChargeTriggerDistance || VisualTime >= ChaseDuration + 4.f))
                EnterState(EIshibashiriState::Telegraph);
            break;
        }
        case EIshibashiriState::Telegraph:
            if (!ToPlayer.IsNearlyZero()) SetActorRotation(ToPlayer.Rotation());
            if (StateTimeRemaining <= 0.f) EnterState(EIshibashiriState::Charge);
            break;
        case EIshibashiriState::Charge:
        {
            const FVector Start = GetActorLocation();
            FHitResult Hit;
            SetActorLocation(Start + ChargeDirection * ChargeSpeed * Step, true, &Hit);
            CheckChargeHit(Start, GetActorLocation());
            if (!Mode->IsEncounterActive()) { Remaining = 0.f; break; }
            if (Hit.bBlockingHit || StateTimeRemaining <= 0.f) EnterState(EIshibashiriState::Recover);
            break;
        }
        case EIshibashiriState::Recover:
            if (StateTimeRemaining <= 0.f) { VisualTime = 0.f; EnterState(EIshibashiriState::Chase); }
            break;
        default: Remaining = 0.f; break;
        }
    }
    UpdateVisuals();
}

void AIshibashiriBoss::CheckChargeHit(const FVector& Start, const FVector& End)
{
    if (bChargeHitPlayer || !Target) return;
    TArray<FHitResult> Hits;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(BossCharge), false, this);
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_Pawn);
    GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, Objects,
        FCollisionShape::MakeCapsule(Collision->GetScaledCapsuleRadius(), Collision->GetScaledCapsuleHalfHeight()), Params);
    for (const FHitResult& Hit : Hits)
    {
        if (Hit.GetActor() == Target && Target->ReceiveChargeHit(Start))
        {
            bChargeHitPlayer = true;
            break;
        }
    }
}

bool AIshibashiriBoss::TryReceiveCounter()
{
    APrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (!Mode || !Mode->IsEncounterActive() || !CanBeCountered()) return false;
    bCounterUsed = true;
    Health = FMath::Max(0, Health - 1);
    if (Health == 0)
    {
        EnterState(EIshibashiriState::Calmed);
        Mode->FinishEncounter(true);
    }
    UpdateVisuals();
    return true;
}

void AIshibashiriBoss::UpdateVisuals()
{
    FLinearColor Color(0.27f, 0.16f, 0.1f);
    if (State == EIshibashiriState::Telegraph)
    {
        Color = FMath::Sin(VisualTime * 32.f) > 0.f ? FLinearColor(1.f, 0.025f, 0.01f) : FLinearColor(0.35f, 0.01f, 0.01f);
        const FVector Start = GetActorLocation() - FVector(0.f, 0.f, 203.f);
        DrawDebugDirectionalArrow(GetWorld(), Start, Start + GetActorForwardVector() * ChargeSpeed * MaxChargeDuration,
            140.f, FColor::Red, false, -1.f, 0, 8.f);
    }
    else if (State == EIshibashiriState::Charge) Color = FLinearColor(0.8f, 0.08f, 0.015f);
    else if (CanBeCountered()) Color = FLinearColor(0.12f, 0.85f, 0.28f);
    else if (State == EIshibashiriState::Recover) Color = FLinearColor(0.8f, 0.5f, 0.08f);
    else if (State == EIshibashiriState::Calmed) Color = FLinearColor(0.3f, 0.6f, 0.9f);
    SetPrimitiveColor(BodyMaterial, Color);
    if (State == EIshibashiriState::Chase) PlayModelClip(Model, WalkAnimation, true, 1.5f);
    else if (State == EIshibashiriState::Charge) PlayModelClip(Model, WalkAnimation, true, 4.f);
    else if (State == EIshibashiriState::Telegraph)
        PlayModelClip(Model, AttackAnimation, false, AttackAnimation ? AttackAnimation->GetPlayLength() / TelegraphDuration : 1.f);
    else PlayModelClip(Model, IdleAnimation, true);
    const FLinearColor Tint = State == EIshibashiriState::Chase ? FLinearColor::White
        : State == EIshibashiriState::Telegraph ? FLinearColor(2.5f, 0.2f, 0.12f)
        : State == EIshibashiriState::Charge ? FLinearColor(1.8f, 0.5f, 0.3f)
        : CanBeCountered() ? FLinearColor(0.65f, 2.f, 0.8f)
        : State == EIshibashiriState::Calmed ? FLinearColor(0.65f, 1.2f, 1.8f) : FLinearColor(1.6f, 1.3f, 0.5f);
    if (ModelMaterial) ModelMaterial->SetVectorParameterValue(TEXT("Tint"), Tint);
}

bool AIshibashiriBoss::HasImportedVisuals() const
{
    return Model->GetSkeletalMeshAsset() && IdleAnimation && WalkAnimation && AttackAnimation
        && ModelMaterial && !Body->IsVisible();
}

FString AIshibashiriBoss::GetStateLabel() const
{
    switch (State)
    {
    case EIshibashiriState::Chase: return TEXT("CHASE");
    case EIshibashiriState::Telegraph: return TEXT("TELEGRAPH - dodge sideways at the charge!");
    case EIshibashiriState::Charge: return TEXT("CHARGE - direction locked");
    case EIshibashiriState::Recover: return bCounterUsed ? TEXT("RECOVERY - counter already used") : TEXT("RECOVERY - COUNTER NOW!");
    case EIshibashiriState::Calmed: return TEXT("CALMED");
    default: return TEXT("UNKNOWN");
    }
}
