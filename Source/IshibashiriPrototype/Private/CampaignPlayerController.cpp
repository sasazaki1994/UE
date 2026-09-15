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
ACampaignPlayerController::ACampaignPlayerController(){PrimaryActorTick.bCanEverTick=true;}
void ACampaignPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction(TEXT("Attack"),IE_Pressed,this,&ACampaignPlayerController::Confirm);
    InputComponent->BindAction(TEXT("Jump"),IE_Pressed,this,&ACampaignPlayerController::Confirm);
}
void ACampaignPlayerController::Confirm(){if(auto* Mode=GetWorld()->GetAuthGameMode<ACampaignGameMode>())Mode->ConfirmCard();}
void ACampaignPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    if(!FParse::Param(FCommandLine::Get(),TEXT("CampaignE2E")))return;
    AutoConfirmSeconds+=DeltaTime;
    const auto* Campaign=GetGameInstance<UCampaignGameInstance>();
    if(Campaign&&Campaign->GetCampaignState()==ECampaignState::Completed)
    {
        if(!bCompleted&&FParse::Param(FCommandLine::Get(),TEXT("PrototypeCapture")))
        {
            FString RunId;FParse::Value(FCommandLine::Get(),TEXT("PrototypeTestRun="),RunId);
            const FString Dir=FPaths::ProjectSavedDir()/TEXT("Screenshots/Campaign")/RunId;IFileManager::Get().MakeDirectory(*Dir,true);
            FScreenshotRequest::RequestScreenshot(Dir/TEXT("15-Completed.png"),false,false);
            bCompleted=true;AutoConfirmSeconds=0.f;return;
        }
        if(bCompleted&&AutoConfirmSeconds<.5f)return;
        UE_LOG(LogTemp,Display,TEXT("CAMPAIGN_E2E_PASS total_campaign_time=%.3f"),Campaign->GetCampaignElapsedSeconds());
        FPlatformMisc::RequestExitWithStatus(false,0);
        return;
    }
    if(AutoConfirmSeconds<.35f)return;
    AutoConfirmSeconds=0.f;
    if(Campaign&&FParse::Param(FCommandLine::Get(),TEXT("PrototypeCapture")))
    {
        const TCHAR* Name=nullptr;
        switch(Campaign->GetCampaignState())
        {
        case ECampaignState::Title:Name=TEXT("01-Title.png");break;case ECampaignState::Prologue:Name=TEXT("02-Prologue.png");break;
        case ECampaignState::Interlude1:Name=TEXT("05-Interlude1.png");break;case ECampaignState::Interlude2:Name=TEXT("08-Interlude2.png");break;
        case ECampaignState::Interlude3:Name=TEXT("11-Interlude3.png");break;case ECampaignState::Ending:Name=TEXT("14-Ending.png");break;default:break;
        }
        if(Name){FString RunId;FParse::Value(FCommandLine::Get(),TEXT("PrototypeTestRun="),RunId);const FString Dir=FPaths::ProjectSavedDir()/TEXT("Screenshots/Campaign")/RunId;IFileManager::Get().MakeDirectory(*Dir,true);FScreenshotRequest::RequestScreenshot(Dir/Name,false,false);}
    }
    const FKey Key=FParse::Param(FCommandLine::Get(),TEXT("CampaignGamepad"))?EKeys::Gamepad_FaceButton_Left:EKeys::LeftMouseButton;
    InputKey(FInputKeyEventArgs::CreateSimulated(Key,IE_Pressed,1.f));
    InputKey(FInputKeyEventArgs::CreateSimulated(Key,IE_Released,0.f));
}
