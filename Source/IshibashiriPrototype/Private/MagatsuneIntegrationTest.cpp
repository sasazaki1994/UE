#include "MagatsuneIntegrationTest.h"
#include "MagatsuneGameMode.h"
#include "MagatsuneBoss.h"
#include "MagatsunePlayer.h"
#include "GrabComponent.h"
#include "NushiProgressComponent.h"
#include "NushiEncounterManager.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"

namespace
{
    int32 CountMagatsuneActors(UWorld* W)
    {
        int32 N = 0;
        for (TActorIterator<AActor> I(W); I; ++I) ++N;
        return N;
    }
}

AMagatsuneIntegrationTest::AMagatsuneIntegrationTest()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void AMagatsuneIntegrationTest::BeginPlay()
{
    Super::BeginPlay();
    int32 FPS = 60;
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestRun="), RunId);
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestFPS="), FPS);
    bGamepad = FParse::Param(FCommandLine::Get(), TEXT("MagatsuneGamepad")) || FParse::Param(FCommandLine::Get(), TEXT("CampaignGamepad"));
    bCapture = FParse::Param(FCommandLine::Get(), TEXT("PrototypeCapture"));
    FApp::SetUseFixedTimeStep(true);
    FApp::SetFixedDeltaTime(1.0 / FPS);
    Mode = GetWorld()->GetAuthGameMode<AMagatsuneGameMode>();
    if (!Mode || !Mode->GetBoss() || !Mode->GetPlayer()) Fail(TEXT("spawn"));
    else InitialActors = CountMagatsuneActors(GetWorld());
}

bool AMagatsuneIntegrationTest::Fail(const TCHAR* W)
{
    UE_LOG(LogTemp, Error, TEXT("MAGATSUNE_TEST_FAIL %s phase=%d %s"), *RunId, Phase, W);
    bDone = true;
    FApp::SetUseFixedTimeStep(false);
    FPlatformMisc::RequestExitWithStatus(false, 1);
    return false;
}

void AMagatsuneIntegrationTest::Hold(FKey K, bool D)
{
    if (Held.Contains(K) == D) return;
    FKey M = K;
    if (bGamepad)
    {
        if (K == EKeys::E) M = EKeys::Gamepad_RightShoulder;
        if (K == EKeys::R) M = EKeys::Gamepad_FaceButton_Top;
        if (K == EKeys::SpaceBar) M = EKeys::Gamepad_FaceButton_Bottom;
        if (K == EKeys::LeftMouseButton) M = EKeys::Gamepad_FaceButton_Left;
    }
    Cast<APlayerController>(Mode->GetPlayer()->GetController())
        ->InputKey(FInputKeyEventArgs::CreateSimulated(M, D ? IE_Pressed : IE_Released, D ? 1 : 0));
    if (D) Held.Add(K);
    else Held.Remove(K);
}

void AMagatsuneIntegrationTest::Tap(FKey K)
{
    Hold(K, true);
    Releases.Add(K);
}

void AMagatsuneIntegrationTest::Move(float V)
{
    Axis = V;
    if (!bGamepad)
    {
        Hold(EKeys::W, V > .2f);
        Hold(EKeys::S, V < -.2f);
    }
}

void AMagatsuneIntegrationTest::MoveToward(FVector T)
{
    auto* P = Mode->GetPlayer();
    const FVector D = (T - P->GetActorLocation()).GetSafeNormal2D();
    Move(FVector::DotProduct(D, FRotator(0, P->GetControlRotation().Yaw, 0).Vector()));
}

void AMagatsuneIntegrationTest::Shot(const TCHAR* N)
{
    if (!bCapture || Captures.Contains(N)) return;
    const FString D = FPaths::ProjectSavedDir() / TEXT("Screenshots/Magatsune") / RunId;
    IFileManager::Get().MakeDirectory(*D, true);
    FScreenshotRequest::RequestScreenshot(D / (FString(N) + TEXT(".png")), false, false);
    Captures.Add(N);
}

