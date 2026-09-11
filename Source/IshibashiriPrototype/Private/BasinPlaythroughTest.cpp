#include "BasinPlaythroughTest.h"
#include "BasinPrototypeArena.h"
#include "ColossusClimbingComponent.h"
#include "IshibashiriBoss.h"
#include "PrototypeGameMode.h"
#include "PrototypePlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/LightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "InputKeyEventArgs.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

ABasinPlaythroughTest::ABasinPlaythroughTest()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void ABasinPlaythroughTest::BeginPlay()
{
    Super::BeginPlay();
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestRun="), RunId);
    int32 FPS = 60;
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestFPS="), FPS);
    bPreviousFixedTimeStep = FApp::UseFixedTimeStep();
    PreviousFixedDeltaTime = FApp::GetFixedDeltaTime();
    FApp::SetUseFixedTimeStep(true);
    FApp::SetFixedDeltaTime(1.0 / FMath::Clamp(FPS, 15, 240));
    bCapture = FParse::Param(FCommandLine::Get(), TEXT("PrototypeCapture"));
    Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (!Mode || !Mode->GetBasinArena() || !Mode->GetPlayer() || !Mode->GetBoss())
    {
        Fail(TEXT("basin encounter did not spawn"));
        return;
    }
    SpawnLocation = Mode->GetBasinArena()->PlayerStart;
    ABasinPrototypeArena* Arena = Mode->GetBasinArena();
    PlayerStartTransform = FTransform(FRotator::ZeroRotator, Arena->PlayerStart);
    BossStartTransform = FTransform(FRotator(0.f, 180.f, 0.f), Arena->BossStart);
    TArray<UStaticMeshComponent*> Meshes; Arena->GetComponents(Meshes); InitialStaticMeshes = Meshes.Num();
    TArray<ULightComponent*> Lights; Arena->GetComponents(Lights); InitialLights = Lights.Num();
    TArray<UExponentialHeightFogComponent*> Fogs; Arena->GetComponents(Fogs); InitialFogs = Fogs.Num();
    InitialRocks = Arena->GetVisualRockCount(); InitialBoundaries = Arena->GetBoundaryCount(); InitialAccents = Arena->GetAccentCount();
    UE_LOG(LogTemp, Display, TEXT("BASIN_SCENARIO_BEGIN %s %s"), *RunId, *Diagnostic());
    Shot(TEXT("01-Start"));
}

void ABasinPlaythroughTest::EndPlay(const EEndPlayReason::Type Reason)
{
    ReleaseAll();
    RestoreExecutionSettings();
    Super::EndPlay(Reason);
}

void ABasinPlaythroughTest::RestoreExecutionSettings()
{
    FApp::SetFixedDeltaTime(PreviousFixedDeltaTime);
    FApp::SetUseFixedTimeStep(bPreviousFixedTimeStep);
}

void ABasinPlaythroughTest::Finish(int32 ExitCode)
{
    ReleaseAll();
    RestoreExecutionSettings();
    bFinished = true;
    FPlatformMisc::RequestExitWithStatus(false, ExitCode);
}

void ABasinPlaythroughTest::Hold(const FKey& Key, bool bDown)
{
    if (!Mode || !Mode->GetPlayer() || Held.Contains(Key) == bDown) return;
    if (APlayerController* PC = Cast<APlayerController>(Mode->GetPlayer()->GetController()))
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, bDown ? IE_Pressed : IE_Released, bDown ? 1.f : 0.f));
    if (bDown) Held.Add(Key); else Held.Remove(Key);
}

void ABasinPlaythroughTest::Tap(const FKey& Key)
{
    Hold(Key, true);
    PendingRelease.AddUnique(Key);
}

void ABasinPlaythroughTest::AimAndMove(const FVector& Destination)
{
    APrototypePlayer* Player = Mode->GetPlayer();
    APlayerController* PC = Cast<APlayerController>(Player->GetController());
    if (!PC) return;
    const float DesiredYaw = (Destination - Player->GetActorLocation()).Rotation().Yaw;
    const float DeltaYaw = FMath::FindDeltaAngleDegrees(PC->GetControlRotation().Yaw, DesiredYaw);
    // MouseX follows the same input mapping as normal play. Re-evaluate every frame
    // rather than assuming a fixed travel time or a stationary target.
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::MouseX, IE_Axis, FMath::Clamp(DeltaYaw * .08f, -8.f, 8.f)));
    Hold(EKeys::W, FMath::Abs(DeltaYaw) < 28.f);
}

