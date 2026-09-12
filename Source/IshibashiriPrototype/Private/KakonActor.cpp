#include "KakonActor.h"
#include "Components/SceneComponent.h"

AKakonActor::AKakonActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("KakonRoot")));
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
    CurrentShellHealth = FMath::Max(0.f, MaxShellHealth);
    State = CurrentShellHealth > 0.f ? EKakonState::Covered : EKakonState::Exposed;
}
