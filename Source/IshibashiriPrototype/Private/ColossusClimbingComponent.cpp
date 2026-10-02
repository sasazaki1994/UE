#include "ColossusClimbingComponent.h"
#include "PrototypePlayer.h"
#include "PlayerSenseComponent.h"
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
    // Every authored route hold is creature-local. The optional Control Rig can
    // therefore keep contact from foreleg through both shoulders and the back.
    return IsValid(Boss) && Node >= 0 && (Destination == INDEX_NONE || Destination >= 0);
}

void UColossusClimbingComponent::UpdateIK(float Dt)
{
    const float Desired = IsIKVerticalSlice() ? (IsResting() && !Boss->IsBucking() ? .82f : 1.f) : 0.f;
    const float Seconds = Desired > IKWeight ? IKBlendInSeconds : IKBlendOutSeconds;
    IKWeight = FMath::FInterpConstantTo(IKWeight, Desired, Dt, 1.f / FMath::Max(.01f, Seconds));
    if (!IsValid(Boss)) return;

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
void UColossusClimbingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Reset();
    Super::EndPlay(EndPlayReason);
}
bool UColossusClimbingComponent::IsClimbing() const
{
    return IsValid(Boss) && Node != INDEX_NONE;
}
bool UColossusClimbingComponent::IsResting() const
{
    return IsClimbing() && Destination == INDEX_NONE && Boss->IsRestNode(Node);
}
void UColossusClimbingComponent::GrabPressed()
{
    bGripHeld = true;
    // Cling and repeated presses during an accepted approach do not queue a mount.
    if (!IsValid(Player) || Boss || bGrabWarping) return;
    ClearGrabRequest();
    APrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    AIshibashiriBoss* Candidate = Mode ? Mode->GetBoss() : nullptr;
    if (!Mode || !Mode->IsEncounterActive() || !IsValid(Candidate)
        || !Player->bUseRouteClimbing || Player->GetHealth() <= 0) return;
    if (!Candidate->CanMount()) return;
    if (GetGrabDistance(Candidate) > FMath::Min(GrabRange, MaximumWarpDistance)) return;
    if (TryStartGrab(Candidate)) return;

    // Preserve this press only for the current nearby Kneel. Holding the button
    // cannot renew it, or cause a later charge cycle to mount automatically.
    BufferedGrabBoss = Candidate;
    GrabBufferRemaining = FMath::Max(0.f, GrabBufferSeconds);
    GrabFeedbackRemaining = .75f;
    GrabFeedback = Stamina < MinimumGrabStamina ? TEXT("スタミナの回復を待つ")
        : FacingAngle(Player, MakeGrabWarpTarget(Candidate)) > MaximumWarpAngle ? TEXT("金色の印へ向き直る")
        : TEXT("姿勢を整えて取り付く");
    UE_LOG(LogTemp, Display, TEXT("GRAB_BUFFERED distance=%.1f angle=%.1f seconds=%.3f"),
        GetGrabDistance(Candidate), FacingAngle(Player, MakeGrabWarpTarget(Candidate)), GrabBufferRemaining);
}

float UColossusClimbingComponent::GetGrabDistance(const AIshibashiriBoss* Candidate) const
{
    return IsValid(Player) && IsValid(Candidate)
        ? FVector::Dist(Player->GetActorLocation(), Candidate->GetClimbPosition(0)) : MAX_flt;
}

bool UColossusClimbingComponent::TryStartGrab(AIshibashiriBoss* Candidate)
{
    if (!IsValid(Player) || !IsValid(Candidate) || !Candidate->CanMount()
        || Boss || Player->IsGrabbing() || !Player->bUseRouteClimbing || Player->GetHealth() <= 0
        || Stamina < MinimumGrabStamina || Player->IsDodging() || Player->IsAttacking()
        || Player->GetCharacterMovement()->IsFalling()) return false;
    const FTransform WarpTarget = MakeGrabWarpTarget(Candidate);
    if (GetGrabDistance(Candidate) > FMath::Min(GrabRange, MaximumWarpDistance)
        || FacingAngle(Player, WarpTarget) > MaximumWarpAngle) return false;
    ClearGrabRequest();
    if (!StartGrabWarp(Candidate)) StartFallbackGrabApproach(Candidate);
    return true;
}