bool ABasinPlaythroughTest::DriveApproach()
{
    APrototypePlayer* P = Mode->GetPlayer();
    AIshibashiriBoss* B = Mode->GetBoss();
    UColossusClimbingComponent* C = P->GetClimbing();
    AimAndMove(B->GetClimbPosition(0));
    if (bDodgedThisThreat && !P->IsDodging()) Hold(EKeys::D, false);
    if ((B->GetState() == EIshibashiriState::Telegraph || B->GetState() == EIshibashiriState::Charge)
        && !bDodgedThisThreat && !P->GetCharacterMovement()->IsFalling())
    {
        Hold(EKeys::D, true); Tap(EKeys::LeftShift); bDodgedThisThreat = true;
    }
    if (B->GetState() == EIshibashiriState::Chase || B->GetState() == EIshibashiriState::Recover) bDodgedThisThreat = false;
    const float Distance = FVector::Dist(P->GetActorLocation(), B->GetClimbPosition(0));
    if (Distance <= C->GrabRange - 12.f && B->GetState() != EIshibashiriState::Charge)
    {
        ReleaseAll(); Tap(EKeys::E); return true;
    }
    return false;
}

bool ABasinPlaythroughTest::IsOnBasinFloor(FString* Detail) const
{
    const APrototypePlayer* P = Mode->GetPlayer();
    const ABasinPrototypeArena* Arena = Mode->GetBasinArena();
    const UCharacterMovementComponent* Movement = P->GetCharacterMovement();
    const UPrimitiveComponent* Floor = Arena->GetFloorComponent();
    const float CapsuleBottom = P->GetActorLocation().Z - P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const float FloorTop = Arena->GetActorLocation().Z;
    const bool bValid = Movement->IsWalking() && Movement->CurrentFloor.IsWalkableFloor()
        && Movement->CurrentFloor.HitResult.GetComponent() == Floor
        && FMath::Abs(CapsuleBottom - FloorTop) <= 5.f;
    if (Detail) *Detail = FString::Printf(TEXT("walking=%d walkable=%d floor=%s expected=%s capsuleBottom=%.2f floorTop=%.2f"),
        Movement->IsWalking(), Movement->CurrentFloor.IsWalkableFloor(),
        *GetNameSafe(Movement->CurrentFloor.HitResult.GetComponent()), *GetNameSafe(Floor), CapsuleBottom, FloorTop);
    return bValid;
}

bool ABasinPlaythroughTest::ValidateRetryReset(FString& Detail) const
{
    const APrototypePlayer* P = Mode->GetPlayer(); const AIshibashiriBoss* B = Mode->GetBoss();
    const UColossusClimbingComponent* C = P->GetClimbing(); const ABasinPrototypeArena* Arena = Mode->GetBasinArena();
    TArray<UStaticMeshComponent*> Meshes; Arena->GetComponents(Meshes);
    TArray<ULightComponent*> Lights; Arena->GetComponents(Lights);
    TArray<UExponentialHeightFogComponent*> Fogs; Arena->GetComponents(Fogs);
    const bool bTransforms = P->GetActorTransform().Equals(PlayerStartTransform, .1f)
        && B->GetActorTransform().Equals(BossStartTransform, .1f);
    const bool bPlayer = P->GetHealth() == P->MaxHealth && !P->IsDodging() && !P->IsAttacking()
        && !P->IsGrabbing() && P->GetCharacterMovement()->IsWalking();
    const bool bBoss = B->GetHealth() == B->MaxHealth && B->GetPurifiedCount() == 0
        && B->GetState() == EIshibashiriState::Chase;
    const bool bInput = !C->IsClimbing() && !C->IsGripping() && !C->HasMovementInput();
    const bool bArena = Meshes.Num() == InitialStaticMeshes && Lights.Num() == InitialLights && Fogs.Num() == InitialFogs
        && Arena->GetVisualRockCount() == InitialRocks && Arena->GetBoundaryCount() == InitialBoundaries
        && Arena->GetAccentCount() == InitialAccents;
    Detail = FString::Printf(TEXT("transforms=%d player=%d boss=%d input=%d arena=%d hp=%d/%d bossHp=%d/%d purified=%d state=%s components(mesh/light/fog)=%d/%d/%d"),
        bTransforms,bPlayer,bBoss,bInput,bArena,P->GetHealth(),P->MaxHealth,B->GetHealth(),B->MaxHealth,
        B->GetPurifiedCount(),*B->GetStateLabel(),Meshes.Num(),Lights.Num(),Fogs.Num());
    return bTransforms && bPlayer && bBoss && bInput && bArena;
}

