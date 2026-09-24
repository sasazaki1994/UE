#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CampaignGameMode.generated.h"

UCLASS()
class ISHIBASHIRIPROTOTYPE_API ACampaignGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ACampaignGameMode();
    virtual void StartPlay() override;
    void ConfirmCard();
    void ContinueSavedCampaign();
    int32 GetCardIndex() const { return CardIndex; }
private:
    int32 CardIndex=0;
};
