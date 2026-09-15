#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "IshibashiriApproachGameMode.generated.h"

class AIshibashiriApproachArena;
class APrototypePlayer;
class AActor;

UCLASS()
class ISHIBASHIRIPROTOTYPE_API AIshibashiriApproachGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AIshibashiriApproachGameMode();
    virtual void StartPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    AIshibashiriApproachArena* GetApproachArena() const{return Arena;}
    APrototypePlayer* GetPlayer() const{return Player;}
private:
    void EnterBasin();
    UPROPERTY() TObjectPtr<AIshibashiriApproachArena> Arena;
    UPROPERTY() TObjectPtr<APrototypePlayer> Player;
    UPROPERTY() TObjectPtr<AActor> SenseTarget;
    bool bTransitioning=false;
    bool bAutomationDriving=false;
    int32 AutomationPoint=1;
};
