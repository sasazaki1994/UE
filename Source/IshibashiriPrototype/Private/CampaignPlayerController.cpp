#include "CampaignPlayerController.h"
#include "CampaignGameMode.h"
#include "CampaignGameInstance.h"
#include "InputKeyEventArgs.h"
#include "InputCoreTypes.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

ACampaignPlayerController::ACampaignPlayerController() { PrimaryActorTick.bCanEverTick = true; }

void ACampaignPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction(TEXT("Attack"), IE_Pressed, this, &ACampaignPlayerController::Confirm);
    InputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACampaignPlayerController::Confirm);
    InputComponent->BindAction(TEXT("Retry"), IE_Pressed, this, &ACampaignPlayerController::ContinueSaved);
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ACampaignPlayerController::CancelNewGameConfirmation);
    InputComponent->BindKey(EKeys::Gamepad_FaceButton_Right, IE_Pressed, this, &ACampaignPlayerController::CancelNewGameConfirmation);
}

void ACampaignPlayerController::Confirm()
{
    if (auto* Mode = GetWorld()->GetAuthGameMode<ACampaignGameMode>()) Mode->ConfirmCard();
}

void ACampaignPlayerController::ContinueSaved()
{
    if (auto* Mode = GetWorld()->GetAuthGameMode<ACampaignGameMode>()) Mode->ContinueSavedCampaign();
}

void ACampaignPlayerController::CancelNewGameConfirmation()
{
    if (auto* Mode = GetWorld()->GetAuthGameMode<ACampaignGameMode>()) Mode->CancelNewGameConfirmation();
}

void ACampaignPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    static const bool bCampaignE2E = FParse::Param(FCommandLine::Get(), TEXT("CampaignE2E"));
    static const bool bCapture = FParse::Param(FCommandLine::Get(), TEXT("PrototypeCapture"));
    static const bool bGamepad = FParse::Param(FCommandLine::Get(), TEXT("CampaignGamepad"));
    if (!bCampaignE2E) return;
    FString RunId;
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestRun="), RunId);
    AutoConfirmSeconds += DeltaTime;
    const auto* Campaign = GetGameInstance<UCampaignGameInstance>();
    if (Campaign && Campaign->GetCampaignState() == ECampaignState::Completed)
    {
        if (!bCompleted && bCapture)
        {
            const FString Dir = FPaths::ProjectSavedDir() / TEXT("Screenshots/Campaign") / RunId;
            IFileManager::Get().MakeDirectory(*Dir, true);
            FScreenshotRequest::RequestScreenshot(Dir / TEXT("15-Completed.png"), false, false);
            bCompleted = true;
            AutoConfirmSeconds = 0.f;
            return;
        }
        if (bCompleted && AutoConfirmSeconds < .5f) return;
        UE_LOG(LogTemp, Display, TEXT("CAMPAIGN_E2E_PASS %s total_campaign_time=%.3f"), *RunId, Campaign->GetCampaignElapsedSeconds());
        FPlatformMisc::RequestExitWithStatus(false, 0);
        return;
    }
    if (AutoConfirmSeconds < .35f) return;
    AutoConfirmSeconds = 0.f;
    if (Campaign && bCapture)
    {
        const TCHAR* Name = nullptr;
        switch (Campaign->GetCampaignState())
        {
        case ECampaignState::Title: Name = TEXT("01-Title.png"); break;
        case ECampaignState::Prologue: Name = TEXT("02-Prologue.png"); break;
        case ECampaignState::Interlude1: Name = TEXT("05-Interlude1.png"); break;
        case ECampaignState::Interlude2: Name = TEXT("08-Interlude2.png"); break;
        case ECampaignState::Interlude3: Name = TEXT("11-Interlude3.png"); break;
        case ECampaignState::Ending: Name = TEXT("14-Ending.png"); break;
        default: break;
        }
        if (Name)
        {
            const FString Dir = FPaths::ProjectSavedDir() / TEXT("Screenshots/Campaign") / RunId;
            IFileManager::Get().MakeDirectory(*Dir, true);
            FScreenshotRequest::RequestScreenshot(Dir / Name, false, false);
        }
    }
    const FKey Key = bGamepad ? EKeys::Gamepad_FaceButton_Left : EKeys::LeftMouseButton;
    InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.f));
    InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.f));
}
