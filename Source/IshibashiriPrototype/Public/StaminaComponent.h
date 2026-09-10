#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StaminaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStaminaChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStaminaDepletedSignature);

// Owns only the bounded numeric stamina resource; callers decide when to consume or restore it.
UCLASS(ClassGroup=(Player), meta=(BlueprintSpawnableComponent))
class ISHIBASHIRIPROTOTYPE_API UStaminaComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStaminaComponent();

    UFUNCTION(BlueprintCallable, Category="Stamina")
    void ConsumeStamina(float Amount);

    UFUNCTION(BlueprintCallable, Category="Stamina")
    void RestoreStamina(float Amount);

    UFUNCTION(BlueprintCallable, Category="Stamina")
    void RefillStamina();

    UFUNCTION(BlueprintCallable, Category="Stamina")
    void ResetStamina();

    UFUNCTION(BlueprintCallable, Category="Stamina")
    void SetMaxStamina(float NewMaxStamina);

    UFUNCTION(BlueprintPure, Category="Stamina")
    float GetMaxStamina() const { return MaxStamina; }

    UFUNCTION(BlueprintPure, Category="Stamina")
    float GetCurrentStamina() const { return CurrentStamina; }

    UFUNCTION(BlueprintPure, Category="Stamina")
    bool IsDepleted() const { return CurrentStamina <= 0.0f; }

    UPROPERTY(BlueprintAssignable, Category="Stamina")
    FStaminaChangedSignature OnStaminaChanged;

    UPROPERTY(BlueprintAssignable, Category="Stamina")
    FStaminaDepletedSignature OnStaminaDepleted;

protected:
    virtual void PostInitProperties() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    void SetCurrentStamina(float NewStamina);
    void ClampConfiguration();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stamina", meta=(AllowPrivateAccess="true", ClampMin="0.0"))
    float MaxStamina = 100.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Stamina", meta=(AllowPrivateAccess="true"))
    float CurrentStamina = 100.0f;
};
