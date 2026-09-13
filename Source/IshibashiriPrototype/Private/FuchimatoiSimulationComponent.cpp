#include "FuchimatoiSimulationComponent.h"
#include "FuchimatoiBoss.h"
UFuchimatoiSimulationComponent::UFuchimatoiSimulationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
}
void UFuchimatoiSimulationComponent::TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* Function)
{
    Super::TickComponent(Dt,TickType,Function);
    if (AFuchimatoiBoss* Boss = Cast<AFuchimatoiBoss>(GetOwner())) Boss->AdvanceEncounter(Dt);
}
