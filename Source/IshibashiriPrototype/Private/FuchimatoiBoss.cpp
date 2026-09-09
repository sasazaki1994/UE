#include "FuchimatoiBoss.h"

AFuchimatoiBoss::AFuchimatoiBoss()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AFuchimatoiBoss::BeginBiteWindup()
{
    TryTransition(EFuchimatoiActionState::Submerged, EFuchimatoiActionState::BiteWindup);
}

void AFuchimatoiBoss::BeginBiteLunge()
{
    TryTransition(EFuchimatoiActionState::BiteWindup, EFuchimatoiActionState::BiteLunge);
}

void AFuchimatoiBoss::NotifyHeadSnagged()
{
    TryTransition(EFuchimatoiActionState::BiteLunge, EFuchimatoiActionState::Snagged);
}

void AFuchimatoiBoss::BeginCoiling()
{
    TryTransition(EFuchimatoiActionState::Snagged, EFuchimatoiActionState::Coiling);
}

void AFuchimatoiBoss::ReturnToSubmerged()
{
    TryTransition(EFuchimatoiActionState::Coiling, EFuchimatoiActionState::Submerged);
}

void AFuchimatoiBoss::ResetFuchimatoi()
{
    ResetNushi();
    SetActionState(EFuchimatoiActionState::Submerged);
}

void AFuchimatoiBoss::TryTransition(
    EFuchimatoiActionState ExpectedState,
    EFuchimatoiActionState NewState)
{
    if (ActionState == ExpectedState)
    {
        SetActionState(NewState);
    }
}

void AFuchimatoiBoss::SetActionState(EFuchimatoiActionState NewState)
{
    if (ActionState == NewState)
    {
        return;
    }

    ActionState = NewState;
    OnFuchimatoiActionStateChanged.Broadcast();
}
