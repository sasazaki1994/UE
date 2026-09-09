#include "NushiEncounterManager.h"

#include "NushiBase.h"
#include "NushiStateComponent.h"

ANushiEncounterManager::ANushiEncounterManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ANushiEncounterManager::SetNushi(ANushiBase* InNushi)
{
    if (Nushi == InNushi)
    {
        return;
    }

    UnbindNushi();
    Nushi = InNushi;

    if (IsValid(Nushi) && Nushi->GetNushiStateComponent())
    {
        Nushi->GetNushiStateComponent()->OnNushiStateChanged.AddUniqueDynamic(
            this, &ANushiEncounterManager::HandleNushiStateChanged);
    }
}

void ANushiEncounterManager::StartEncounter()
{
    if (EncounterState != ENushiEncounterState::Idle || !IsValid(Nushi))
    {
        return;
    }

    Nushi->StartEncounter();
    EncounterState = ENushiEncounterState::Running;
    OnEncounterStarted.Broadcast();
}

void ANushiEncounterManager::ResetEncounter()
{
    const bool bEncounterStateChanged = EncounterState != ENushiEncounterState::Idle;

    if (IsValid(Nushi))
    {
        Nushi->ResetNushi();
    }

    EncounterState = ENushiEncounterState::Idle;
    if (bEncounterStateChanged)
    {
        OnEncounterReset.Broadcast();
    }
}

void ANushiEncounterManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnbindNushi();
    Super::EndPlay(EndPlayReason);
}

void ANushiEncounterManager::UnbindNushi()
{
    if (IsValid(Nushi) && Nushi->GetNushiStateComponent())
    {
        Nushi->GetNushiStateComponent()->OnNushiStateChanged.RemoveDynamic(
            this, &ANushiEncounterManager::HandleNushiStateChanged);
    }
}

void ANushiEncounterManager::HandleNushiStateChanged()
{
    if (EncounterState != ENushiEncounterState::Running || !IsValid(Nushi) ||
        Nushi->GetNushiState() != ENushiState::Calm)
    {
        return;
    }

    EncounterState = ENushiEncounterState::Completed;
    OnEncounterCompleted.Broadcast();
}
