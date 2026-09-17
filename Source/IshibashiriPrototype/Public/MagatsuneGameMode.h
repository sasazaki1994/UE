#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MagatsuneGameMode.generated.h"
class AMagatsuneBoss;
class AMagatsunePlayer;
class ANushiEncounterManager;
UCLASS()
class ISHIBASHIRIPROTOTYPE_API AMagatsuneGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AMagatsuneGameMode();
    virtual void StartPlay() override;
    void RetryEncounter();

    AMagatsuneBoss* GetBoss() const { return Boss; }

    AMagatsunePlayer* GetPlayer() const { return Player; }

    ANushiEncounterManager* GetManager() const { return Manager; }

private:
    UFUNCTION() void HandleCompleted();
    void AdvanceCampaign();
    UPROPERTY() TObjectPtr<AMagatsuneBoss> Boss;
    UPROPERTY() TObjectPtr<AMagatsunePlayer> Player;
    UPROPERTY() TObjectPtr<ANushiEncounterManager> Manager;
    FTimerHandle CampaignAdvanceTimer;
};
