#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NushiStateComponent.generated.h"

class UNushiProgressComponent;

UENUM(BlueprintType)
enum class ENushiState : uint8
{
    Dormant,
    Active,
    Calm
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNushiStateChangedSignature);

// Tracks the encounter state of one Nushi without owning AI or boss behavior.
UCLASS(ClassGroup=(Nushi), meta=(BlueprintSpawnableComponent))
class ISHIBASHIRIPROTOTYPE_API UNushiStateComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNushiStateComponent();

    UFUNCTION(BlueprintCallable, Category="Nushi")
    void StartEncounter();

    UFUNCTION(BlueprintCallable, Category="Nushi")
    void ResetNushi();

    UFUNCTION(BlueprintPure, Category="Nushi")
    ENushiState GetNushiState() const { return State; }

    UFUNCTION(BlueprintPure, Category="Nushi")
    bool IsNushiActive() const { return State == ENushiState::Active; }

    UFUNCTION(BlueprintPure, Category="Nushi")
    bool IsCalm() const { return State == ENushiState::Calm; }

    UPROPERTY(BlueprintAssignable, Category="Nushi")
    FNushiStateChangedSignature OnNushiStateChanged;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void BindProgressComponent();
    void SetState(ENushiState NewState);

    UFUNCTION()
    void HandleAllKakonPurified();

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Nushi", meta=(AllowPrivateAccess="true"))
    ENushiState State = ENushiState::Dormant;

    UPROPERTY()
    TObjectPtr<UNushiProgressComponent> ProgressComponent;
};
