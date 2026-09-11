#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ColossusClimbingComponent.generated.h"

class APrototypePlayer;
class AIshibashiriBoss;

USTRUCT(BlueprintType)
struct FClimbingIKTargets
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere) FTransform LeftHand;
    UPROPERTY(VisibleAnywhere) FTransform RightHand;
    UPROPERTY(VisibleAnywhere) FTransform LeftFoot;
    UPROPERTY(VisibleAnywhere) FTransform RightFoot;
};

UCLASS()
class ISHIBASHIRIPROTOTYPE_API UColossusClimbingComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UColossusClimbingComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float Dt, ELevelTick Type, FActorComponentTickFunction* Tick) override;
    void GrabPressed();
    void GrabReleased();
    void Detach(bool bJump = true);
    void Reset();
    void SetInput(float Forward, float Right) { ForwardInput = Forward; RightInput = Right; }
    bool TryPurify();
    bool IsClimbing() const { return Boss != nullptr; }
    bool IsGrabWarping() const { return bGrabWarping; }
    bool IsMoving() const { return Destination != INDEX_NONE; }
    bool IsResting() const;
    bool IsGripping() const { return bGripHeld; }
    bool HasMovementInput() const { return !FMath::IsNearlyZero(ForwardInput) || !FMath::IsNearlyZero(RightInput); }
    float GetStamina() const { return Stamina; }
    int32 GetNode() const { return Node; }
    int32 GetDestination() const { return Destination; }
    float GetIKWeight() const { return IKWeight; }
    bool IsIKVerticalSlice() const;
    const FClimbingIKTargets& GetIKTargets() const { return IKTargets; }
    AIshibashiriBoss* GetBoss() const { return Boss; }
    FString GetHint() const;
    UPROPERTY(EditAnywhere, Category="Climbing") float ClimbSpeed = 190.f;
    UPROPERTY(EditAnywhere, Category="Climbing") float GrabRange = 240.f;
    UPROPERTY(EditAnywhere, Category="Climbing|Motion Warp", meta=(ClampMin="1")) float MaximumWarpDistance = 240.f;
    UPROPERTY(EditAnywhere, Category="Climbing|Motion Warp", meta=(ClampMin="1", ClampMax="179")) float MaximumWarpAngle = 100.f;
    UPROPERTY(EditAnywhere, Category="Climbing|Motion Warp", meta=(ClampMin="1")) float CompletionDistanceTolerance = 35.f;
    UPROPERTY(EditAnywhere, Category="Climbing|Motion Warp", meta=(ClampMin="1", ClampMax="90")) float CompletionAngleTolerance = 18.f;
    UPROPERTY(EditAnywhere, Category="Climbing|IK", meta=(ClampMin="0.01")) float IKBlendInSeconds = .22f;
    UPROPERTY(EditAnywhere, Category="Climbing|IK", meta=(ClampMin="0.01")) float IKBlendOutSeconds = .16f;
private:
    void UpdateIK(float Dt);
    bool StartGrabWarp(AIshibashiriBoss* Candidate);
    void UpdateGrabWarp(float Dt);
    void CancelGrabWarp(const TCHAR* Reason);
    void CompleteGrabWarp();
    FTransform MakeGrabWarpTarget(const AIshibashiriBoss* Candidate) const;
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
    bool bGrabWarping = false;
    float GrabWarpElapsed = 0.f;
    float GrabWarpDuration = 0.f;
    float GrabWarpStartDistance = 0.f;
    float GrabWarpStartAngle = 0.f;
    FVector GrabWarpStartLocation = FVector::ZeroVector;
    UPROPERTY() TObjectPtr<AIshibashiriBoss> GrabWarpBoss;
    float IKWeight = 0.f;
    FClimbingIKTargets IKTargets;
};
