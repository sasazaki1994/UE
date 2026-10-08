#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/InputComponent.h"
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
    enum class EPhase : uint8 { Approach, Mount, Climb, Rest, Detach, Land, GroundMove, GroundDodge, Retry, RetryApproach, Done };
    void Hold(const FKey& Key, bool bDown);
    void Tap(const FKey& Key);
    float AimAt(const FVector& Destination);
    void AimAndMove(const FVector& Destination, float StopDistance = 0.f);
    bool DriveApproach(float DeltaSeconds);
    bool IsOnBasinFloor(FString* Detail = nullptr) const;
    bool ValidateRetryReset(FString& Detail) const;
    void HandleRetryInput(FKey Key);
    void RestoreRetryInput();
    void RestoreExecutionSettings();
    void Finish(int32 ExitCode);
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
    // Each approach includes three full charge/recovery counters before Grab.
    float PhaseTimeout = 60.f;
    float CombatStateTime = 0.f;
    float AttackWait = 0.f;
    int32 LastBossState = INDEX_NONE;
    FKey DodgeSide;
    float TotalTime = 0.f;
    float StaminaAtLedge = 0.f;
    FVector SpawnLocation = FVector::ZeroVector;
    FTransform PlayerStartTransform;
    FTransform BossStartTransform;
    FVector GroundMoveStart = FVector::ZeroVector;
    FString RunId;
    FInputActionUnifiedDelegate OriginalRetryDelegate;
    int32 RetryBindingHandle = INDEX_NONE;
    bool bAwaitingRetry = false;
    bool bRetryObserved = false;
    bool bCapture = false;
    bool bFinished = false;
    bool bDodgedThisCharge = false;
    bool bReachedForelegApproach = false;
    bool bChargeShot = false;
    bool bChargeEffectsShot = false;
    bool bStartShot = false;
    bool bSawFalling = false;
    bool bSawGroundMovement = false;
    bool bPreviousFixedTimeStep = false;
    double PreviousFixedDeltaTime = 0.0;
    int32 InitialStaticMeshes = 0;
    int32 InitialLights = 0;
    int32 InitialFogs = 0;
    int32 InitialRocks = 0;
    int32 InitialBoundaries = 0;
    int32 InitialAccents = 0;
};
