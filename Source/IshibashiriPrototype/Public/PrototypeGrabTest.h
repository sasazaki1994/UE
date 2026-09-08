#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "PrototypeGrabTest.generated.h"

class APrototypeGameMode;

UCLASS(NotBlueprintable, Transient)
class APrototypeGrabTest : public AActor
{
    GENERATED_BODY()

public:
    APrototypeGrabTest();
    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;

private:
    enum class EPhase : uint8 { Press, FollowTranslation, FollowRotation, Release, Regrab, Retry, Done };
    bool Require(bool Condition, const TCHAR* Message);
    void SendKey(const FKey& Key, EInputEvent Event);
    void Next(EPhase NewPhase);
    void Finish(bool Success);

    UPROPERTY() TObjectPtr<APrototypeGameMode> Mode;
    EPhase Phase = EPhase::Press;
    float Elapsed = 0.f;
    float TotalElapsed = 0.f;
    FTransform ExpectedRelative = FTransform::Identity;
    FVector RetryLocation = FVector::ZeroVector;
    FString RunId;
};
