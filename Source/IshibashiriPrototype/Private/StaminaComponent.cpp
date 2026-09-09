#include "StaminaComponent.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

UStaminaComponent::UStaminaComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UStaminaComponent::PostInitProperties()
{
    Super::PostInitProperties();
    ClampConfiguration();
}

#if WITH_EDITOR
void UStaminaComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    ClampConfiguration();
}
#endif

void UStaminaComponent::ConsumeStamina(float Amount)
{
    if (Amount < 0.0f)
    {
        return;
    }

    SetCurrentStamina(CurrentStamina - Amount);
}

void UStaminaComponent::RestoreStamina(float Amount)
{
    if (Amount < 0.0f)
    {
        return;
    }

    SetCurrentStamina(CurrentStamina + Amount);
}

void UStaminaComponent::RefillStamina()
{
    SetCurrentStamina(MaxStamina);
}

void UStaminaComponent::ResetStamina()
{
    RefillStamina();
}

void UStaminaComponent::SetMaxStamina(float NewMaxStamina)
{
    MaxStamina = FMath::Max(0.0f, NewMaxStamina);
    SetCurrentStamina(CurrentStamina);
}

void UStaminaComponent::SetCurrentStamina(float NewStamina)
{
    const float PreviousStamina = CurrentStamina;
    const float ClampedStamina = FMath::Clamp(NewStamina, 0.0f, MaxStamina);
    if (FMath::IsNearlyEqual(PreviousStamina, ClampedStamina))
    {
        return;
    }

    CurrentStamina = ClampedStamina;
    OnStaminaChanged.Broadcast();

    if (PreviousStamina > 0.0f && IsDepleted())
    {
        OnStaminaDepleted.Broadcast();
    }
}

void UStaminaComponent::ClampConfiguration()
{
    MaxStamina = FMath::Max(0.0f, MaxStamina);
    CurrentStamina = FMath::Clamp(CurrentStamina, 0.0f, MaxStamina);
}
