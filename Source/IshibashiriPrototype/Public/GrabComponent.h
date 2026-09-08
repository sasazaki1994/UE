#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrabComponent.generated.h"

class ACharacter;

// Keeps its owning character at a transform relative to a moving actor.
// The relative transform is intentionally writable for a future climbing system.
UCLASS(ClassGroup=(Movement), meta=(BlueprintSpawnableComponent))
class ISHIBASHIRIPROTOTYPE_API UGrabComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGrabComponent();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    bool TryGrab(AActor* Candidate, float MaximumDistance);
    void Release();

    bool IsGrabbing() const { return bGrabbing; }
    AActor* GetGrabTarget() const { return GrabTarget; }
    const FVector& GetGrabStartWorldLocation() const { return GrabStartWorldLocation; }
    const FTransform& GetRelativeGrabTransform() const { return RelativeGrabTransform; }
    void SetRelativeGrabTransform(const FTransform& NewTransform) { RelativeGrabTransform = NewTransform; }

private:
    UPROPERTY() TObjectPtr<AActor> GrabTarget;
    FVector GrabStartWorldLocation = FVector::ZeroVector;
    FTransform RelativeGrabTransform = FTransform::Identity;
    bool bGrabbing = false;
};
