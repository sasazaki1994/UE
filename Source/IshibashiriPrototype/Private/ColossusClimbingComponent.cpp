#include "ColossusClimbingComponent.h"
#include "PrototypePlayer.h"
#include "IshibashiriBoss.h"
#include "PrototypeGameMode.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

bool UColossusClimbingComponent::IsIKVerticalSlice() const
{
    return Boss && Node >= 0 && Node <= 3 && (Destination == INDEX_NONE || Destination <= 3);
}

void UColossusClimbingComponent::UpdateIK(float Dt)
{
    const float Desired = IsIKVerticalSlice() ? (IsResting() ? .72f : 1.f) : 0.f;
    const float Seconds = Desired > IKWeight ? IKBlendInSeconds : IKBlendOutSeconds;
    IKWeight = FMath::FInterpConstantTo(IKWeight, Desired, Dt, 1.f / FMath::Max(.01f, Seconds));
    if (!Boss) return;

    // Authored against the actual Shirotsura rig. Values remain in the creature
    // component's local frame, so walking, turning and buck rotation carry them.
    const FTransform Frame = Boss->GetClimbFrame();
    const FVector AnchorWorld = Destination == INDEX_NONE ? Boss->GetClimbPosition(Node)
        : FMath::Lerp(Boss->GetClimbPosition(Node), Boss->GetClimbPosition(Destination), Progress);
    const FVector Anchor = Frame.InverseTransformPosition(AnchorWorld - FVector(0, 0, 88));
    auto Target = [&Frame, &Anchor](const FVector& Offset, const FRotator& Rotation)
    {
        return FTransform(Frame.TransformRotation(Rotation.Quaternion()), Frame.TransformPosition(Anchor + Offset));
    };
    IKTargets.LeftHand  = Target(FVector(-18,-5, 72), FRotator(0,0,-12));
    IKTargets.RightHand = Target(FVector( 18,-5, 66), FRotator(0,0, 12));
    IKTargets.LeftFoot  = Target(FVector(-16,-2,-72), FRotator(0,0,-5));
    IKTargets.RightFoot = Target(FVector( 16,-2,-78), FRotator(0,0, 5));
}

