#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "PrototypePlaythroughTest.generated.h"

class APrototypeGameMode;

// Input-only acceptance test. Never teleports actors or calls combat/reset methods.
UCLASS(NotBlueprintable, Transient)
class ISHIBASHIRIPROTOTYPE_API APrototypePlaythroughTest : public AActor
{
    GENERATED_BODY()
public:
    APrototypePlaythroughTest();
    virtual void Tick(float DeltaSeconds) override;
protected:
    virtual void BeginPlay() override;
private:
    void SendKey(const FKey& Key, EInputEvent Event, float Amount = 1.f);
    void HoldKey(const FKey& Key, bool bDown);
    void TapKey(const FKey& Key);
    void ReleaseMovement();
    float AimAt(const FVector& Position);
    void WalkTo(const FVector& Position, float StopDistance);
    bool Require(bool Condition, const TCHAR* Message);
    void Finish(bool bSuccess);

    UPROPERTY() TObjectPtr<APrototypeGameMode> Mode;
    TSet<FKey> HeldKeys;
    TArray<FKey> PendingReleases;
    FKey SideKey = EKeys::D;
    FString RunId;
    float TotalSeconds = 0.f;
    float EncounterSeconds = 0.f;
    float StateSeconds = 0.f;
    float VictorySeconds = 0.f;
    float AttackWait = 0.f;
    float LastAttackWindow = 0.f;
    int32 MinimumSeconds = 0;
    int32 CompletedEncounters = 0;
    int32 LastState = -1;
    int32 LastBossHealth = 3;
    int32 RoundDodges = 0;
    int32 RoundCounters = 0;
    int32 InitialActorCount = 0;
    bool bDodgedThisCharge = false;
    bool bRestarting = false;
    bool bCapture = false;
    bool bCaptured = false;
    bool bDone = false;
};
