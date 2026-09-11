#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ColossusClimbingComponent.generated.h"

class APrototypePlayer;
class AIshibashiriBoss;

UCLASS()
class ISHIBASHIRIPROTOTYPE_API UColossusClimbingComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UColossusClimbingComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float Dt, ELevelTick Type, FActorComponentTickFunction* Tick) override;
    void GrabPressed();
    void GrabReleased() { bGripHeld = false; }
    void Detach(bool bJump = true);
    void Reset();
    void SetInput(float Forward, float Right) { ForwardInput = Forward; RightInput = Right; }
    bool TryPurify();
    bool IsClimbing() const { return Boss != nullptr; }
    bool IsMoving() const { return Destination != INDEX_NONE; }
    bool IsResting() const;
    bool IsGripping() const { return bGripHeld; }
    bool HasMovementInput() const { return !FMath::IsNearlyZero(ForwardInput) || !FMath::IsNearlyZero(RightInput); }
    float GetStamina() const { return Stamina; }
    int32 GetNode() const { return Node; }
    AIshibashiriBoss* GetBoss() const { return Boss; }
    FString GetHint() const;
    UPROPERTY(EditAnywhere, Category="Climbing") float ClimbSpeed = 190.f;
    UPROPERTY(EditAnywhere, Category="Climbing") float GrabRange = 240.f;
private:
    UPROPERTY() TObjectPtr<APrototypePlayer> Player;
    UPROPERTY() TObjectPtr<AIshibashiriBoss> Boss;
    int32 Node = INDEX_NONE;
    int32 Destination = INDEX_NONE;
    float Progress = 0.f;
    float Stamina = 100.f;
    float ForwardInput = 0.f;
    float RightInput = 0.f;
    float InputDelay = 0.f;
    float UnsafeBuckTime = 0.f;
    bool bGripHeld = false;
};
