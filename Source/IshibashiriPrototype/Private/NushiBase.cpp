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
    PurificationPresentation = 0.f;
    CalmPresentation = 0.f;
}

ENushiState ANushiBase::GetNushiState() const
{
    return NushiStateComponent->GetNushiState();
}

void ANushiBase::NotifyPurificationPresentation()
{
    PurificationPresentation = 1.f;
}

void ANushiBase::AdvancePresentation(float DeltaSeconds)
{
    if (DeltaSeconds <= 0.f || !FMath::IsFinite(DeltaSeconds)) return;
    PurificationPresentation = FMath::Max(0.f, PurificationPresentation - DeltaSeconds / .45f);
    const float CalmTarget = GetNushiState() == ENushiState::Calm ? 1.f : 0.f;
    CalmPresentation = FMath::FInterpTo(CalmPresentation, CalmTarget, DeltaSeconds, 1.8f);
}