void AMagatsuneIntegrationTest::Tick(float Dt)
{
    Super::Tick(Dt);
    if (bDone || !Mode) return;
    for (FKey K : Releases) Hold(K, false);
    Releases.Empty();
    auto* P = Mode->GetPlayer();
    auto* B = Mode->GetBoss();
    if (bGamepad)
        Cast<APlayerController>(P->GetController())->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftY, IE_Axis, Axis));
    Time += Dt;
    Total += Dt;
    if (Total > 240 || Time > 40)
    {
        Fail(TEXT("timeout"));
        return;
    }
    if (P->IsMounted())
    {
        const FTransform E = P->GetGrab()->GetRelativeGrabTransform() * B->GetGrabFrame()->GetActorTransform();
        MaxFollowError = FMath::Max(MaxFollowError, (float)FVector::Dist(E.GetLocation(), P->GetActorLocation()));
        if (MaxFollowError > 1)
        {
            Fail(TEXT("root follow drift"));
            return;
        }
    }
    switch (Phase)
    {
    case 0:
        Shot(TEXT("01-Arrival"));
        MoveToward(B->GetRouteWorld(0));
        if (FVector::Dist(P->GetActorLocation(), B->GetRouteWorld(0)) < 220)
        {
            Move(0);
            Shot(TEXT("02-RootMovement"));
            Tap(EKeys::E);
            Next(1);
        }
        break;
    case 1:
        if (!P->IsMounted())
        {
            Fail(TEXT("grab"));
            break;
        }
        Shot(TEXT("03-FirstGrab"));
        Hold(EKeys::E, true);
        Move(1);
        Next(2);
        break;
    case 2:
        if (P->GetRouteNode() == 3 && !P->IsRouteMoving())
        {
            Move(0);
            Shot(TEXT("04-Kakon1"));
            Tap(EKeys::LeftMouseButton);
            Next(3);
        }
        break;
    case 3:
        Shot(TEXT("05-Phase2Opening"));
        if (B->GetPhase() == EMagatsunePhase::RootRockRoute && Time > B->TransitionSeconds)
        {
            Hold(EKeys::E, false);
            if (!bRecoveryDone)
            {
                Tap(EKeys::SpaceBar);
                Next(4);
            }
            else
            {
                Move(1);
                Next(5);
            }
        }
        break;
    case 4:
        if (P->IsRecovering())
        {
            if (B->GetNushiProgressComponent()->GetPurifiedCount() != 1)
            {
                Fail(TEXT("recovery lost progress"));
                break;
            }
            MoveToward(B->GetRecoveryWorld());
            if (FVector::Dist(P->GetActorLocation(), B->GetRecoveryWorld()) < 220)
            {
                Move(0);
                Tap(EKeys::E);
                bRecoveryDone = true;
                Next(5);
            }
        }
        break;
    case 5:
        Shot(TEXT("06-RootRockTransition"));
        Move(1);
        if (P->GetRouteNode() == 7 && !P->IsRouteMoving())
        {
            Move(0);
            Shot(TEXT("07-Kakon2"));
            Tap(EKeys::LeftMouseButton);
            Hold(EKeys::E, true);
            Next(6);
        }
        break;
    case 6:
        Shot(TEXT("08-FinalRootRise"));
        if (B->GetPhase() == EMagatsunePhase::FinalRise && Time > B->TransitionSeconds)
        {
            Shot(TEXT("09-FinalCling"));
            Hold(EKeys::E, false);
            Move(1);
            Next(7);
        }
        break;
    case 7:
        if (P->GetRouteNode() == 11 && !P->IsRouteMoving())
        {
            Move(0);
            Shot(TEXT("10-Kakon3"));
            Tap(EKeys::LeftMouseButton);
            Next(8);
        }
        break;
    case 8:
        if (B->GetPhase() == EMagatsunePhase::Calm)
        {
            Shot(TEXT("11-Calm"));
            if (B->GetNushiProgressComponent()->GetPurifiedCount() != 3 ||
                Mode->GetManager()->GetEncounterState() != ENushiEncounterState::Completed)
            {
                Fail(TEXT("lifecycle"));
                break;
            }
            Next(9);
        }
        break;
    case 9:
        Shot(TEXT("12-Victory"));
        if (Time > .3f)
        {
            ++Round;
            if (FParse::Param(FCommandLine::Get(), TEXT("CampaignE2E")) && Round >= 2)
            {
                UE_LOG(LogTemp, Display, TEXT("MAGATSUNE_TEST_PASS %s rounds=2 recovery=1 seconds=%.3f follow_error=%.6f gamepad=%d"),
                    *RunId, Total, MaxFollowError, bGamepad);
                bDone = true;
                FApp::SetUseFixedTimeStep(false);
            }
            else
            {
                Tap(EKeys::R);
                Next(10);
            }
        }
        break;
    case 10:
        if (Time > .3f)
        {
            if (B->GetNushiProgressComponent()->GetPurifiedCount() != 0 || P->IsMounted() ||
                CountMagatsuneActors(GetWorld()) != InitialActors)
            {
                Fail(TEXT("retry reset or actor growth"));
                break;
            }
            if (Round >= 2)
            {
                UE_LOG(LogTemp, Display, TEXT("MAGATSUNE_TEST_PASS %s rounds=2 recovery=1 seconds=%.3f follow_error=%.6f gamepad=%d"),
                    *RunId, Total, MaxFollowError, bGamepad);
                bDone = true;
                FApp::SetUseFixedTimeStep(false);
                FPlatformMisc::RequestExitWithStatus(false, 0);
            }
            else Next(0);
        }
        break;
    }
}
