#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FuchimatoiSimulationComponent.generated.h"

// Opt-in runtime driver; AFuchimatoiBoss keeps its explicit, tick-free action API.
UCLASS()
class ISHIBASHIRIPROTOTYPE_API UFuchimatoiSimulationComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UFuchimatoiSimulationComponent();
    virtual void TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* Function) override;
};
