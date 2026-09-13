#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FuchimatoiIntegrationTest.generated.h"
class AFuchimatoiGameMode;
UCLASS()
class ISHIBASHIRIPROTOTYPE_API AFuchimatoiIntegrationTest : public AActor
{
    GENERATED_BODY()
public:
    AFuchimatoiIntegrationTest();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
private:
    void Hold(const FKey& Key,bool Down);
    void Tap(const FKey& Key);
    void SetMove(float Forward,float Right);
    void MoveToward(FVector WorldTarget);
    void Next(int32 NewPhase);
    void Shot(const TCHAR* Name);
    bool Check(bool Condition,const TCHAR* Message);
    void Finish();
    UPROPERTY() TObjectPtr<AFuchimatoiGameMode> Mode;
    TSet<FKey> Held;
    TArray<FKey> Releases;
    FString RunId;
    int32 Phase=0, Rounds=0, InitialActors=0;
    float Time=0, Total=0, ForwardAxis=0, RightAxis=0, BeforeStamina=0;
    FVector LockedTarget, BeforeAnchor, RockBefore;
    bool bCapture=false, bGamepad=false, bDone=false, bSawDodge=false, bSawCoilFollow=false;
};
