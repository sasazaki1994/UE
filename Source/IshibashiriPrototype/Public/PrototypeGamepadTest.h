#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "PrototypeGamepadTest.generated.h"

class APrototypeGameMode;

UCLASS(NotBlueprintable, Transient)
class APrototypeGamepadTest : public AActor
{
    GENERATED_BODY()

public:
    APrototypeGamepadTest();
    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;

private:
    enum class EPhase : uint8
    {
        DeadZone, MoveForward, MoveRight, CameraYaw, CameraPitchUp, CameraPitchDown, Jump, WaitForLanding, Dodge, Attack,
        Grab, ClimbQuarter, ClimbHalf, ClimbFull, BossFollow, Release, RetryGrab, Retry, Done
    };
    bool Require(bool Condition, const TCHAR* Message);
    void SendKey(const FKey& Key, EInputEvent Event, float Value = 1.f);
    void SendAxis(const FKey& Key, float Value);
    void Next(EPhase NewPhase);
    void Finish(bool Success);

    UPROPERTY() TObjectPtr<APrototypeGameMode> Mode;
    EPhase Phase = EPhase::DeadZone;
    float Elapsed = 0.f;
    float TotalElapsed = 0.f;
    FVector BaselineLocation = FVector::ZeroVector;
    FVector BossStart = FVector::ZeroVector;
    FRotator BaselineRotation = FRotator::ZeroRotator;
    FTransform BaselineGrab = FTransform::Identity;
    float QuarterDistance = 0.f;
    float HalfDistance = 0.f;
    FString RunId;
};
