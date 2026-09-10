#include "NushiBase.h"

#include "NushiProgressComponent.h"

ANushiBase::ANushiBase()
{
    PrimaryActorTick.bCanEverTick = false;

    NushiProgressComponent = CreateDefaultSubobject<UNushiProgressComponent>(TEXT("NushiProgress"));
    NushiStateComponent = CreateDefaultSubobject<UNushiStateComponent>(TEXT("NushiState"));
}

void ANushiBase::RegisterKakon(AKakonActor* Kakon)
{
    NushiProgressComponent->RegisterKakon(Kakon);
}

void ANushiBase::StartEncounter()
{
    NushiStateComponent->StartEncounter();
}

void ANushiBase::ResetNushi()
{
    NushiStateComponent->ResetNushi();
}

ENushiState ANushiBase::GetNushiState() const
{
    return NushiStateComponent->GetNushiState();
}
