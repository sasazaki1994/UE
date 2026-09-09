#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PrototypePlayer.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UColossusClimbingComponent;
class UAnimSequence;
class UGrabComponent;

UCLASS()
class ISHIBASHIRIPROTOTYPE_API APrototypePlayer : public ACharacter
{
    GENERATED_BODY()

public:
    APrototypePlayer();
    virtual void Tick(float DeltaSeconds) override;
    virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    void ResetForEncounter(const FTransform& Spawn);
    void StopCombat();
    bool ReceiveChargeHit(const FVector& From);
    void Attack();
    void BeginGrab();
    void ReleaseGrab();
    bool IsGrabbing() const;
    bool HasImportedVisuals() const;
    UGrabComponent* GetGrabComponent() const { return GrabComponent; }
    void Dodge();
    UColossusClimbingComponent* GetClimbing() const { return Climbing; }

    int32 GetHealth() const { return Health; }
    bool IsDodging() const { return DodgeRemaining > 0.f; }
    bool IsInvulnerable() const { return IsDodging() || HurtInvulnerabilityRemaining > 0.f; }
    bool IsAttacking() const { return AttackRemaining > 0.f; }
    float GetDodgeCooldown() const { return DodgeCooldownRemaining; }
    bool IsUsingRaisedCamera() const { return bUsingRaisedCamera; }
    const FString& GetFeedback() const { return Feedback; }

    // Default encounter uses authored holds; disable for the original local-space Grab prototype.
    UPROPERTY(EditAnywhere, Category="Grab") bool bUseRouteClimbing = true;
    UPROPERTY(EditAnywhere, Category="Grab", meta=(ClampMin="1")) float GrabDistance = 360.f;
    UPROPERTY(EditAnywhere, Category="Camera|Gamepad", meta=(ClampMin="0")) float GamepadCameraYawSpeed = 120.f;
    UPROPERTY(EditAnywhere, Category="Camera|Gamepad", meta=(ClampMin="0")) float GamepadCameraPitchSpeed = 90.f;
    UPROPERTY(EditAnywhere, Category="Combat", meta=(ClampMin="1")) int32 MaxHealth = 3;
    UPROPERTY(EditAnywhere, Category="Movement", meta=(ClampMin="1")) float WalkSpeed = 600.f;
    UPROPERTY(EditAnywhere, Category="Combat|Dodge", meta=(ClampMin="1")) float DodgeSpeed = 1500.f;
    UPROPERTY(EditAnywhere, Category="Combat|Dodge", meta=(ClampMin="0.01")) float DodgeDuration = 0.28f;
    UPROPERTY(EditAnywhere, Category="Combat|Dodge", meta=(ClampMin="0")) float DodgeCooldown = 0.55f;
    UPROPERTY(EditAnywhere, Category="Combat", meta=(ClampMin="0")) float HurtInvulnerabilityDuration = 0.85f;
    UPROPERTY(EditAnywhere, Category="Combat|Attack", meta=(ClampMin="0.01")) float AttackDuration = 0.53f;
    UPROPERTY(EditAnywhere, Category="Combat|Attack", meta=(ClampMin="0")) float AttackCooldown = 0.48f;
    UPROPERTY(EditAnywhere, Category="Combat|Attack", meta=(ClampMin="1")) float AttackReach = 210.f;
    UPROPERTY(EditAnywhere, Category="Combat|Attack", meta=(ClampMin="1")) float AttackRadius = 85.f;
    UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="100")) float MinimumCameraDistance = 300.f;
    UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="500")) float RaisedCameraHeight = 720.f;

protected:
    virtual void BeginPlay() override;

private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void TryJump();
    void Retry();
    void TraceAttack();
    bool CanAct() const;
    void ShowFeedback(const FString& Text);
    void UpdateAnimation();
    void TurnRate(float Value);
    void LookUpRate(float Value);

    UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> SpringArm;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Sword;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UColossusClimbingComponent> Climbing;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UGrabComponent> GrabComponent;
    UPROPERTY() TArray<TObjectPtr<UAnimSequence>> Animations;
    int32 CurrentAnimation = INDEX_NONE;
    bool bWeaponHidden = false;
    int32 Health = 3;
    float ForwardInput = 0.f;
    float RightInput = 0.f;
    float DodgeRemaining = 0.f;
    float DodgeCooldownRemaining = 0.f;
    float AttackRemaining = 0.f;
    float AttackCooldownRemaining = 0.f;
    float HurtInvulnerabilityRemaining = 0.f;
    float FeedbackRemaining = 0.f;
    bool bAttackConnected = false;
    bool bUsingRaisedCamera = false;
    FVector DodgeDirection = FVector::ForwardVector;
    FVector AttackDirection = FVector::ForwardVector;
    FString Feedback;
};
