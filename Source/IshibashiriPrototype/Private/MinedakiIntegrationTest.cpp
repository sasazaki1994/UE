#include "MinedakiIntegrationTest.h"
#include "MinedakiGameMode.h"
#include "MinedakiBoss.h"
#include "MinedakiPlayer.h"
#include "GrabComponent.h"
#include "StaminaComponent.h"
#include "KakonActor.h"
#include "NushiEncounterManager.h"
#include "NushiProgressComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SceneComponent.h"
#include "InputKeyEventArgs.h"
#include "Engine/World.h"
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
    int32 ActorCount(UWorld* W)
    {
        int32 N = 0;
        for (TActorIterator<AActor> It(W); It; ++It) ++N;
        return N;
    }
}

AMinedakiIntegrationTest::AMinedakiIntegrationTest()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void AMinedakiIntegrationTest::BeginPlay()
{
    Super::BeginPlay();
    int32 FPS = 60;
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestRun="), RunId);
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestFPS="), FPS);
    bGamepad = FParse::Param(FCommandLine::Get(), TEXT("MinedakiGamepad")) || FParse::Param(FCommandLine::Get(), TEXT("CampaignGamepad"));
    bCapture = FParse::Param(FCommandLine::Get(), TEXT("PrototypeCapture"));
    FApp::SetUseFixedTimeStep(true);
    FApp::SetFixedDeltaTime(1.0 / FMath::Clamp(FPS, 15, 240));
    Mode = GetWorld()->GetAuthGameMode<AMinedakiGameMode>();
    if (!Check(Mode && Mode->GetBoss() && Mode->GetPlayer() && Mode->GetManager(), TEXT("Encounter spawned"))) return;
    InitialActors = ActorCount(GetWorld());
    AddTickPrerequisiteComponent(Mode->GetPlayer()->GetGrab());
}

bool AMinedakiIntegrationTest::Check(bool Condition, const TCHAR* Message)
{
    if (!Condition)
    {
        UE_LOG(LogTemp, Error, TEXT("MINEDAKI_TEST_FAIL %s phase=%d round=%d seconds=%.3f: %s"), *RunId, Phase, Rounds, Total, Message);
        bDone = true;
        FApp::SetUseFixedTimeStep(false);
        FPlatformMisc::RequestExitWithStatus(false, 1);
    }
    else UE_LOG(LogTemp, Display, TEXT("MINEDAKI_CHECK %s"), Message);
    return Condition;
}

void AMinedakiIntegrationTest::Hold(FKey Key, bool Down)
{
    if (Held.Contains(Key) == Down) return;
    FKey Mapped = Key;
    if (bGamepad)
    {
        if (Key == EKeys::E) Mapped = EKeys::Gamepad_RightShoulder;
        if (Key == EKeys::R) Mapped = EKeys::Gamepad_FaceButton_Top;
        if (Key == EKeys::SpaceBar) Mapped = EKeys::Gamepad_FaceButton_Bottom;
        if (Key == EKeys::LeftMouseButton) Mapped = EKeys::Gamepad_FaceButton_Left;
    }
    Cast<APlayerController>(Mode->GetPlayer()->GetController())
        ->InputKey(FInputKeyEventArgs::CreateSimulated(Mapped, Down ? IE_Pressed : IE_Released, Down ? 1.f : 0.f));
    if (Down) Held.Add(Key);
    else Held.Remove(Key);
}

void AMinedakiIntegrationTest::Tap(FKey Key)
{
    Hold(Key, true);
    Releases.Add(Key);
}

void AMinedakiIntegrationTest::Move(float Forward, float Right)
{
    ForwardAxis = Forward;
    RightAxis = Right;
    if (bGamepad) return;
    Hold(EKeys::W, Forward > .2f);
    Hold(EKeys::S, Forward < -.2f);
    Hold(EKeys::D, Right > .2f);
    Hold(EKeys::A, Right < -.2f);
}

void AMinedakiIntegrationTest::MoveToward(FVector Target)
{
    auto* P = Mode->GetPlayer();
    FVector Direction = (Target - P->GetActorLocation()).GetSafeNormal2D();
    FRotationMatrix Rotation(FRotator(0, P->GetControlRotation().Yaw, 0));
    Move(FVector::DotProduct(Direction, Rotation.GetUnitAxis(EAxis::X)), FVector::DotProduct(Direction, Rotation.GetUnitAxis(EAxis::Y)));
}

