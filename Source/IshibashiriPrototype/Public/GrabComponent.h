#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrabComponent.generated.h"

class ACharacter;

// Keeps its owning character at a transform relative to a moving actor and
// applies the prototype's small local-space climbing offset while held.
UCLASS(ClassGroup=(Movement), meta=(BlueprintSpawnableComponent))
class ISHIBASHIRIPROTOTYPE_API UGrabComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGrabComponent();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    bool TryGrab(AActor* Candidate, float MaximumDistance);
    void Release();
    void Climb(float UpInput, float RightInput, float DeltaSeconds);

    bool IsGrabbing() const { return bGrabbing; }
    AActor* GetGrabTarget() const { return GrabTarget; }
    const FVector& GetGrabStartWorldLocation() const { return GrabStartWorldLocation; }
    const FTransform& GetRelativeGrabTransform() const { return RelativeGrabTransform; }
    void SetRelativeGrabTransform(const FTransform& NewTransform) { RelativeGrabTransform = NewTransform; }

    // Unreal actor local +Z is up and local +Y is right. X is not changed by
    // climbing yet, but is clamped as a guard against externally supplied offsets.
    UPROPERTY(EditAnywhere, Category="Climbing", meta=(ClampMin="0")) float ClimbSpeed = 180.f;
    UPROPERTY(EditAnywhere, Category="Climbing") FVector MinimumClimbLocation = FVector(-650.f, -350.f, -350.f);
    UPROPERTY(EditAnywhere, Category="Climbing") FVector MaximumClimbLocation = FVector(650.f, 350.f, 650.f);

private:
    UPROPERTY() TObjectPtr<AActor> GrabTarget;
    FVector GrabStartWorldLocation = FVector::ZeroVector;
    FTransform RelativeGrabTransform = FTransform::Identity;
    bool bGrabbing = false;
};
