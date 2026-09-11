#include "IshibashiriBoss.h"
#include "PrototypeGameMode.h"
#include "PrototypePlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PrimitiveAppearance.h"
#include "ColossusClimbingComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    // Coordinates in the authored mesh (centimeters, -Y forward); locations are
    // transformed through the animated creature component each frame.
    const FVector Route[] = {
        {-270,-215,65}, {-292,-210,200}, {-288,-202,340},
        {-213,-198,485}, {-178,-100,591}, {-95,-48,685},
        {0,55,770}, {36,218,714}, {-122,305,668},
        {218,-155,593}, {140,-60,666}
    };
    const int32 Neighbors[][4] = {
        {1,-1,-1,-1}, {2,0,-1,-1}, {3,1,-1,-1}, {4,2,-1,-1},
        {5,3,-1,-1}, {6,4,10,-1}, {7,5,-1,-1}, {8,6,-1,-1},
        {-1,7,-1,-1}, {-1,10,-1,-1}, {9,5,9,5}
    };
    const FVector Cores[] = {{218,-155,611}, {0,60,784}, {-122,305,688}};
}

AIshibashiriBoss::AIshibashiriBoss()
{
    PrimaryActorTick.bCanEverTick = true;
    // Character movement finishes before the boss checks charge contact.
    PrimaryActorTick.TickGroup = TG_PostPhysics;
    Collision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Collision"));
    SetRootComponent(Collision);
    Collision->InitCapsuleSize(310.f, 350.f);
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
    // The original primitive parts remain as an asset-load fallback only.
    Creature = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RiggedIshibashiri"));
    Creature->SetupAttachment(RootComponent);
    Creature->SetRelativeLocation(FVector(0,0,-350));
    Creature->SetRelativeRotation(FRotator(0,90,0));
    Creature->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Creature->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    Creature->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Rigged(TEXT("/Game/Characters/Rigged/Ishibashiri/SK_Ishibashiri"));
    Creature->SetSkeletalMesh(Rigged.Object);
    const TCHAR* Clips[] = {TEXT("Idle"),TEXT("Walk"),TEXT("Charge"),TEXT("Buck"),TEXT("Calmed")};
    for (const TCHAR* Clip : Clips)
        CreatureAnimations.Add(LoadObject<UAnimSequence>(nullptr,*FString::Printf(TEXT("/Game/Characters/Rigged/Ishibashiri/AN_Ishibashiri_%s"),Clip)));
    if (Rigged.Succeeded())
    {
        TArray<UStaticMeshComponent*> Parts; GetComponents(Parts);
        for (UStaticMeshComponent* OldPart : Parts) OldPart->SetVisibility(false);
    }
    for (int32 I=0; I<3; ++I)
    {
        UStaticMeshComponent* Core = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("CoreMarker%d"),I));
        Core->SetupAttachment(Creature); Core->SetRelativeLocation(Cores[I]+FVector(0,0,40));
        Core->SetRelativeScale3D(FVector(.35)); Core->SetStaticMesh(Sphere.Object);
        Core->SetMaterial(0,Material.Object); Core->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        CoreMarkers.Add(Core);
    }
    GrabMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ForelegGrabMarker"));
    GrabMarker->SetupAttachment(Creature); GrabMarker->SetRelativeLocation(Route[0]+FVector(0,0,90));
    GrabMarker->SetRelativeScale3D(FVector(.30)); GrabMarker->SetStaticMesh(Sphere.Object);
    GrabMarker->SetMaterial(0,Material.Object); GrabMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    for (int32 I=3; I<UE_ARRAY_COUNT(Route); ++I)
    {
        UBoxComponent* Ledge = CreateDefaultSubobject<UBoxComponent>(*FString::Printf(TEXT("ClimbLedge%d"),I));
        Ledge->SetupAttachment(Creature); Ledge->SetRelativeLocation(Route[I]-FVector(0,0,18));
        Ledge->SetBoxExtent(FVector(65,65,18));
        Ledge->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Ledge->SetCollisionObjectType(ECC_WorldDynamic);
        Ledge->SetCollisionResponseToAllChannels(ECR_Ignore);
        Ledge->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);
        Ledge->SetCanEverAffectNavigation(false); LedgeCollision.Add(Ledge);
    }
}