void AMinedakiIntegrationTest::Shot(const TCHAR* Name)
{
    if (!bCapture || Captures.Contains(Name)) return;
    const FString Dir = FPaths::ProjectSavedDir() / TEXT("Screenshots/Minedaki") / RunId;
    IFileManager::Get().MakeDirectory(*Dir, true);
    FScreenshotRequest::RequestScreenshot(Dir / (FString(Name) + TEXT(".png")), false, false);
    Captures.Add(Name);
}

void AMinedakiIntegrationTest::Tick(float Dt)
{
    Super::Tick(Dt);
    if (bDone || !Mode) return;
    for (FKey Key : Releases) Hold(Key, false);
    Releases.Empty();
    auto* P = Mode->GetPlayer();
    auto* B = Mode->GetBoss();
    auto* PC = Cast<APlayerController>(P->GetController());
    if (bGamepad)
    {
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftY, IE_Axis, ForwardAxis));
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftX, IE_Axis, RightAxis));
    }
    Time += Dt;
    Total += Dt;
    if (Total > 300 || Time > 50)
    {
        Check(false, TEXT("Input playthrough timeout"));
        return;
    }
    if (P->IsMounted())
    {
        const FTransform Expected = P->GetGrab()->GetRelativeGrabTransform() * B->GetGrabFrame()->GetActorTransform();
        MaxFollowError = FMath::Max(MaxFollowError, static_cast<float>(FVector::Dist(Expected.GetLocation(), P->GetActorLocation())));
        MaxAngleError = FMath::Max(
            MaxAngleError, static_cast<float>(FMath::RadiansToDegrees(Expected.GetRotation().AngularDistance(P->GetActorQuat()))));
        if (MaxFollowError > 1 || MaxAngleError > .1f)
        {
            Check(false, TEXT("Full transform attachment error"));
            return;
        }
    }
    PeakPitch = FMath::Max(PeakPitch, static_cast<float>(B->GetBodyRoot()->GetRelativeRotation().Pitch));
    PeakYaw = FMath::Max(PeakYaw, FMath::Abs(static_cast<float>(B->GetBodyRoot()->GetRelativeRotation().Yaw)));
    PeakRoll = FMath::Max(PeakRoll, FMath::Abs(static_cast<float>(B->GetBodyRoot()->GetRelativeRotation().Roll)));
    switch (Phase)
    {
    case 0:
        if (Time > .1f) Shot(TEXT("01-GroundGrab"));
        MoveToward(B->GetRouteWorld(0));
        if (FVector::Dist(P->GetActorLocation(), B->GetRouteWorld(0)) < P->GrabRange - 30)
        {
            Move(0);
            Tap(EKeys::E);
            Next(1);
        }
        break;
    case 1:
        if (Time > .2f)
        {
            if (!Check(P->IsMounted() && P->GetRouteNode() == 0, TEXT("Ground Grab through shared component"))) return;
            Move(1);
            Next(2);
        }
        break;
    case 2:
        if (B->IsWallMoving())
        {
            Move(0);
            Hold(EKeys::E, true);
            WallRelative = P->GetGrab()->GetRelativeGrabTransform();
            Shot(TEXT("02-Phase1WallClimb"));
            Next(3);
        }
        break;
    case 3:
        if (!P->IsMounted())
        {
            Check(false, TEXT("Phase1 cling failed"));
            return;
        }
        if (B->GetActionState() == EMinedakiActionState::Shaking)
        {
            bSawShake = true;
            Shot(TEXT("03-FirstShake"));
        }
        if (B->GetActionState() == EMinedakiActionState::UpperPlatform)
        {
            Hold(EKeys::E, false);
            Move(1);
            Next(4);
        }
        break;
    case 4:
        if (P->GetRouteNode() == AMinedakiBoss::Kakon1Node && !P->IsRouteMoving())
        {
            Move(0);
            Shot(TEXT("04-Kakon1"));
            Tap(EKeys::LeftMouseButton);
            Hold(EKeys::E, true);
            Next(5);
        }
        break;
    case 5:
        Shot(TEXT("05-Phase2BodyTransition"));
        if (B->GetActionState() == EMinedakiActionState::ArmBridge)
        {
            Hold(EKeys::E, false);
            if (!Check(B->GetNushiProgressComponent()->GetPurifiedCount() == 1 && B->IsRouteNodeEnabled(8), TEXT("Kakon1 opens arm route")))
                return;
            if (Rounds == 0 && RecoveryRuns == 0)
            {
                Shot(TEXT("14-Fall"));
                Tap(EKeys::SpaceBar);
                Next(6);
            }
            else
            {
                Move(1);
                Next(7);
            }
        }
        break;
    case 6:
        if (P->IsRecovering())
        {
            Shot(TEXT("15-Recovery"));
            MoveToward(B->GetRecoveryAnchorWorld());
            if (FVector::Dist(P->GetActorLocation(), B->GetRecoveryAnchorWorld()) < P->GrabRange - 30)
            {
                Move(0);
                Tap(EKeys::E);
                ++RecoveryRuns;
                Next(7);
            }
        }
        break;
    case 7:
        Shot(TEXT("06-ArmRoutePlatform"));
        if (P->GetRouteNode() == AMinedakiBoss::Kakon2Node && !P->IsRouteMoving())
        {
            Move(0);
            Shot(TEXT("07-Kakon2"));
            Tap(EKeys::LeftMouseButton);
            Hold(EKeys::E, true);
            Next(8);
        }
        else Move(1);
        break;
    case 8:
        Shot(TEXT("08-Phase3Transition"));
        if (B->GetActionState() == EMinedakiActionState::FinalRoute)
        {
            Hold(EKeys::E, false);
            Shot(TEXT("09-FinalRoute"));
            Next(14);
        }
        break;
    case 9:
        if (Time > .2f && !P->IsMounted())
        {
            Check(false, TEXT("Final route recovery regrab failed"));
            return;
        }
        if (B->GetActionState() == EMinedakiActionState::FinalRoute) Shot(TEXT("10-FinalCling"));
        if (P->GetRouteNode() == AMinedakiBoss::Kakon3Node && !P->IsRouteMoving())
        {
            Move(0);
            Shot(TEXT("11-Kakon3"));
            Tap(EKeys::LeftMouseButton);
            Next(10);
        }
        else if (P->IsMounted()) Move(1);
        break;
    case 10:
        if (B->GetActionState() == EMinedakiActionState::Calm && B->GetNushiState() == ENushiState::Calm &&
            Mode->GetManager()->GetEncounterState() == ENushiEncounterState::Completed)
        {
            Shot(TEXT("12-Calm"));
            if (!Check(B->GetNushiProgressComponent()->GetPurifiedCount() == 3 && B->Telemetry.Phase1Completes == 1 &&
                        B->Telemetry.Phase2Completes == 1 && B->Telemetry.Phase3Completes == 1,
                    TEXT("3/3 Calm Completed Victory lifecycle")))
                return;
            Next(13);
        }
        break;
    case 12:
        if (P->IsRecovering())
        {
            MoveToward(B->GetRecoveryAnchorWorld());
            if (FVector::Dist(P->GetActorLocation(), B->GetRecoveryAnchorWorld()) < P->GrabRange - 30)
            {
                Move(0);
                Tap(EKeys::E);
                ++RecoveryRuns;
                Next(9);
            }
        }
        break;
    case 13:
        Shot(TEXT("13-Victory"));
        if (Time > .2f)
        {
            ++Rounds;
            if (FParse::Param(FCommandLine::Get(), TEXT("CampaignE2E")) && Rounds >= 2)
            {
                UE_LOG(LogTemp, Display, TEXT("MINEDAKI_TEST_PASS %s rounds=2 recovery=%d seconds=%.3f"), *RunId, RecoveryRuns, Total);
                bDone = true;
                FApp::SetUseFixedTimeStep(false);
            }
            else
            {
                Tap(EKeys::R);
                Next(11);
            }
        }
        break;
    case 14:
        if (Time > .2f)
        {
            if (Rounds == 0 && RecoveryRuns == 1)
            {
                Tap(EKeys::SpaceBar);
                Next(12);
            }
            else
            {
                Move(1);
                Next(9);
            }
        }
        break;
    case 11:
        if (Time > .25f)
        {
            if (!Check(!P->IsMounted() && !P->HasFallen() && !P->IsRecovering() && P->GetStamina()->GetCurrentStamina() == 100 &&
                        B->GetActionState() == EMinedakiActionState::Grounded && B->GetNushiProgressComponent()->GetPurifiedCount() == 0 &&
                        ActorCount(GetWorld()) == InitialActors,
                    TEXT("Retry fully resets without actor growth")))
                return;
            Shot(TEXT("16-Retry"));
            if (Rounds >= 2 && RecoveryRuns >= 2)
            {
                UE_LOG(LogTemp, Display,
                    TEXT(
                        "MINEDAKI_TEST_PASS %s rounds=2 recovery=2 seconds=%.3f max_position_error_cm=%.6f max_rotation_error_deg=%.6f gamepad=%d"),
                    *RunId, Total, MaxFollowError, MaxAngleError, bGamepad);
                bDone = true;
                FApp::SetUseFixedTimeStep(false);
                FPlatformMisc::RequestExitWithStatus(false, 0);
            }
            else
            {
                bSawShake = false;
                Next(0);
            }
        }
        break;
    }
}
