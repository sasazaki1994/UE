#include "NushiProgressComponent.h"
#include "KakonActor.h"

UNushiProgressComponent::UNushiProgressComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UNushiProgressComponent::RegisterKakon(AKakonActor* Kakon)
{
    if (!IsValid(Kakon) || RegisteredKakon.Contains(Kakon)) return;

    RegisteredKakon.Add(Kakon);
    Kakon->OnPurified.AddUniqueDynamic(this, &UNushiProgressComponent::HandleKakonPurified);

    if (Kakon->GetState() == EKakonState::Purified)
    {
        HandleKakonPurified(Kakon);
    }
}

void UNushiProgressComponent::HandleKakonPurified(AKakonActor* Kakon)
{
    if (bAllPurified || !RegisteredKakon.Contains(Kakon) || PurifiedKakon.Contains(Kakon)) return;

    PurifiedKakon.Add(Kakon);
    if (RegisteredKakon.Num() > 0 && PurifiedKakon.Num() == RegisteredKakon.Num())
    {
        bAllPurified = true;
        OnAllPurified.Broadcast();
    }
}

void UNushiProgressComponent::ResetProgress()
{
    PurifiedKakon.Reset();
    bAllPurified = false;

    for (AKakonActor* Kakon : RegisteredKakon)
    {
        if (IsValid(Kakon))
        {
            Kakon->ResetKakon();
        }
    }
}
