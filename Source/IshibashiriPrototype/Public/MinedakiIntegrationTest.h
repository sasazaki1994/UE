#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MinedakiIntegrationTest.generated.h"
class AMinedakiGameMode;
UCLASS()
class AMinedakiIntegrationTest : public AActor
{
    GENERATED_BODY()
public:
    AMinedakiIntegrationTest();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
private:
    void Hold(FKey Key,bool Down);
    void Tap(FKey Key);
    void Move(float Forward,float Right=0);
    void MoveToward(FVector Target);
    void Next(int32 NewPhase) { Phase=NewPhase; Time=0; }
    bool Check(bool Condition,const TCHAR* Message);
    void Shot(const TCHAR* Name);
    UPROPERTY() TObjectPtr<AMinedakiGameMode> Mode;
    TSet<FKey> Held;
    TArray<FKey> Releases;
    TSet<FString> Captures;
    FString RunId;
    FTransform WallRelative;
    int32 Phase=0, Rounds=0, InitialActors=0;
    float Time=0, Total=0, ForwardAxis=0, RightAxis=0, MaxFollowError=0, MaxAngleError=0, PeakPitch=0, PeakYaw=0, PeakRoll=0;
    bool bGamepad=false, bCapture=false, bDone=false, bSawShake=false;
    int32 RecoveryRuns=0;
};
