#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MagatsuneIntegrationTest.generated.h"
class AMagatsuneGameMode;
UCLASS() class AMagatsuneIntegrationTest : public AActor
{
    GENERATED_BODY()
public: AMagatsuneIntegrationTest(); virtual void BeginPlay() override; virtual void Tick(float Dt) override;
private: void Hold(FKey K,bool Down);void Tap(FKey K);void Move(float V);void MoveToward(FVector P);void Next(int32 P){Phase=P;Time=0;}void Shot(const TCHAR* N);bool Fail(const TCHAR* Why);
    UPROPERTY() TObjectPtr<AMagatsuneGameMode> Mode;TSet<FKey> Held;TArray<FKey> Releases;TSet<FString> Captures;FString RunId;int32 Phase=0,Round=0,InitialActors=0;float Time=0,Total=0,Axis=0,MaxFollowError=0;bool bGamepad=false,bCapture=false,bRecoveryDone=false,bDone=false;
};
