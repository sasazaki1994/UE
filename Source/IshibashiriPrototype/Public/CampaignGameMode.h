#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CampaignGameMode.generated.h"

// Small non-persistent policy object so Title confirmation behavior can be
// exercised without a World or a save slot in automation.
struct FCampaignNewGameConfirmation
{
    bool RequestStart(bool bHasContinue)
    {
        if (bHasContinue && !bPending)
        {
            bPending = true;
            return false;
        }
        return true;
    }

    void Cancel() { bPending = false; }

    bool IsPending() const { return bPending; }

private:
    bool bPending = false;
};

UCLASS()

class ISHIBASHIRIPROTOTYPE_API ACampaignGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ACampaignGameMode();
    virtual void StartPlay() override;
    void ConfirmCard();
    void ContinueSavedCampaign();
    void CancelNewGameConfirmation();

    int32 GetCardIndex() const { return CardIndex; }

    bool IsNewGameConfirmationPending() const { return NewGameConfirmation.IsPending(); }

private:
    int32 CardIndex = 0;
    // Title-world-only UI state; never serialized as campaign progress.
    FCampaignNewGameConfirmation NewGameConfirmation;
};
