#include "NushiStateComponent.h"
#include "GameFramework/Actor.h"
#include "NushiProgressComponent.h"

UNushiStateComponent::UNushiStateComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UNushiStateComponent::BeginPlay()
{
    Super::BeginPlay();
    BindProgressComponent();
}

void UNushiStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ProgressComponent)
    {
        ProgressComponent->OnAllPurified.RemoveDynamic(this, &UNushiStateComponent::HandleAllKakonPurified);
    }

    Super::EndPlay(EndPlayReason);
}

void UNushiStateComponent::BindProgressComponent()
{
    if (!ProgressComponent && GetOwner())
    {
        ProgressComponent = GetOwner()->FindComponentByClass<UNushiProgressComponent>();
    }

    if (ProgressComponent)
    {
        ProgressComponent->OnAllPurified.AddUniqueDynamic(this, &UNushiStateComponent::HandleAllKakonPurified);
    }
}

void UNushiStateComponent::StartEncounter()
{
    BindProgressComponent();
    if (State == ENushiState::Dormant)
    {
        SetState(ENushiState::Active);
    }
}

void UNushiStateComponent::ResetNushi()
{
    BindProgressComponent();
    if (ProgressComponent)
    {
        ProgressComponent->ResetProgress();
    }

    SetState(ENushiState::Dormant);
}

void UNushiStateComponent::HandleAllKakonPurified()
{
    CalmNushi();
}

void UNushiStateComponent::CalmNushi()
{
    if (State == ENushiState::Active)
    {
        SetState(ENushiState::Calm);
    }
}

void UNushiStateComponent::SetState(ENushiState NewState)
{
    if (State == NewState) return;

    State = NewState;
    OnNushiStateChanged.Broadcast();
}
