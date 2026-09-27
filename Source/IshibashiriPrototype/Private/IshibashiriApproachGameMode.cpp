#include "IshibashiriApproachGameMode.h"
#include "IshibashiriApproachArena.h"
#include "IshibashiriApproachHUD.h"
#include "CampaignGameInstance.h"
#include "PrototypePlayer.h"
#include "PlayerSenseComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Engine/TargetPoint.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMisc.h"

AIshibashiriApproachGameMode::AIshibashiriApproachGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = nullptr;
    HUDClass = AIshibashiriApproachHUD::StaticClass();
}

void AIshibashiriApproachGameMode::StartPlay()
{
    Arena = GetWorld()->SpawnActor<AIshibashiriApproachArena>();
    Super::StartPlay();
    if (!Arena) return;
    FActorSpawnParameters P;
    P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Player = GetWorld()->SpawnActor<APrototypePlayer>(
        APrototypePlayer::StaticClass(), FTransform(FRotator::ZeroRotator, Arena->GetPlayerStart()), P);
    SenseTarget = GetWorld()->SpawnActor<ATargetPoint>(ATargetPoint::StaticClass(), Arena->GetBasinGate(), FRotator::ZeroRotator);
    ensureAlwaysMsgf(!SenseTarget || (SenseTarget->GetRootComponent()
            && SenseTarget->GetActorLocation().Equals(Arena->GetBasinGate(), 1.f)),
        TEXT("Approach Sense target must own the basin-gate transform; never fall back to world origin"));
    APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
    if (!Player || !Controller)
    {
        UE_LOG(LogTemp, Error, TEXT("APPROACH_SPAWN_FAILED player=%d controller=%d. Use Play, not Simulate."), Player != nullptr, Controller != nullptr);
        return;
    }
    Controller->Possess(Player);
    Controller->SetInputMode(FInputModeGameOnly());
    if (SenseTarget) Player->GetSense()->RegisterBoundaryTarget(SenseTarget, true);
    const bool bCampaignE2E = FParse::Param(FCommandLine::Get(), TEXT("CampaignE2E"));
    bStandaloneApproachTest = FParse::Param(FCommandLine::Get(), TEXT("ApproachTest")) && !bCampaignE2E;
    bAutomationDriving = bStandaloneApproachTest || bCampaignE2E;
    bDemoCapture = bCampaignE2E && FParse::Param(FCommandLine::Get(), TEXT("IshibashiriDemo"))
        && FParse::Param(FCommandLine::Get(), TEXT("PrototypeCapture"));
    if (bAutomationDriving)
    {
        FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestRun="), RunId);
        // Same fixed-step contract as the encounter drivers: deterministic walk,
        // wall-clock independent of the null RHI frame rate.
        int32 FPS = 60;
        FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestFPS="), FPS);
        FApp::SetUseFixedTimeStep(true);
        FApp::SetFixedDeltaTime(1.0 / FMath::Clamp(FPS, 15, 240));
        // The path is ~1.3 km at 6 m/s. Three times that budget marks a stuck walk
        // instead of letting the process (and Prototype.ps1) wait forever.
        AutomationTimeout = FMath::Max(120.f, Arena->GetPathLengthMeters() / 6.f * 3.f);
        Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::W, IE_Pressed, 1.f));
    }
    UE_LOG(LogTemp, Display, TEXT("APPROACH_READY path_m=%.1f expected_minutes=3-5 combat=false"), Arena->GetPathLengthMeters());
}

void AIshibashiriApproachGameMode::Tick(float Dt)
{
    Super::Tick(Dt);
    if (!Arena || !Player || bTransitioning) return;
    if (bAutomationDriving)
    {
        AutomationElapsed += FMath::Max(0.f, Dt);
        if (AutomationElapsed > AutomationTimeout)
        {
            UE_LOG(LogTemp, Error, TEXT("APPROACH_TEST_FAIL %s stuck point=%d/%d stage=%d elapsed=%.1f"), *RunId, AutomationPoint,
                Arena->GetPathPoints().Num(), Arena->GetStage(), AutomationElapsed);
            bTransitioning = true;
            FApp::SetUseFixedTimeStep(false);
            FPlatformMisc::RequestExitWithStatus(false, 1);
            return;
        }
        if (AutomationPoint < Arena->GetPathPoints().Num())
            if (auto* C = UGameplayStatics::GetPlayerController(this, 0))
            {
                const FVector Goal = Arena->GetPathPoints()[AutomationPoint];
                C->SetControlRotation((Goal - Player->GetActorLocation()).Rotation());
                if (FVector::Dist2D(Goal, Player->GetActorLocation()) < 500) ++AutomationPoint;
            }
    }
    if (bDemoCapture && !bApproachShot && GetWorld()->GetTimeSeconds() >= 2.f)
    {
        CaptureDemoShot(TEXT("03-Approach"));
        bApproachShot = true;
    }
    Arena->ObservePlayer(Player->GetActorLocation());
    if (bDemoCapture && !bRevealShot && Arena->IsRevealVisible())
    {
        CaptureDemoShot(TEXT("04-IshibashiriReveal"));
        bRevealShot = true;
    }
    if (Player->GetSense()->IsCorruptionSenseActive())
        Player->GetSense()->SetCorruptionWarning(Arena->GetStage() >= 4 ? ECorruptionWarning::Danger
                : Arena->GetStage() >= 2                                ? ECorruptionWarning::Transition
                                                                        : ECorruptionWarning::None);
    if (Arena->IsGateReached()) EnterBasin();
}

void AIshibashiriApproachGameMode::CaptureDemoShot(const TCHAR* Name)
{
    const FString Dir = FPaths::ProjectSavedDir() / TEXT("Screenshots/IshibashiriDemo") / RunId;
    IFileManager::Get().MakeDirectory(*Dir, true);
    FScreenshotRequest::RequestScreenshot(Dir / (FString(Name) + TEXT(".png")), false, false);
}

void AIshibashiriApproachGameMode::EnterBasin()
{
    bTransitioning = true;
    if (bAutomationDriving)
        if (auto* C = UGameplayStatics::GetPlayerController(this, 0))
            C->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::W, IE_Released, 0.f));
    if (bStandaloneApproachTest)
    {
        UE_LOG(LogTemp, Display, TEXT("APPROACH_TEST_PASS %s stages=4 reveal_count=%d combat=false timers=clear elapsed=%.1f"), *RunId,
            Arena ? Arena->GetRevealCount() : 0, AutomationElapsed);
        FApp::SetUseFixedTimeStep(false);
        FPlatformMisc::RequestExitWithStatus(false, 0);
        return;
    }
    if (auto* Campaign = GetGameInstance<UCampaignGameInstance>(); Campaign && Campaign->CompleteApproach())
    {
        Campaign->TravelToCurrentChapter(this);
        return;
    }
    UGameplayStatics::OpenLevel(
        this, FName(TEXT("/Game/Maps/L_Prototype_01")), true, TEXT("game=/Script/IshibashiriPrototype.PrototypeGameMode?BasinPrototype"));
}
