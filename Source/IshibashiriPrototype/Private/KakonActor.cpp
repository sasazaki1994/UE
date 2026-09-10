#include "KakonActor.h"

AKakonActor::AKakonActor()
{
    PrimaryActorTick.bCanEverTick = false;
    CurrentShellHealth = MaxShellHealth;
}

void AKakonActor::BeginPlay()
{
    Super::BeginPlay();
    ResetKakon();
}

void AKakonActor::ApplyShellDamage(float Damage)
{
    if (State != EKakonState::Covered || Damage <= 0.f) return;

    CurrentShellHealth = FMath::Max(0.f, CurrentShellHealth - Damage);
    if (CurrentShellHealth <= 0.f)
    {
        State = EKakonState::Exposed;
    }
}

bool AKakonActor::Purify()
{
    if (State != EKakonState::Exposed) return false;

    State = EKakonState::Purified;
    OnPurified.Broadcast(this);
    return true;
}

void AKakonActor::ResetKakon()
{
    State = EKakonState::Covered;
    CurrentShellHealth = FMath::Max(0.f, MaxShellHealth);
}
