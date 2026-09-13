#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FuchimatoiRouteAnchor.generated.h"
class UStaticMeshComponent;

// A stable actor-space attachment target for the existing UGrabComponent.
UCLASS()
class ISHIBASHIRIPROTOTYPE_API AFuchimatoiRouteAnchor : public AActor
{
    GENERATED_BODY()
public:
    AFuchimatoiRouteAnchor();
    void Configure(bool bRock, int32 Index);
    bool IsRock() const { return bRockAnchor; }
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Marker;
    bool bRockAnchor = false;
};
