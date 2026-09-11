#include "BasinPlaythroughTest.h"
#include "BasinPrototypeArena.h"
#include "ColossusClimbingComponent.h"
#include "IshibashiriBoss.h"
#include "PrototypeGameMode.h"
#include "PrototypePlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
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
    UE_LOG(LogTemp, Display, TEXT("BASIN_SCENARIO_BEGIN %s %s"), *RunId, *Diagnostic());
    Shot(TEXT("01-Start"));
}

void ABasinPlaythroughTest::EndPlay(const EEndPlayReason::Type Reason)
{
    ReleaseAll();
    Super::EndPlay(Reason);
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
    ReleaseAll();
    UE_LOG(LogTemp, Error, TEXT("BASIN_SCENARIO_FAIL %s reason=%s %s"), *RunId, Reason, *Diagnostic());
    bFinished = true;
    FPlatformMisc::RequestExitWithStatus(false, 1);
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
    if (PhaseTime > PhaseTimeout) { Fail(TEXT("phase timeout")); return; }

    APrototypePlayer* P = Mode->GetPlayer();
    AIshibashiriBoss* B = Mode->GetBoss();
    UColossusClimbingComponent* C = P->GetClimbing();
    const float EntryDistance = FVector::Dist(P->GetActorLocation(), B->GetClimbPosition(0));
    switch (Phase)
    {
    case EPhase::Approach:
        AimAndMove(B->GetClimbPosition(0));
        if (bDodgedThisThreat && !P->IsDodging()) Hold(EKeys::D, false);
        if ((B->GetState() == EIshibashiriState::Telegraph || B->GetState() == EIshibashiriState::Charge) && !bDodgedThisThreat
            && !P->GetCharacterMovement()->IsFalling())
        {
            Hold(EKeys::D, true);
            Tap(EKeys::LeftShift);
            bDodgedThisThreat = true;
        }
        if (B->GetState() == EIshibashiriState::Chase || B->GetState() == EIshibashiriState::Recover) bDodgedThisThreat = false;
        if (EntryDistance <= C->GrabRange - 12.f && B->GetState() != EIshibashiriState::Charge)
        {
            ReleaseAll();
            Shot(TEXT("02-BeforeMount"));
            Tap(EKeys::E);
            Next(EPhase::Mount, 2.f);
        }
        break;
    case EPhase::Mount:
        if (C->IsClimbing()) { Hold(EKeys::W, true); Next(EPhase::Climb, 12.f); Hold(EKeys::W, true); }
        break;
    case EPhase::Climb:
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
        if (PhaseTime >= 1.f && C->GetStamina() > StaminaAtLedge + 5.f)
        {
            Tap(EKeys::SpaceBar);
            Next(EPhase::Detach, 2.f);
        }
        break;
    case EPhase::Detach:
        if (!C->IsClimbing() && P->GetCharacterMovement()->IsFalling()) Next(EPhase::Land, 8.f);
        break;
    case EPhase::Land:
        if (!P->GetCharacterMovement()->IsFalling())
        {
            if (P->GetActorLocation().Z < 70.f || P->GetActorLocation().Z > 130.f) { Fail(TEXT("landed outside basin floor height")); return; }
            Shot(TEXT("04-Landed"));
            GroundMoveStart = P->GetActorLocation();
            AimAndMove(B->GetActorLocation());
            Next(EPhase::GroundMove, 3.f);
            Hold(EKeys::W, true);
        }
        break;
    case EPhase::GroundMove:
        AimAndMove(B->GetActorLocation());
        if (PhaseTime > .25f && !P->IsDodging()) Tap(EKeys::LeftShift);
        if (FVector::Dist2D(GroundMoveStart, P->GetActorLocation()) > 160.f && P->IsDodging())
        {
            ReleaseAll();
            Tap(EKeys::R);
            Next(EPhase::Retry, 3.f);
        }
        break;
    case EPhase::Retry:
        if (FVector::Dist(P->GetActorLocation(), SpawnLocation) < 3.f && !C->IsClimbing()
            && C->GetStamina() == 100.f && P->GetCharacterMovement()->IsWalking())
        {
            Shot(TEXT("05-Retry"));
            Next(EPhase::RetryApproach, 20.f);
        }
        break;
    case EPhase::RetryApproach:
        AimAndMove(B->GetClimbPosition(0));
        if (bDodgedThisThreat && !P->IsDodging()) Hold(EKeys::D, false);
        if ((B->GetState() == EIshibashiriState::Telegraph || B->GetState() == EIshibashiriState::Charge) && !bDodgedThisThreat
            && !P->GetCharacterMovement()->IsFalling())
        {
            Hold(EKeys::D, true);
            Tap(EKeys::LeftShift);
            bDodgedThisThreat = true;
        }
        if (B->GetState() == EIshibashiriState::Chase || B->GetState() == EIshibashiriState::Recover) bDodgedThisThreat = false;
        if (EntryDistance <= C->GrabRange - 12.f && B->GetState() != EIshibashiriState::Charge) Tap(EKeys::E);
        if (C->IsClimbing())
        {
            ReleaseAll();
            UE_LOG(LogTemp, Display, TEXT("BASIN_SCENARIO_PASS %s %.2fs %s"), *RunId, TotalTime, *Diagnostic());
            bFinished = true;
            FPlatformMisc::RequestExitWithStatus(false, 0);
        }
        break;
    case EPhase::Done: break;
    }
}
