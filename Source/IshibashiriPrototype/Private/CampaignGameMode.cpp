#include "CampaignGameMode.h"
#include "CampaignGameInstance.h"
#include "CampaignHUD.h"
#include "CampaignPlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ACampaignGameMode::ACampaignGameMode()
{
    DefaultPawnClass = nullptr;
    HUDClass = ACampaignHUD::StaticClass();
    PlayerControllerClass = ACampaignPlayerController::StaticClass();
}

void ACampaignGameMode::StartPlay()
{
    Super::StartPlay();
    if (auto* Campaign = GetGameInstance<UCampaignGameInstance>())
    {
        if (!Campaign->IsCampaignActive() && (FParse::Param(FCommandLine::Get(), TEXT("Campaign"))
            || FParse::Param(FCommandLine::Get(), TEXT("IshibashiriDemo")))) Campaign->RestartCampaign();
    }
}

void ACampaignGameMode::ConfirmCard()
{
    UCampaignGameInstance* Campaign = GetGameInstance<UCampaignGameInstance>();
    if (!Campaign) return;
    const ECampaignState State = Campaign->GetCampaignState();
    if (State == ECampaignState::Title && !NewGameConfirmation.RequestStart(
        Campaign->IsPersistenceEnabled() && Campaign->HasContinue())) { return; }
    const int32 LastCard = Campaign->GetLastCardIndex();
    if (State == ECampaignState::Title || State == ECampaignState::Completed || CardIndex >= LastCard)
    {
        if (Campaign->AdvanceCardChapter()) Campaign->TravelToCurrentChapter(this);
        return;
    }
    ++CardIndex;
}

void ACampaignGameMode::ContinueSavedCampaign()
{
    UCampaignGameInstance* Campaign = GetGameInstance<UCampaignGameInstance>();
    if (Campaign && Campaign->ContinueCampaign())
    {
        NewGameConfirmation.Cancel();
        Campaign->TravelToCurrentChapter(this);
    }
}

void ACampaignGameMode::CancelNewGameConfirmation() { NewGameConfirmation.Cancel(); }
