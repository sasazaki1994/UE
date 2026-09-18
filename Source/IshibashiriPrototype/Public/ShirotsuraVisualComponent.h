#pragma once

#include "Components/ActorComponent.h"
#include "ShirotsuraVisualComponent.generated.h"

class UAnimSequence;
class UPrimitiveComponent;
class USkeletalMeshComponent;

UENUM()
enum class EShirotsuraVisualState : uint8
{
    Idle,
    Walk,
    Run,
    Slash,
    Dodge,
    Climb,
    Hang,
    Grip,
    Jump,
    Death,
    Count
};

// Presentation-only adapter shared by all encounter pawns. Gameplay remains the
// authority for state changes; animation never drives movement or actions.
UCLASS(ClassGroup = (Presentation), meta = (BlueprintSpawnableComponent))

class ISHIBASHIRIPROTOTYPE_API UShirotsuraVisualComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UShirotsuraVisualComponent();
    void Configure(USkeletalMeshComponent* InMesh, UPrimitiveComponent* InFallback, UPrimitiveComponent* InFallbackWeapon = nullptr,
        UPrimitiveComponent* InAdditionalFallback = nullptr);
    void SetState(EShirotsuraVisualState State, float PlayRate = 1.f);
    void PlayOneShot(EShirotsuraVisualState State, float MinimumDuration = .35f);
    void ResetPresentation();
    void SetWeaponHidden(bool bHidden);

    bool IsUsingRig() const { return bUsingRig; }

    USkeletalMeshComponent* GetVisualMesh() const { return Mesh; }

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    void RefreshFallbackVisibility();
    void ApplyState(EShirotsuraVisualState State, float PlayRate);
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Mesh;
    UPROPERTY() TObjectPtr<UPrimitiveComponent> Fallback;
    UPROPERTY() TObjectPtr<UPrimitiveComponent> FallbackWeapon;
    UPROPERTY() TObjectPtr<UPrimitiveComponent> AdditionalFallback;
    UPROPERTY() TArray<TObjectPtr<UAnimSequence>> Animations;
    int32 CurrentState = INDEX_NONE;
    float OneShotRemaining = 0.f;
    bool bUsingRig = false;
    bool bWeaponHidden = false;
};
