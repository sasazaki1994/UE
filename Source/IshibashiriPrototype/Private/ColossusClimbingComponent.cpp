#include "ColossusClimbingComponent.h"
#include "PrototypePlayer.h"
#include "IshibashiriBoss.h"
#include "PrototypeGameMode.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "MotionWarpingComponent.h"
#include "DrawDebugHelpers.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
const FName GrabWarpTargetName(TEXT("IshibashiriGrab"));
float FacingAngle(const APrototypePlayer* Player, const FTransform& Target)
{
    return FMath::Abs(FMath::FindDeltaAngleDegrees(Player->GetActorRotation().Yaw, Target.Rotator().Yaw));
}
}

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
    const FTransform WarpTarget = MakeGrabWarpTarget(Candidate);
    if (FVector::Dist(Player->GetActorLocation(), WarpTarget.GetLocation()) > MaximumWarpDistance
        || FacingAngle(Player, WarpTarget) > MaximumWarpAngle) return;
    if (StartGrabWarp(Candidate)) return;

    // Missing editor-authored assets retain the pre-warp gameplay path. This is
    // intentionally a visible legacy snap, not a claim that warping succeeded.
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
    UE_LOG(LogTemp, Warning, TEXT("GRAB_WARP_LEGACY_FALLBACK asset contract unavailable"));
}

void UColossusClimbingComponent::GrabReleased()
{
    bGripHeld = false;
}

FTransform UColossusClimbingComponent::MakeGrabWarpTarget(const AIshibashiriBoss* Candidate) const
{
    const FVector Location = Candidate->GetClimbPosition(0);
    FVector Facing = Candidate->GetActorLocation() - Location;
    Facing.Z = 0.f;
    const FRotator Rotation = Facing.IsNearlyZero() ? Candidate->GetActorRotation() : Facing.Rotation();
    return FTransform(Rotation, Location);
}

bool UColossusClimbingComponent::StartGrabWarp(AIshibashiriBoss* Candidate)
{
    if (!Player || !IsValid(Candidate) || bGrabWarping) return false;
    const FTransform Target = MakeGrabWarpTarget(Candidate);
    GrabWarpStartDistance = FVector::Dist(Player->GetActorLocation(), Target.GetLocation());
    GrabWarpStartAngle = FacingAngle(Player, Target);
    if (GrabWarpStartDistance > FMath::Min(GrabRange, MaximumWarpDistance)
        || GrabWarpStartAngle > MaximumWarpAngle) return false;
    UMotionWarpingComponent* Warping = Player->GetMotionWarping();
    if (!Warping) return false;
    Warping->AddOrUpdateWarpTargetFromTransform(GrabWarpTargetName, Target);
    if (!Player->BeginGrabWarpAnimation())
    {
        Warping->RemoveWarpTarget(GrabWarpTargetName);
        UE_LOG(LogTemp, Warning, TEXT("GRAB_WARP_CANCEL AssetMissing distance=%.1f angle=%.1f"), GrabWarpStartDistance, GrabWarpStartAngle);
        return false;
    }
    bGrabWarping = true;
    GrabWarpBoss = Candidate;
    GrabWarpElapsed = 0.f;
    GrabWarpDuration = Player->GetGrabWarpAnimationLength();
    GrabWarpStartLocation = Player->GetActorLocation();
    Player->StopJumping();
    Player->GetCharacterMovement()->ClearAccumulatedForces();
    Player->GetCharacterMovement()->StopMovementImmediately();
    // Flying lets CharacterMovement consume montage Root Motion without gravity;
    // player movement input is independently gated by IsGrabWarping().
    Player->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
    Player->GetCharacterMovement()->bOrientRotationToMovement = false;
    AddTickPrerequisiteActor(Candidate);
    UE_LOG(LogTemp, Display, TEXT("GRAB_WARP_START distance=%.1f angle=%.1f duration=%.3f"),
        GrabWarpStartDistance, GrabWarpStartAngle, GrabWarpDuration);
    return true;
}

void UColossusClimbingComponent::CancelGrabWarp(const TCHAR* Reason)
{
    if (!bGrabWarping) return;
    AIshibashiriBoss* Previous = GrabWarpBoss;
    bGrabWarping = false;
    GrabWarpBoss = nullptr;
    GrabWarpElapsed = GrabWarpDuration = 0.f;
    if (Previous) RemoveTickPrerequisiteActor(Previous);
    if (Player)
    {
        if (UMotionWarpingComponent* Warping = Player->GetMotionWarping()) Warping->RemoveWarpTarget(GrabWarpTargetName);
        Player->EndGrabWarpAnimation();
        Player->GetCharacterMovement()->ClearAccumulatedForces();
        Player->GetCharacterMovement()->StopMovementImmediately();
        Player->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        Player->GetCharacterMovement()->bOrientRotationToMovement = true;
    }
    UE_LOG(LogTemp, Warning, TEXT("GRAB_WARP_CANCEL %s"), Reason);
}