void AIshibashiriBoss::BeginPlay()
{
    Super::BeginPlay();
    Health = MaxHealth;
    BodyMaterial = Body->CreateDynamicMaterialInstance(0);
    if (BodyMaterial)
        for (UStaticMeshComponent* Part : ColoredParts) Part->SetMaterial(0, BodyMaterial);
    EnterState(EIshibashiriState::Chase);
    for (UStaticMeshComponent* Core : CoreMarkers)
        SetPrimitiveColor(Core->CreateDynamicMaterialInstance(0), FLinearColor(1,.015,.005));
    SetPrimitiveColor(GrabMarker->CreateDynamicMaterialInstance(0), FLinearColor(1,.68,.05));
    UpdateCreatureAnimation();
}

void AIshibashiriBoss::ResetForEncounter(const FTransform& Spawn, APrototypePlayer* Player)
{
    if (Target) RemoveTickPrerequisiteComponent(Target->GetCharacterMovement());
    Target = Player;
    if (Target) AddTickPrerequisiteComponent(Target->GetCharacterMovement());
    Health = MaxHealth;
    bCounterUsed = bChargeHitPlayer = false;
    VisualTime = 0.f;
    RiderTime = 0.f; AnimationIndex = INDEX_NONE;
    for (int32 I=0; I<3; ++I)
    {
        PurifiedCores[I] = false; CoreMarkers[I]->SetVisibility(true);
        Creature->UnHideBoneByName(*FString::Printf(TEXT("core_%d"),I));
    }
    Creature->SetRelativeRotation(FRotator(0,90,0));
    ChargeDirection = Spawn.GetRotation().GetForwardVector();
    SetActorTransform(Spawn, false, nullptr, ETeleportType::TeleportPhysics);
    EnterState(EIshibashiriState::Chase);
    if (Target) AddTickPrerequisiteComponent(Target->GetCharacterMovement());
    UpdateVisuals();
    UpdateCreatureAnimation();
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
    if (Target->IsGrabbing() && !Target->GetClimbing()->IsGrabWarping())
    {
        RiderTime = Target->GetClimbing()->IsClimbing() ? RiderTime + DeltaSeconds : 0.f;
        // Continue moving under the rider. Turn inward before the full model
        // reaches the arena edge, instead of aiming at the rider on our back.
        if (GetActorLocation().Size2D() > Mode->ArenaHalfExtent-900.f)
            SetActorRotation((-GetActorLocation()).GetSafeNormal2D().Rotation());
        FHitResult Hit;
        SetActorLocation(GetActorLocation()+GetActorForwardVector()*85.f*DeltaSeconds,true,&Hit);
        if (Hit.bBlockingHit) AddActorWorldRotation(FRotator(0,90,0));
        Creature->SetRelativeRotation(FRotator(IsBucking() ? FMath::Sin(RiderTime*19.f)*4.f : 0.f,90,0));
        UpdateCreatureAnimation();
        return;
    }
    RiderTime = 0.f;
    Creature->SetRelativeRotation(FRotator(0,90,0));
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
    UpdateCreatureAnimation();
}

void AIshibashiriBoss::CheckChargeHit(const FVector& Start, const FVector& End)
{
    if (bChargeHitPlayer || !Target || Target->GetClimbing()->IsClimbing()) return;
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
    UpdateCreatureAnimation();
    return true;
}

void AIshibashiriBoss::UpdateVisuals()
{
    FLinearColor Color(0.27f, 0.16f, 0.1f);
    if (State == EIshibashiriState::Telegraph)
    {
        Color = FMath::Sin(VisualTime * 32.f) > 0.f ? FLinearColor(1.f, 0.025f, 0.01f) : FLinearColor(0.35f, 0.01f, 0.01f);
        const FVector Start = GetActorLocation() - FVector(0.f, 0.f, 343.f);
        DrawDebugDirectionalArrow(GetWorld(), Start, Start + GetActorForwardVector() * ChargeSpeed * MaxChargeDuration,
            140.f, FColor::Red, false, -1.f, 0, 8.f);
    }
    else if (State == EIshibashiriState::Charge) Color = FLinearColor(0.8f, 0.08f, 0.015f);
    else if (CanBeCountered()) Color = FLinearColor(0.12f, 0.85f, 0.28f);
    else if (State == EIshibashiriState::Recover) Color = FLinearColor(0.8f, 0.5f, 0.08f);
    else if (State == EIshibashiriState::Calmed) Color = FLinearColor(0.3f, 0.6f, 0.9f);
    SetPrimitiveColor(BodyMaterial, Color);
}