void ABasinPlaythroughTest::Next(EPhase NewPhase, float Timeout)
{
    Hold(EKeys::W, false);
    Hold(EKeys::A, false);
    Hold(EKeys::D, false);
    Phase = NewPhase;
    PhaseTime = 0.f;
    PhaseTimeout = Timeout;
    bDodgedThisThreat = false;
}

void ABasinPlaythroughTest::ReleaseAll()
{
    if (!Mode || !Mode->GetPlayer()) { Held.Empty(); PendingRelease.Empty(); return; }
    for (const FKey& Key : Held.Array()) Hold(Key, false);
    PendingRelease.Empty();
    if (APlayerController* PC = Cast<APlayerController>(Mode->GetPlayer()->GetController()))
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::MouseX, IE_Axis, 0.f));
}

FString ABasinPlaythroughTest::Diagnostic() const
{
    if (!Mode || !Mode->GetPlayer() || !Mode->GetBoss()) return TEXT("encounter=invalid");
    const APrototypePlayer* P = Mode->GetPlayer();
    const AIshibashiriBoss* B = Mode->GetBoss();
    const UColossusClimbingComponent* C = P->GetClimbing();
    return FString::Printf(TEXT("phase=%d player=%s boss=%s entryDistance=%.1f bossState=%s climb=%d node=%d moving=%d movement=%d"),
        int32(Phase), *P->GetActorLocation().ToCompactString(), *B->GetActorLocation().ToCompactString(),
        FVector::Dist(P->GetActorLocation(), B->GetClimbPosition(0)), *B->GetStateLabel(), C->IsClimbing(),
        C->GetNode(), C->IsMoving(), int32(P->GetCharacterMovement()->MovementMode));
}

void ABasinPlaythroughTest::Fail(const TCHAR* Reason)
{
    if (bFinished) return;
    UE_LOG(LogTemp, Error, TEXT("BASIN_SCENARIO_FAIL %s reason=%s %s"), *RunId, Reason, *Diagnostic());
    Finish(1);
}

void ABasinPlaythroughTest::Shot(const TCHAR* Name)
{
    if (!bCapture) return;
    const FString Dir = FPaths::ProjectSavedDir() / TEXT("Screenshots/Basin") / RunId;
    IFileManager::Get().MakeDirectory(*Dir, true);
    FScreenshotRequest::RequestScreenshot(Dir / (FString(Name) + TEXT(".png")), false, false);
}