void UColossusClimbingComponent::CompleteGrabWarp()
{
    AIshibashiriBoss* Candidate = GrabWarpBoss;
    const FTransform Target = IsValid(Candidate) ? MakeGrabWarpTarget(Candidate) : FTransform::Identity;
    const float DistanceError = Player ? FVector::Dist(Player->GetActorLocation(), Target.GetLocation()) : MAX_flt;
    const float AngleError = Player ? FacingAngle(Player, Target) : 180.f;
    if (!IsValid(Candidate) || DistanceError > CompletionDistanceTolerance || AngleError > CompletionAngleTolerance)
    {
        CancelGrabWarp(TEXT("CompletionTolerance"));
        return;
    }
    bGrabWarping = false;
    GrabWarpBoss = nullptr;
    if (UMotionWarpingComponent* Warping = Player->GetMotionWarping()) Warping->RemoveWarpTarget(GrabWarpTargetName);
    Player->EndGrabWarpAnimation();
    Boss = Candidate;
    Node = 0; Destination = INDEX_NONE; Progress = 0.f; UnsafeBuckTime = 0.f;
    InputDelay = .15f;
    Player->GetCharacterMovement()->StopMovementImmediately();
    Player->GetCharacterMovement()->ClearAccumulatedForces();
    Player->GetCharacterMovement()->DisableMovement();
    Player->GetCharacterMovement()->bOrientRotationToMovement = false;
    Player->GetCapsuleComponent()->IgnoreActorWhenMoving(Boss, true);
    // The prerequisite installed for the moving warp target remains valid for climbing.
    UE_LOG(LogTemp, Display, TEXT("GRAB_WARP_COMPLETE position_error=%.1f angle_error=%.1f"), DistanceError, AngleError);
}

void UColossusClimbingComponent::UpdateGrabWarp(float Dt)
{
    if (!bGrabWarping) return;
    if (!Player || !IsValid(GrabWarpBoss) || Player->GetHealth() <= 0)
    {
        CancelGrabWarp(TEXT("InvalidParticipant")); return;
    }
    if (GrabWarpBoss->GetState() == EIshibashiriState::Charge || Player->GetCharacterMovement()->IsFalling())
    {
        CancelGrabWarp(TEXT("UnsafeState")); return;
    }
    const FTransform Target = MakeGrabWarpTarget(GrabWarpBoss);
    Player->GetMotionWarping()->AddOrUpdateWarpTargetFromTransform(GrabWarpTargetName, Target);
    GrabWarpElapsed += Dt;
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("GrabWarpDebug")))
    {
        DrawDebugCoordinateSystem(GetWorld(), Target.GetLocation(), Target.Rotator(), 45.f, false, -1.f, 0, 2.f);
        DrawDebugSphere(GetWorld(), Player->GetActorLocation(), 7.f, 12, FColor::Cyan, false, -1.f, 0, 2.f);
        DrawDebugSphere(GetWorld(), GrabWarpStartLocation, 7.f, 12, FColor::Blue, false, -1.f, 0, 2.f);
        DrawDebugLine(GetWorld(), Player->GetActorLocation(), Target.GetLocation(), FColor::Yellow, false, -1.f, 0, 1.f);
        DrawDebugString(GetWorld(), Player->GetActorLocation()+FVector(0,0,120),
            FString::Printf(TEXT("GrabWarp ACTIVE  Distance %.1f  Angle %.1f"),
                FVector::Dist(Player->GetActorLocation(),Target.GetLocation()), FacingAngle(Player,Target)),
            nullptr, FColor::White, 0.f, true);
    }
#endif
    if (GrabWarpDuration <= 0.f || GrabWarpElapsed >= GrabWarpDuration) CompleteGrabWarp();
}
void UColossusClimbingComponent::Detach(bool bJump)
{
    if (bGrabWarping) { CancelGrabWarp(TEXT("Detach")); return; }
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
    if (bGrabWarping) CancelGrabWarp(TEXT("Reset"));
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
    UpdateGrabWarp(Dt);
    if (bGrabWarping) return;
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