FString AIshibashiriBoss::GetStateLabel() const
{
    if (IsBucking()) return TEXT("SHAKING - HOLD E!");
    if (IsBuckWarning()) return TEXT("SHAKE INCOMING - HOLD E!");
    if (Target && Target->GetClimbing()->IsClimbing()) return TEXT("CARRYING RIDER - purify the three red cores");
    if (Target && Target->IsGrabbing()) return TEXT("CARRYING RIDER - local climbing");
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

FVector AIshibashiriBoss::GetClimbPosition(int32 Node) const
{
    if (Node < 0 || Node >= UE_ARRAY_COUNT(Route)) return GetActorLocation();
    // Add the character capsule half-height in world space, not the tilted
    // mesh frame, so the rider stays upright when the creature shakes.
    return Creature->GetComponentTransform().TransformPosition(Route[Node])+FVector(0,0,88);
}
FTransform AIshibashiriBoss::GetClimbFrame() const
{
    return Creature->GetComponentTransform();
}
int32 AIshibashiriBoss::GetClimbNeighbor(int32 Node, int32 Direction) const
{
    return Node >= 0 && Node < UE_ARRAY_COUNT(Route) && Direction >= 0 && Direction < 4 ? Neighbors[Node][Direction] : INDEX_NONE;
}
bool AIshibashiriBoss::IsBucking() const { return RiderTime > 0.f && FMath::Fmod(RiderTime,12.f) >= 10.f; }
bool AIshibashiriBoss::IsBuckWarning() const
{
    const float Phase = FMath::Fmod(RiderTime,12.f); return RiderTime > 0.f && Phase >= 8.f && Phase < 10.f;
}
int32 AIshibashiriBoss::GetPurifiedCount() const
{
    return int32(PurifiedCores[0])+int32(PurifiedCores[1])+int32(PurifiedCores[2]);
}
bool AIshibashiriBoss::TryPurifyCore(const FVector& Position)
{
    APrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (!Mode || !Mode->IsEncounterActive() || !Target || !Target->GetClimbing()->IsResting() || IsBucking()) return false;
    for (int32 I=0; I<3; ++I)
    {
        const FVector WorldCore = Creature->GetComponentTransform().TransformPosition(Cores[I]);
        if (!PurifiedCores[I] && FVector::Dist(Position,WorldCore) < 170.f)
        {
            PurifiedCores[I] = true; CoreMarkers[I]->SetVisibility(false);
            Creature->HideBoneByName(*FString::Printf(TEXT("core_%d"),I),EPhysBodyOp::PBO_None);
            Health = FMath::Max(0,Health-1);
            if (Health == 0) { EnterState(EIshibashiriState::Calmed); Mode->FinishEncounter(true); }
            UpdateCreatureAnimation();
            return true;
        }
    }
    return false;
}
void AIshibashiriBoss::UpdateCreatureAnimation()
{
    const bool Rider = Target && Target->IsGrabbing();
    int32 Next = State == EIshibashiriState::Calmed ? 4 : IsBucking() ? 3
        : Rider ? 1 : State == EIshibashiriState::Charge ? 2 : State == EIshibashiriState::Chase ? 1 : 0;
    if (Next != AnimationIndex && CreatureAnimations.IsValidIndex(Next) && CreatureAnimations[Next])
    {
        AnimationIndex = Next; Creature->PlayAnimation(CreatureAnimations[Next],Next != 4);
    }
}

bool AIshibashiriBoss::HasImportedVisuals() const
{
    if (!Creature->GetSkeletalMeshAsset() || Body->IsVisible() || CreatureAnimations.Num() != 5) return false;
    for (const UAnimSequence* Clip : CreatureAnimations) if (!Clip) return false;
    return Creature->GetMaterial(0) != nullptr;
}
