#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FuchimatoiGameMode.generated.h"
class AFuchimatoiBoss;
class AFuchimatoiPlayer;
class AFuchimatoiArena;
class ANushiEncounterManager;
UCLASS()
class ISHIBASHIRIPROTOTYPE_API AFuchimatoiGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AFuchimatoiGameMode();
    virtual void StartPlay() override;
    void RetryEncounter();
    void Defeat();
    bool IsEncounterActive() const;
    bool IsVictory() const;
    AFuchimatoiBoss* GetBoss() const { return Boss; }
    AFuchimatoiPlayer* GetPlayer() const { return Player; }
    AFuchimatoiArena* GetArena() const { return Arena; }
    ANushiEncounterManager* GetEncounterManager() const { return Manager; }
private:
    UFUNCTION() void HandleCompleted();
    UPROPERTY() TObjectPtr<AFuchimatoiBoss> Boss;
    UPROPERTY() TObjectPtr<AFuchimatoiPlayer> Player;
    UPROPERTY() TObjectPtr<AFuchimatoiArena> Arena;
    UPROPERTY() TObjectPtr<ANushiEncounterManager> Manager;
    bool bDefeated=false;
};
