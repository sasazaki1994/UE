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
        if (!Campaign->IsCampaignActive() && FParse::Param(FCommandLine::Get(), TEXT("Campaign"))) Campaign->RestartCampaign();
    }
}

void ACampaignGameMode::ConfirmCard()
{
    UCampaignGameInstance* Campaign = GetGameInstance<UCampaignGameInstance>();
    if (!Campaign) return;
    const ECampaignState State = Campaign->GetCampaignState();
    const int32 LastCard = State == ECampaignState::Prologue ? 3
        : State == ECampaignState::Ending                    ? 3
                                          : (State == ECampaignState::Interlude1 || State == ECampaignState::Interlude3 ? 2 : 1);
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
    if (Campaign && Campaign->ContinueCampaign()) Campaign->TravelToCurrentChapter(this);
}
