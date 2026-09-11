#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BasinPlaythroughTest.generated.h"

class APrototypeGameMode;

/** Input-driven traversal of the basin's shortest playable climbing loop. */
UCLASS(NotBlueprintable)
class ISHIBASHIRIPROTOTYPE_API ABasinPlaythroughTest : public AActor
{
    GENERATED_BODY()
public:
    ABasinPlaythroughTest();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float DeltaSeconds) override;

private:
    enum class EPhase : uint8 { Approach, Mount, Climb, Rest, Detach, Land, GroundMove, Retry, RetryApproach, Done };
    void Hold(const FKey& Key, bool bDown);
    void Tap(const FKey& Key);
    void AimAndMove(const FVector& Destination);
    void Next(EPhase NewPhase, float Timeout);
    void ReleaseAll();
    void Fail(const TCHAR* Reason);
    void Shot(const TCHAR* Name);
    FString Diagnostic() const;

    UPROPERTY() TObjectPtr<APrototypeGameMode> Mode;
    TSet<FKey> Held;
    TArray<FKey> PendingRelease;
    EPhase Phase = EPhase::Approach;
    float PhaseTime = 0.f;
    float PhaseTimeout = 20.f;
    float TotalTime = 0.f;
    float StaminaAtLedge = 0.f;
    FVector SpawnLocation = FVector::ZeroVector;
    FVector GroundMoveStart = FVector::ZeroVector;
    FString RunId;
    bool bCapture = false;
    bool bFinished = false;
    bool bDodgedThisThreat = false;
};