void UColossusClimbingComponent::ClearGrabRequest()
{
    BufferedGrabBoss.Reset();
    GrabBufferRemaining = 0.f;
    GrabFeedback.Empty();
    GrabFeedbackRemaining = 0.f;
}

void UColossusClimbingComponent::UpdateBufferedGrab(float Dt)
{
    const APrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (!Mode || !Mode->IsEncounterActive() || !IsValid(Player) || Player->GetHealth() <= 0
        || !Player->bUseRouteClimbing || Player->IsGrabbing())
    {
        ClearGrabRequest();
        return;
    }
    GrabFeedbackRemaining = FMath::Max(0.f, GrabFeedbackRemaining - FMath::Max(0.f, Dt));
    if (GrabFeedbackRemaining <= 0.f) GrabFeedback.Empty();
    if (GrabBufferRemaining <= 0.f) return;
    AIshibashiriBoss* Candidate = BufferedGrabBoss.Get();
    if (!IsValid(Candidate) || Mode->GetBoss() != Candidate || !Candidate->CanMount()
        || GetGrabDistance(Candidate) > FMath::Min(GrabRange, MaximumWarpDistance))
    {
        ClearGrabRequest();
        return;
    }
    // Expire before checking eligibility so a long frame cannot replay stale input.
    GrabBufferRemaining = FMath::Max(0.f, GrabBufferRemaining - FMath::Max(0.f, Dt));
    if (GrabBufferRemaining <= 0.f)
    {
        BufferedGrabBoss.Reset();
        return;
    }
    TryStartGrab(Candidate);
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
    if (!IsValid(Player) || !IsValid(Candidate) || !Candidate->CanMount() || bGrabWarping) return false;
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
    bFallbackGrabApproach = false;
    GrabWarpBoss = Candidate;
    GrabWarpElapsed = 0.f;
    GrabWarpDuration = Player->GetGrabWarpAnimationLength();
    GrabWarpStartLocation = Player->GetActorLocation();
    GrabWarpStartRotation = Player->GetActorQuat();
    GrabWarpStartRelative = Player->GetActorTransform().GetRelativeTransform(Candidate->GetActorTransform());
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

void UColossusClimbingComponent::StartFallbackGrabApproach(AIshibashiriBoss* Candidate)
{
    // The shipped rig has no authored grab montage. Preserve the same mount
    // rules while moving the capsule over a short, visible interval.
    bGrabWarping = bFallbackGrabApproach = true;
    GrabWarpBoss = Candidate;
    GrabWarpElapsed = 0.f;
    GrabWarpDuration = FMath::Max(.1f, FallbackApproachSeconds);
    GrabWarpStartLocation = Player->GetActorLocation();
    GrabWarpStartRotation = Player->GetActorQuat();
    GrabWarpStartRelative = Player->GetActorTransform().GetRelativeTransform(Candidate->GetActorTransform());
    Player->StopJumping();
    Player->GetCharacterMovement()->ClearAccumulatedForces();
    Player->GetCharacterMovement()->StopMovementImmediately();
    Player->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
    Player->GetCharacterMovement()->bOrientRotationToMovement = false;
    Player->GetCapsuleComponent()->IgnoreActorWhenMoving(Candidate, true);
    AddTickPrerequisiteActor(Candidate);
    UE_LOG(LogTemp, Display, TEXT("GRAB_APPROACH_START duration=%.3f asset_fallback=1"), GrabWarpDuration);
}

void UColossusClimbingComponent::CancelGrabWarp(const TCHAR* Reason)
{
    if (!bGrabWarping) return;
    AIshibashiriBoss* Previous = GrabWarpBoss;
    bGrabWarping = false;
    const bool bWasFallback = bFallbackGrabApproach;
    bFallbackGrabApproach = false;
    GrabWarpBoss = nullptr;
    GrabWarpElapsed = GrabWarpDuration = 0.f;
    if (IsValid(Previous)) RemoveTickPrerequisiteActor(Previous);
    if (IsValid(Player))
    {
        if (bWasFallback && IsValid(Previous)) Player->GetCapsuleComponent()->IgnoreActorWhenMoving(Previous, false);
        if (UMotionWarpingComponent* Warping = Player->GetMotionWarping()) Warping->RemoveWarpTarget(GrabWarpTargetName);
        if (!bWasFallback) Player->EndGrabWarpAnimation();
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
    if (!IsValid(Candidate) || !Candidate->CanMount()
        || DistanceError > CompletionDistanceTolerance || AngleError > CompletionAngleTolerance)
    {
        CancelGrabWarp(TEXT("CompletionTolerance"));
        return;
    }
    bGrabWarping = false;
    const bool bWasFallback = bFallbackGrabApproach;
    bFallbackGrabApproach = false;
    GrabWarpBoss = nullptr;
    if (!bWasFallback)
    {
        if (UMotionWarpingComponent* Warping = Player->GetMotionWarping()) Warping->RemoveWarpTarget(GrabWarpTargetName);
        Player->EndGrabWarpAnimation();
    }
    Boss = Candidate;
    Boss->NotifyMounted();
    Node = 0; Destination = INDEX_NONE; Progress = 0.f; UnsafeBuckTime = 0.f;
    InputDelay = .15f;
    Player->GetCharacterMovement()->StopMovementImmediately();
    Player->GetCharacterMovement()->ClearAccumulatedForces();
    Player->GetCharacterMovement()->DisableMovement();
    Player->GetCharacterMovement()->bOrientRotationToMovement = false;
    Player->GetCapsuleComponent()->IgnoreActorWhenMoving(Boss, true);
    // The prerequisite installed for the moving warp target remains valid for climbing.
    UE_LOG(LogTemp, Display, TEXT("GRAB_%s_COMPLETE position_error=%.1f angle_error=%.1f"),
        bWasFallback ? TEXT("APPROACH") : TEXT("WARP"), DistanceError, AngleError);
}

void UColossusClimbingComponent::UpdateGrabWarp(float Dt)
{
    if (!bGrabWarping) return;
    if (!IsValid(Player) || !IsValid(GrabWarpBoss) || Player->GetHealth() <= 0)
    {
        CancelGrabWarp(TEXT("InvalidParticipant")); return;
    }
    if (!GrabWarpBoss->CanMount() || Player->GetCharacterMovement()->IsFalling())
    {
        CancelGrabWarp(TEXT("UnsafeState")); return;
    }
    const FTransform Target = MakeGrabWarpTarget(GrabWarpBoss);
    if (!bFallbackGrabApproach) Player->GetMotionWarping()->AddOrUpdateWarpTargetFromTransform(GrabWarpTargetName, Target);
    GrabWarpElapsed += Dt;
    if (bFallbackGrabApproach)
    {
        const float T = FMath::Clamp(GrabWarpElapsed / GrabWarpDuration, 0.f, 1.f);
        // Preserve the start in boss space so the whole approach is carried by
        // the moving creature. Rotation leads translation: Shirotsura visibly
        // turns/reaches before contact, then settles without a final snap.
        const FTransform MovingStart = GrabWarpStartRelative * GrabWarpBoss->GetActorTransform();
        const float ReachT = FMath::SmoothStep(0.f, .78f, T);
        const float AlignT = FMath::SmoothStep(0.f, .58f, T);
        Player->SetActorLocation(FMath::Lerp(MovingStart.GetLocation(), Target.GetLocation(), ReachT));
        Player->SetActorRotation(FQuat::Slerp(MovingStart.GetRotation(), Target.GetRotation(), AlignT));
    }
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
    ClearGrabRequest();
    if (bGrabWarping) { CancelGrabWarp(TEXT("Detach")); return; }
    // Node survives a destroyed target being nulled by GC, so it also records
    // whether this component still owns the character's disabled movement.
    const bool bWasClimbing = Boss != nullptr || Node != INDEX_NONE;
    AIshibashiriBoss* OldBoss = Boss;
    const bool bTargetValid = IsValid(OldBoss);
    const FVector Away = bTargetValid && IsValid(Player)
        ? (Player->GetActorLocation() - OldBoss->GetActorLocation()).GetSafeNormal2D() : FVector::ZeroVector;
    if (bTargetValid) RemoveTickPrerequisiteActor(OldBoss);
    Boss = nullptr; Node = Destination = INDEX_NONE; Progress = 0.f; bGripHeld = false;
    if (!bWasClimbing || !IsValid(Player)) return;
    if (bTargetValid) Player->GetCapsuleComponent()->IgnoreActorWhenMoving(OldBoss, false);
    Player->GetCharacterMovement()->ClearAccumulatedForces();
    Player->GetCharacterMovement()->StopMovementImmediately();
    Player->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
    Player->GetCharacterMovement()->bOrientRotationToMovement = true;
    if (bJump && bTargetValid) Player->LaunchCharacter(Away * 650.f + FVector(0,0,380), true, true);
}
void UColossusClimbingComponent::Reset()
{
    if (bGrabWarping) CancelGrabWarp(TEXT("Reset"));
    Detach(false); Stamina = 100.f; ForwardInput = RightInput = InputDelay = UnsafeBuckTime = 0.f;
    LastSafeNode = INDEX_NONE; PendingPurificationCore = INDEX_NONE; PurificationRemaining = 0.f;
    bGripHeld = false; IKWeight = 0.f; IKTargets = FClimbingIKTargets();
}

bool UColossusClimbingComponent::IsUsableRecoveryLocation(const FVector& Location) const
{
    return !Location.ContainsNaN() && FMath::IsFinite(Location.X) && FMath::IsFinite(Location.Y)
        && FMath::IsFinite(Location.Z) && Location.SizeSquared() < FMath::Square(WORLD_MAX * .5f);
}

void UColossusClimbingComponent::RecoverFromFall(const TCHAR* Reason)
{
    AIshibashiriBoss* RecoveryBoss = Boss;
    if (!IsValid(Player) || !IsValid(RecoveryBoss)) { Detach(false); return; }

    Stamina = FMath::Clamp(FallRecoveryStamina, 0.f, 100.f);
    UnsafeBuckTime = 0.f;
    RecoveryBoss->GrantShakeImmunity(FallShakeImmunity);

    // A reached authored rest is authoritative and moves with Ishibashiri.
    // Recover directly onto it so purified Kakon and route order are untouched.
    const FVector SafeLedge = LastSafeNode != INDEX_NONE ? RecoveryBoss->GetClimbPosition(LastSafeNode) : FVector::ZeroVector;
    if (LastSafeNode != INDEX_NONE && RecoveryBoss->IsRestNode(LastSafeNode) && IsUsableRecoveryLocation(SafeLedge))
    {
        Node = LastSafeNode;
        Destination = INDEX_NONE;
        Progress = 0.f;
        InputDelay = .18f;
        Player->GetCharacterMovement()->StopMovementImmediately();
        Player->GetCharacterMovement()->DisableMovement();
        Player->SetActorLocation(SafeLedge, false, nullptr, ETeleportType::TeleportPhysics);
        UE_LOG(LogTemp, Display, TEXT("CLIMB_FALL_RECOVERY reason=%s safe_node=%d stamina=%.0f"), Reason, LastSafeNode, Stamina);
        return;
    }

    // Before the first safe ledge, return to the authored foreleg. If that
    // transform is corrupt, the encounter spawn is a stable final fallback;
    // never leave the pawn falling at an indeterminate transform.
    FVector Recovery = RecoveryBoss->GetClimbPosition(0);
    APrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (!IsUsableRecoveryLocation(Recovery) && Mode) Recovery = Mode->GetPlayerRecoverySpawn().GetLocation();
    if (!IsUsableRecoveryLocation(Recovery)) Recovery = IsUsableRecoveryLocation(Player->GetActorLocation()) ? Player->GetActorLocation() : FVector::ZeroVector;
    Detach(false);
    Player->SetActorLocation(Recovery, false, nullptr, ETeleportType::TeleportPhysics);
    Player->GetCharacterMovement()->StopMovementImmediately();
    Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    RecoveryBoss->BeginFallRecoveryWindow(FallRegrabWindow);
    UE_LOG(LogTemp, Display, TEXT("CLIMB_FALL_RECOVERY reason=%s foreleg=1 stamina=%.0f regrab=%.1f"), Reason, Stamina, FallRegrabWindow);
}
bool UColossusClimbingComponent::TryPurify()
{
    if (!IsValid(Player) || !IsResting() || PendingPurificationCore != INDEX_NONE || Boss->IsBucking()) return false;
    PendingPurificationCore = Boss->BeginPurifyCore(Player->GetActorLocation(), PurificationDuration);
    if (PendingPurificationCore == INDEX_NONE) return false;
    PurificationRemaining = PurificationDuration;
    return true;
}
void UColossusClimbingComponent::TickComponent(float Dt, ELevelTick Type, FActorComponentTickFunction* Tick)
{
    Super::TickComponent(Dt, Type, Tick);
    if (!IsValid(Player)) return;
    // Release before IK or encounter checks: a target may be destroyed while
    // the encounter is ending, or its reflected pointer may already be null.
    if ((Boss || Node != INDEX_NONE) && !IsValid(Boss)) Detach(false);
    UpdateBufferedGrab(Dt);
    UpdateIK(Dt);
    UpdateGrabWarp(Dt);
    if (bGrabWarping) return;
    if (PendingPurificationCore != INDEX_NONE)
    {
        PurificationRemaining = FMath::Max(0.f, PurificationRemaining - Dt);
        if (PurificationRemaining <= 0.f)
        {
            const int32 Core = PendingPurificationCore;
            PendingPurificationCore = INDEX_NONE;
            if (IsValid(Boss)) Boss->CompletePurifyCore(Core);
        }
    }
    const APrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (!Mode || !Mode->IsEncounterActive()) return;
    const float RecoveryMultiplier = Player->GetSense()->GetRecoveryMultiplier();
    if (!IsValid(Boss))
    {
        if (!Player->GetCharacterMovement()->IsFalling())
            Stamina = FMath::Min(100.f, Stamina + GroundRecoveryPerSecond * RecoveryMultiplier * Dt);
        return;
    }
    const bool Buck = Boss->IsBucking();
    const bool Rest = IsResting();
    float Drain = Rest ? -LedgeRecoveryPerSecond * RecoveryMultiplier : HangDrainPerSecond;
    if (Buck) Drain = bGripHeld ? BracedBuckDrainPerSecond : UnbracedBuckDrainPerSecond;
    Stamina = FMath::Clamp(Stamina-Drain*Dt, 0.f, 100.f);
    UnsafeBuckTime = Buck && !bGripHeld ? UnsafeBuckTime+Dt : 0.f;
    if (Stamina <= 0.f || UnsafeBuckTime > UnbracedBuckTolerance)
    {
        RecoverFromFall(Stamina <= 0.f ? TEXT("Stamina") : TEXT("Shake"));
        return;
    }
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
            if (Boss->IsRestNode(Node)) LastSafeNode = Node;
        }
    }
    FHitResult Hit;
    Player->SetActorLocation(Position,true,&Hit);
    if (Hit.bBlockingHit && Hit.GetActor() != Boss) { RecoverFromFall(TEXT("BlockedRoute")); return; }
    FVector Facing = Destination != INDEX_NONE ? Boss->GetClimbPosition(Destination)-Position
        : Boss->GetActorLocation()-Position;
    Facing.Z = 0;
    if (!Facing.IsNearlyZero()) Player->SetActorRotation(Facing.Rotation());
}
FString UColossusClimbingComponent::GetHint() const
{
    if (!IsClimbing()) return TEXT("Break posture, then E / RB near the gold foreleg hold during KNEEL");
    if (Boss->IsBucking() || Boss->IsBuckWarning()) return TEXT("HOLD E / RB - brace for the shake! Movement pauses during the shake.");
    if (Node == 5) return TEXT("W / LS up: summit | D / LS right: right-shoulder core | S: descend | Space / A: detach");
    if (Node == 10) return TEXT("W: shoulder core | A / S: main route");
    return IsResting() ? TEXT("Ledge: stamina recovers | LMB / X: purify nearby red core | W/S: traverse | Space / A: detach")
        : TEXT("W/S: climb / descend | Hold E / RB during shaking | Space / A: detach");
}