void ABasinPlaythroughTest::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bFinished || !Mode) return;
    for (const FKey& Key : PendingRelease) Hold(Key, false);
    PendingRelease.Empty();
    PhaseTime += DeltaSeconds;
    TotalTime += DeltaSeconds;
    if (Mode->GetResult() == EEncounterResult::Defeat) { Fail(TEXT("player defeated during traversal")); return; }
    if (PhaseTime > PhaseTimeout) { Fail(*FString::Printf(TEXT("phase %d timeout"), int32(Phase))); return; }

    APrototypePlayer* P = Mode->GetPlayer();
    AIshibashiriBoss* B = Mode->GetBoss();
    UColossusClimbingComponent* C = P->GetClimbing();
    if ((Phase == EPhase::Approach || Phase == EPhase::Mount || Phase == EPhase::RetryApproach)
        && P->GetCharacterMovement()->IsFalling())
    { Fail(TEXT("unexpected fall while approaching climb hold")); return; }
    switch (Phase)
    {
    case EPhase::Approach:
        if (DriveApproach())
        {
            Shot(TEXT("02-BeforeMount"));
            Next(EPhase::Mount, 2.f);
        }
        break;
    case EPhase::Mount:
        if (C->IsClimbing()) { Hold(EKeys::W, true); Next(EPhase::Climb, 12.f); Hold(EKeys::W, true); }
        break;
    case EPhase::Climb:
        if (!C->IsClimbing()) { Fail(TEXT("unexpected fall before rest ledge")); return; }
        Hold(EKeys::E, B->IsBuckWarning() || B->IsBucking());
        Hold(EKeys::W, true);
        if (C->GetNode() == 3 && !C->IsMoving())
        {
            Hold(EKeys::W, false);
            StaminaAtLedge = C->GetStamina();
            Shot(TEXT("03-FirstLedge"));
            Next(EPhase::Rest, 3.f);
        }
        break;
    case EPhase::Rest:
        if (!C->IsClimbing()) { Fail(TEXT("detached before ledge recovery")); return; }
        Hold(EKeys::E, B->IsBuckWarning() || B->IsBucking());
        if (C->GetNode() != 3 || C->IsMoving() || FVector::Dist(P->GetActorLocation(), B->GetClimbPosition(3)) > 3.f)
        { Fail(TEXT("player did not remain stopped on rest ledge")); return; }
        if (PhaseTime >= 1.f)
        {
            const float Required = FMath::Min(100.f, StaminaAtLedge + 10.f);
            if (C->GetStamina() + 1.f < Required) { Fail(TEXT("stamina did not recover or remain full at rest ledge")); return; }
            Tap(EKeys::SpaceBar);
            Next(EPhase::Detach, 2.f);
        }
        break;
    case EPhase::Detach:
        if (!C->IsClimbing() && P->GetCharacterMovement()->IsFalling()) { bSawFalling = true; Next(EPhase::Land, 8.f); }
        break;
    case EPhase::Land:
        if (bSawFalling && P->GetCharacterMovement()->IsWalking())
        {
            FString FloorDetail;
            if (!IsOnBasinFloor(&FloorDetail)) { Fail(*FString::Printf(TEXT("invalid landing: %s"), *FloorDetail)); return; }
            Shot(TEXT("04-Landed"));
            GroundMoveStart = P->GetActorLocation();
            AimAndMove(B->GetActorLocation());
            Next(EPhase::GroundMove, 3.f);
            Hold(EKeys::W, true);
        }
        break;
    case EPhase::GroundMove:
        AimAndMove(B->GetActorLocation());
        if (!IsOnBasinFloor()) { Fail(TEXT("left basin floor during post-landing movement")); return; }
        if (FVector::Dist2D(GroundMoveStart, P->GetActorLocation()) > 120.f)
        {
            bSawGroundMovement = true; ReleaseAll(); Tap(EKeys::LeftShift); Next(EPhase::GroundDodge, 1.f);
        }
        break;
    case EPhase::GroundDodge:
        if (!bSawGroundMovement) { Fail(TEXT("dodge attempted before post-landing movement succeeded")); return; }
        if (P->IsDodging())
        {
            ReleaseAll(); Tap(EKeys::R);
            FString ResetDetail;
            if (!ValidateRetryReset(ResetDetail)) { Fail(*FString::Printf(TEXT("retry reset invalid at input dispatch: %s"), *ResetDetail)); return; }
            UE_LOG(LogTemp, Display, TEXT("BASIN_RETRY_RESET %s %s"), *RunId, *ResetDetail);
            Shot(TEXT("05-Retry")); Next(EPhase::Retry, 3.f);
        }
        break;
    case EPhase::Retry:
        if (FVector::Dist(P->GetActorLocation(), SpawnLocation) < 3.f && !C->IsClimbing()
            && FMath::IsNearlyEqual(C->GetStamina(), 100.f) && P->GetCharacterMovement()->IsWalking()
            && !C->IsGripping() && !C->HasMovementInput())
        {
            Next(EPhase::RetryApproach, 20.f);
        }
        break;
    case EPhase::RetryApproach:
        DriveApproach();
        if (C->IsClimbing())
        {
            ReleaseAll();
            UE_LOG(LogTemp, Display, TEXT("BASIN_SCENARIO_PASS %s %.2fs %s"), *RunId, TotalTime, *Diagnostic());
            Finish(0);
        }
        break;
    case EPhase::Done: break;
    }
}
