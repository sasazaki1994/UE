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
    if (ActionState == EFuchimatoiActionState::Snagged)
    {
        CoilingProgress = 0.f;
    }
    TryTransition(EFuchimatoiActionState::Snagged, EFuchimatoiActionState::Coiling);
}

void AFuchimatoiBoss::ReturnToSubmerged()
{
    TryTransition(EFuchimatoiActionState::Coiling, EFuchimatoiActionState::Submerged);
}

void AFuchimatoiBoss::ResetFuchimatoi()
{
    ResetNushi();
    CoilingProgress = 0.f;
    SetActionState(EFuchimatoiActionState::Submerged);
    HeadProxyLocalLocation = FVector::ZeroVector;
    BiteTargetLocalLocation = FVector::ZeroVector;
    bHasBiteTarget = false;
}

void AFuchimatoiBoss::SetBiteTargetLocalLocation(const FVector& NewTargetLocalLocation)
{
    BiteTargetLocalLocation = NewTargetLocalLocation;
    bHasBiteTarget = true;
}

void AFuchimatoiBoss::AdvanceBiteLunge(float DeltaSeconds)
{
    if (ActionState != EFuchimatoiActionState::BiteLunge
        || !bHasBiteTarget
        || DeltaSeconds <= 0.f
        || BiteLungeSpeed <= 0.f
        || !FMath::IsFinite(DeltaSeconds)
        || !FMath::IsFinite(BiteLungeSpeed))
    {
        return;
    }

    HeadProxyLocalLocation = FMath::VInterpConstantTo(
        HeadProxyLocalLocation,
        BiteTargetLocalLocation,
        DeltaSeconds,
        BiteLungeSpeed);

    if (HeadProxyLocalLocation.Equals(BiteTargetLocalLocation, KINDA_SMALL_NUMBER))
    {
        HeadProxyLocalLocation = BiteTargetLocalLocation;
    }
}

void AFuchimatoiBoss::AdvanceCoiling(float DeltaSeconds)
{
    if (ActionState != EFuchimatoiActionState::Coiling
        || DeltaSeconds <= 0.f
        || CoilingDuration <= 0.f
        || !FMath::IsFinite(DeltaSeconds)
        || !FMath::IsFinite(CoilingDuration))
    {
        return;
    }

    // Divide in double precision so even extreme finite float inputs cannot overflow.
    const double ProgressDelta = static_cast<double>(DeltaSeconds) / CoilingDuration;
    CoilingProgress = static_cast<float>(FMath::Clamp(CoilingProgress + ProgressDelta, 0.0, 1.0));
}

bool AFuchimatoiBoss::IsBiteTargetReached() const
{
    return bHasBiteTarget
        && HeadProxyLocalLocation.Equals(BiteTargetLocalLocation, KINDA_SMALL_NUMBER);
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
