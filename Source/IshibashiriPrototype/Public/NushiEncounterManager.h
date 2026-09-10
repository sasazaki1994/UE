#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NushiEncounterManager.generated.h"

class ANushiBase;

UENUM(BlueprintType)
enum class ENushiEncounterState : uint8
{
    Idle,
    Running,
    Completed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNushiEncounterStartedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNushiEncounterCompletedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNushiEncounterResetSignature);

// Coordinates the lifecycle of one Nushi encounter without owning gameplay systems.
UCLASS(Blueprintable)
class ISHIBASHIRIPROTOTYPE_API ANushiEncounterManager : public AActor
{
    GENERATED_BODY()

public:
    ANushiEncounterManager();

    UFUNCTION(BlueprintCallable, Category="Nushi|Encounter")
    void SetNushi(ANushiBase* InNushi);

    UFUNCTION(BlueprintPure, Category="Nushi|Encounter")
    ANushiBase* GetNushi() const { return Nushi; }

    UFUNCTION(BlueprintCallable, Category="Nushi|Encounter")
    void StartEncounter();

    UFUNCTION(BlueprintCallable, Category="Nushi|Encounter")
    void ResetEncounter();

    UFUNCTION(BlueprintPure, Category="Nushi|Encounter")
    ENushiEncounterState GetEncounterState() const { return EncounterState; }

    UPROPERTY(BlueprintAssignable, Category="Nushi|Encounter")
    FNushiEncounterStartedSignature OnEncounterStarted;

    UPROPERTY(BlueprintAssignable, Category="Nushi|Encounter")
    FNushiEncounterCompletedSignature OnEncounterCompleted;

    UPROPERTY(BlueprintAssignable, Category="Nushi|Encounter")
    FNushiEncounterResetSignature OnEncounterReset;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void UnbindNushi();

    UFUNCTION()
    void HandleNushiStateChanged();

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Nushi|Encounter", meta=(AllowPrivateAccess="true"))
    TObjectPtr<ANushiBase> Nushi;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Nushi|Encounter", meta=(AllowPrivateAccess="true"))
    ENushiEncounterState EncounterState = ENushiEncounterState::Idle;
};
