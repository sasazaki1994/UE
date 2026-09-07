#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IshibashiriBoss.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APrototypePlayer;

UENUM()
enum class EIshibashiriState : uint8 { Chase, Telegraph, Charge, Recover, Calmed };

UCLASS()
class ISHIBASHIRIPROTOTYPE_API AIshibashiriBoss : public AActor
{
    GENERATED_BODY()

public:
    AIshibashiriBoss();
    virtual void Tick(float DeltaSeconds) override;
    void ResetForEncounter(const FTransform& Spawn, APrototypePlayer* Player);
    bool TryReceiveCounter();
    int32 GetHealth() const { return Health; }
    EIshibashiriState GetState() const { return State; }
    float GetStateTimeRemaining() const { return StateTimeRemaining; }
    bool CanBeCountered() const { return State == EIshibashiriState::Recover && !bCounterUsed && Health > 0; }
    FString GetStateLabel() const;
    FVector GetChargeDirection() const { return ChargeDirection; }

    UPROPERTY(EditAnywhere, Category="Combat", meta=(ClampMin="1")) int32 MaxHealth = 3;
    UPROPERTY(EditAnywhere, Category="Combat", meta=(ClampMin="1")) float ChaseSpeed = 260.f;
    UPROPERTY(EditAnywhere, Category="Combat", meta=(ClampMin="0.1")) float ChaseDuration = 1.2f;
    UPROPERTY(EditAnywhere, Category="Combat", meta=(ClampMin="1")) float ChargeTriggerDistance = 1400.f;
    UPROPERTY(EditAnywhere, Category="Combat", meta=(ClampMin="0.01")) float TelegraphDuration = 1.f;
    UPROPERTY(EditAnywhere, Category="Combat", meta=(ClampMin="1")) float ChargeSpeed = 1500.f;
    UPROPERTY(EditAnywhere, Category="Combat", meta=(ClampMin="0.01")) float MaxChargeDuration = 1.35f;
    UPROPERTY(EditAnywhere, Category="Combat", meta=(ClampMin="0.01")) float RecoveryDuration = 1.65f;

protected:
    virtual void BeginPlay() override;

private:
    void EnterState(EIshibashiriState NewState);
    void UpdateVisuals();
    void CheckChargeHit(const FVector& Start, const FVector& End);

    UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> Collision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ColoredParts;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;
    UPROPERTY() TObjectPtr<APrototypePlayer> Target;
    EIshibashiriState State = EIshibashiriState::Chase;
    int32 Health = 3;
    float StateTimeRemaining = 0.f;
    float VisualTime = 0.f;
    bool bCounterUsed = false;
    bool bChargeHitPlayer = false;
    FVector ChargeDirection = FVector::ForwardVector;
};
