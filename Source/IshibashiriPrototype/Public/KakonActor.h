#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KakonActor.generated.h"

class AKakonActor;

UENUM(BlueprintType)
enum class EKakonState : uint8
{
    Covered,
    Exposed,
    Purified
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKakonPurifiedSignature, AKakonActor*, Kakon);

// One generic Kakon that must have its shell broken before it can be purified.
UCLASS(BlueprintType)
class ISHIBASHIRIPROTOTYPE_API AKakonActor : public AActor
{
    GENERATED_BODY()

public:
    AKakonActor();

    UFUNCTION(BlueprintCallable, Category="Kakon")
    void ApplyShellDamage(float Damage);

    UFUNCTION(BlueprintCallable, Category="Kakon")
    bool Purify();

    UFUNCTION(BlueprintCallable, Category="Kakon")
    void ResetKakon();

    UFUNCTION(BlueprintPure, Category="Kakon")
    EKakonState GetState() const { return State; }

    UFUNCTION(BlueprintPure, Category="Kakon")
    float GetCurrentShellHealth() const { return CurrentShellHealth; }

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Kakon", meta=(ClampMin="0"))
    float MaxShellHealth = 100.f;

    UPROPERTY(BlueprintAssignable, Category="Kakon")
    FKakonPurifiedSignature OnPurified;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Kakon", meta=(AllowPrivateAccess="true"))
    EKakonState State = EKakonState::Covered;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Kakon", meta=(AllowPrivateAccess="true"))
    float CurrentShellHealth = 100.f;
};