UColossusClimbingComponent::UColossusClimbingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}
void UColossusClimbingComponent::BeginPlay()
{
    Super::BeginPlay();
    Player = Cast<APrototypePlayer>(GetOwner());
}
bool UColossusClimbingComponent::IsResting() const
{
    return Boss && Destination == INDEX_NONE && Boss->IsRestNode(Node);
}
void UColossusClimbingComponent::GrabPressed()
{
    bGripHeld = true;
    if (!Player || Boss || Stamina < 25.f || Player->IsDodging() || Player->IsAttacking()) return;
    APrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    AIshibashiriBoss* Candidate = Mode ? Mode->GetBoss() : nullptr;
    if (!Mode || !Mode->IsEncounterActive() || !Candidate) return;
    if (FVector::Dist(Player->GetActorLocation(), Candidate->GetClimbPosition(0)) > GrabRange) return;
    if (Candidate->GetState() == EIshibashiriState::Charge) return;
    Boss = Candidate;
    Node = 0; Destination = INDEX_NONE; Progress = 0.f; UnsafeBuckTime = 0.f;
    InputDelay = .15f;
    Player->GetCharacterMovement()->StopMovementImmediately();
    Player->GetCharacterMovement()->ClearAccumulatedForces();
    Player->GetCharacterMovement()->DisableMovement();
    Player->GetCharacterMovement()->bOrientRotationToMovement = false;
    Player->GetCapsuleComponent()->IgnoreActorWhenMoving(Boss, true);
    AddTickPrerequisiteActor(Boss);
    Player->SetActorLocation(Boss->GetClimbPosition(Node));
}
void UColossusClimbingComponent::Detach(bool bJump)
{
    if (!Boss || !Player) return;
    AIshibashiriBoss* OldBoss = Boss;
    const FVector Away = (Player->GetActorLocation() - Boss->GetActorLocation()).GetSafeNormal2D();
    RemoveTickPrerequisiteActor(Boss);
    Boss = nullptr; Node = Destination = INDEX_NONE; Progress = 0.f; bGripHeld = false;
    Player->GetCapsuleComponent()->IgnoreActorWhenMoving(OldBoss, false);
    Player->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
    Player->GetCharacterMovement()->bOrientRotationToMovement = true;
    if (bJump) Player->LaunchCharacter(Away * 650.f + FVector(0,0,380), true, true);
}
void UColossusClimbingComponent::Reset()
{
    Detach(false); Stamina = 100.f; ForwardInput = RightInput = InputDelay = UnsafeBuckTime = 0.f;
    bGripHeld = false; IKWeight = 0.f; IKTargets = FClimbingIKTargets();
}
bool UColossusClimbingComponent::TryPurify()
{
    return Boss && IsResting() && Boss->TryPurifyCore(Player->GetActorLocation());
}
void UColossusClimbingComponent::TickComponent(float Dt, ELevelTick Type, FActorComponentTickFunction* Tick)
{
    Super::TickComponent(Dt, Type, Tick);
    if (!Player) return;
    UpdateIK(Dt);
    const APrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (!Mode || !Mode->IsEncounterActive()) return;
    if (!IsValid(Boss))
    {
        if (Boss) Reset();
        if (!Player->GetCharacterMovement()->IsFalling()) Stamina = FMath::Min(100.f, Stamina+18.f*Dt);
        return;
    }
    const bool Buck = Boss->IsBucking();
    const bool Rest = IsResting();
    float Drain = Rest ? -22.f : 5.f;
    if (Buck) Drain = bGripHeld ? 18.f : 50.f;
    Stamina = FMath::Clamp(Stamina-Drain*Dt, 0.f, 100.f);
    UnsafeBuckTime = Buck && !bGripHeld ? UnsafeBuckTime+Dt : 0.f;
    if (Stamina <= 0.f || UnsafeBuckTime > .70f) { Detach(); return; }
    InputDelay = FMath::Max(0.f, InputDelay-Dt);
    if (Destination == INDEX_NONE && InputDelay <= 0.f && !Buck && !Player->IsAttacking())
    {
        int32 Next = INDEX_NONE;
        if (FMath::Abs(RightInput) > .5f) Next = Boss->GetClimbNeighbor(Node, RightInput > 0 ? 2 : 3);
        else if (FMath::Abs(ForwardInput) > .5f) Next = Boss->GetClimbNeighbor(Node, ForwardInput > 0 ? 0 : 1);
        if (Next != INDEX_NONE) { Destination = Next; Progress = 0.f; }
    }
    FVector Position = Boss->GetClimbPosition(Node);
    if (Destination != INDEX_NONE)
    {
        const FVector End = Boss->GetClimbPosition(Destination);
        if (!Buck) Progress = FMath::Min(1.f, Progress + Dt*ClimbSpeed/FMath::Max(1.f,FVector::Dist(Position,End)));
        Position = FMath::Lerp(Position,End,Progress);
        if (Progress >= 1.f)
        {
            Node = Destination; Destination = INDEX_NONE; InputDelay = .18f;
        }
    }
    FHitResult Hit;
    Player->SetActorLocation(Position,true,&Hit);
    if (Hit.bBlockingHit && Hit.GetActor() != Boss) { Detach(); return; }
    FVector Facing = Destination != INDEX_NONE ? Boss->GetClimbPosition(Destination)-Position
        : Boss->GetActorLocation()-Position;
    Facing.Z = 0;
    if (!Facing.IsNearlyZero()) Player->SetActorRotation(Facing.Rotation());
}
FString UColossusClimbingComponent::GetHint() const
{
    if (!Boss) return TEXT("E / RB near the gold foreleg hold: grab (wait until the charge stops)");
    if (Boss->IsBucking() || Boss->IsBuckWarning()) return TEXT("HOLD E / RB - brace for the shake! Movement pauses during the shake.");
    if (Node == 5) return TEXT("W / LS up: summit | D / LS right: right-shoulder core | S: descend | Space / A: detach");
    if (Node == 10) return TEXT("W: shoulder core | A / S: main route");
    return IsResting() ? TEXT("Ledge: stamina recovers | LMB / X: purify nearby red core | W/S: traverse | Space / A: detach")
        : TEXT("W/S: climb / descend | Hold E / RB during shaking | Space / A: detach");
}
