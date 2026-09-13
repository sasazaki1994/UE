#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MinedakiGameMode.generated.h"
class AMinedakiBoss;
class AMinedakiPlayer;
class ANushiEncounterManager;
UCLASS()
class ISHIBASHIRIPROTOTYPE_API AMinedakiGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AMinedakiGameMode();
    virtual void StartPlay() override;
    void RetryEncounter();
    AMinedakiBoss* GetBoss() const { return Boss; }
    AMinedakiPlayer* GetPlayer() const { return Player; }
    ANushiEncounterManager* GetManager() const { return Manager; }
private:
    UPROPERTY() TObjectPtr<AMinedakiBoss> Boss;
    UPROPERTY() TObjectPtr<AMinedakiPlayer> Player;
    UPROPERTY() TObjectPtr<ANushiEncounterManager> Manager;
};
