#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NushiStateComponent.h"
#include "NushiBase.generated.h"

class AKakonActor;
class UNushiProgressComponent;

// Minimal common actor shared by every Nushi implementation.
UCLASS(Blueprintable)
class ISHIBASHIRIPROTOTYPE_API ANushiBase : public AActor
{
    GENERATED_BODY()

public:
    ANushiBase();

    UFUNCTION(BlueprintPure, Category="Nushi")
    UNushiProgressComponent* GetNushiProgressComponent() const { return NushiProgressComponent; }

    UFUNCTION(BlueprintPure, Category="Nushi")
    UNushiStateComponent* GetNushiStateComponent() const { return NushiStateComponent; }

    UFUNCTION(BlueprintCallable, Category="Nushi")
    void RegisterKakon(AKakonActor* Kakon);

    UFUNCTION(BlueprintCallable, Category="Nushi")
    void StartEncounter();

    UFUNCTION(BlueprintCallable, Category="Nushi")
    virtual void ResetNushi();

    UFUNCTION(BlueprintPure, Category="Nushi")
    ENushiState GetNushiState() const;

    // Small, gameplay-neutral presentation envelope shared by every Nushi.
    // Encounter implementations decide how to render it; progression never waits for it.
    void NotifyPurificationPresentation();
    void AdvancePresentation(float DeltaSeconds);
    float GetPurificationPresentation() const { return PurificationPresentation; }
    float GetCalmPresentation() const { return CalmPresentation; }

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Nushi", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UNushiProgressComponent> NushiProgressComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Nushi", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UNushiStateComponent> NushiStateComponent;

    float PurificationPresentation = 0.f;
    float CalmPresentation = 0.f;
};
