#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IshibashiriBoss.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APrototypePlayer;
class USkeletalMeshComponent;
class UAnimSequence;
class UBoxComponent;

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
    bool HasImportedVisuals() const;
    FVector GetClimbPosition(int32 Node) const;
    int32 GetClimbNeighbor(int32 Node, int32 Direction) const;
    bool IsRestNode(int32 Node) const { return Node >= 3; }
    bool TryPurifyCore(const FVector& Position);
    bool IsBucking() const;
    bool IsBuckWarning() const;
    int32 GetPurifiedCount() const;
    USkeletalMeshComponent* GetVisualMesh() const { return Creature; }

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
    void UpdateCreatureAnimation();

    UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> Collision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ColoredParts;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;
    UPROPERTY() TObjectPtr<APrototypePlayer> Target;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> Creature;
    UPROPERTY() TArray<TObjectPtr<UAnimSequence>> CreatureAnimations;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> CoreMarkers;
    UPROPERTY() TArray<TObjectPtr<UBoxComponent>> LedgeCollision;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> GrabMarker;
    bool PurifiedCores[3] = {false,false,false};
    float RiderTime = 0.f;
    int32 AnimationIndex = INDEX_NONE;
    EIshibashiriState State = EIshibashiriState::Chase;
    int32 Health = 3;
    float StateTimeRemaining = 0.f;
    float VisualTime = 0.f;
    bool bCounterUsed = false;
    bool bChargeHitPlayer = false;
    FVector ChargeDirection = FVector::ForwardVector;
};
