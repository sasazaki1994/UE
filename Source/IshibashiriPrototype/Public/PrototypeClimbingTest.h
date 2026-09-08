#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "PrototypeClimbingTest.generated.h"

class APrototypeGameMode;

UCLASS(NotBlueprintable, Transient)
class APrototypeClimbingTest : public AActor
{
    GENERATED_BODY()

public:
    APrototypeClimbingTest();
    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;

private:
    enum class EPhase : uint8 { Grab, Up, Down, Left, Right, Clamp, RotatedUp, BossAI, Release, Regrab, Retry, Done };
    bool Require(bool Condition, const TCHAR* Message);
    void SendKey(const FKey& Key, EInputEvent Event);
    void Next(EPhase NewPhase);
    void Finish(bool Success);

    UPROPERTY() TObjectPtr<APrototypeGameMode> Mode;
    EPhase Phase = EPhase::Grab;
    float Elapsed = 0.f;
    float TotalElapsed = 0.f;
    FTransform Baseline = FTransform::Identity;
    FVector BossAIStart = FVector::ZeroVector;
    FVector ReleasedRelativeLocation = FVector::ZeroVector;
    FString RunId;
};
