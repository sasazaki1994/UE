#pragma once

#include "CoreMinimal.h"
#include "NushiBase.h"
#include "FuchimatoiBoss.generated.h"

class AFuchimatoiArena;
class AFuchimatoiPlayer;
class AFuchimatoiRouteAnchor;
class UFuchimatoiSimulationComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EFuchimatoiActionState : uint8
{
    Submerged,
    BiteWindup,
    BiteLunge,
    Snagged,
    Coiling
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFuchimatoiActionStateChangedSignature);

// Minimal action state machine specific to Fuchimatoi, the serpent Nushi.
UCLASS(Blueprintable)
class ISHIBASHIRIPROTOTYPE_API AFuchimatoiBoss : public ANushiBase
{
    GENERATED_BODY()

public:
    AFuchimatoiBoss();
    virtual void ResetNushi() override;
    void ConfigureEncounter(AFuchimatoiArena* Arena, AFuchimatoiPlayer* Player);
    void AdvanceEncounter(float Dt);
    AFuchimatoiRouteAnchor* GetRouteAnchor(int32 Node) const;
    AKakonActor* GetKakon(int32 Index) const;
    FVector GetHeadWorldLocation() const;
    bool CanMount() const;
    bool TryPurifyAtNode(int32 Node);
    FString GetActionLabel() const;
    float GetActionTimeRemaining() const { return ActionTimeRemaining; }
    float GetBodyLength() const;
    static constexpr int32 RouteNodeCount = 10;

    UPROPERTY(EditAnywhere, Category="Nushi|Fuchimatoi|Bite") float WindupDuration = 1.25f;
    UPROPERTY(EditAnywhere, Category="Nushi|Fuchimatoi|Bite") float SubmergedDuration = 1.5f;
    UPROPERTY(EditAnywhere, Category="Nushi|Fuchimatoi|Bite") float SnaggedDuration = 4.5f;

    UFUNCTION(BlueprintCallable, Category="Nushi|Fuchimatoi")
    void BeginBiteWindup();

    UFUNCTION(BlueprintCallable, Category="Nushi|Fuchimatoi")
    void BeginBiteLunge();

    UFUNCTION(BlueprintCallable, Category="Nushi|Fuchimatoi")
    void NotifyHeadSnagged();

    UFUNCTION(BlueprintCallable, Category="Nushi|Fuchimatoi")
    void BeginCoiling();

    UFUNCTION(BlueprintCallable, Category="Nushi|Fuchimatoi")
    void ReturnToSubmerged();

    UFUNCTION(BlueprintCallable, Category="Nushi|Fuchimatoi")
    void ResetFuchimatoi();

    UFUNCTION(BlueprintCallable, Category="Nushi|Fuchimatoi|Bite")
    void SetBiteTargetLocalLocation(const FVector& NewTargetLocalLocation);

    UFUNCTION(BlueprintCallable, Category="Nushi|Fuchimatoi|Bite")
    void AdvanceBiteLunge(float DeltaSeconds);

    // Explicit advancement only; invalid delta time or duration leaves progress unchanged.
    UFUNCTION(BlueprintCallable, Category="Nushi|Fuchimatoi|Coiling")
    void AdvanceCoiling(float DeltaSeconds);

    UFUNCTION(BlueprintPure, Category="Nushi|Fuchimatoi|Coiling")
    float GetCoilingProgress() const { return CoilingProgress; }

    // Completion does not change ActionState; retained until the next coiling start or reset.
    UFUNCTION(BlueprintPure, Category="Nushi|Fuchimatoi|Coiling")
    bool IsCoilingComplete() const { return CoilingProgress >= 1.f; }

    UFUNCTION(BlueprintPure, Category="Nushi|Fuchimatoi|Bite")
    FVector GetHeadProxyLocalLocation() const { return HeadProxyLocalLocation; }

    UFUNCTION(BlueprintPure, Category="Nushi|Fuchimatoi|Bite")
    FVector GetBiteTargetLocalLocation() const { return BiteTargetLocalLocation; }

    UFUNCTION(BlueprintPure, Category="Nushi|Fuchimatoi|Bite")
    bool HasBiteTarget() const { return bHasBiteTarget; }

    UFUNCTION(BlueprintPure, Category="Nushi|Fuchimatoi|Bite")
    bool IsBiteTargetReached() const;

    UFUNCTION(BlueprintPure, Category="Nushi|Fuchimatoi")
    EFuchimatoiActionState GetActionState() const { return ActionState; }

    UFUNCTION(BlueprintPure, Category="Nushi|Fuchimatoi")
    bool IsSnagged() const { return ActionState == EFuchimatoiActionState::Snagged; }

    UFUNCTION(BlueprintPure, Category="Nushi|Fuchimatoi")
    bool IsCoiling() const { return ActionState == EFuchimatoiActionState::Coiling; }

    UPROPERTY(BlueprintAssignable, Category="Nushi|Fuchimatoi")
    FFuchimatoiActionStateChangedSignature OnFuchimatoiActionStateChanged;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nushi|Fuchimatoi|Bite", meta=(ClampMin="0.0"))
    float BiteLungeSpeed = 1200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nushi|Fuchimatoi|Coiling", meta=(ClampMin="0.0", Units="s"))
    float CoilingDuration = 2.f;

private:
    void CreatePrimitiveBody();
    void UpdateBody();
    void WithdrawHead();
    UFUNCTION() void HandleKakonPurified(AKakonActor* Kakon);

    UPROPERTY() TObjectPtr<UFuchimatoiSimulationComponent> Simulation;
    UPROPERTY() TObjectPtr<AFuchimatoiArena> Arena;
    UPROPERTY() TObjectPtr<AFuchimatoiPlayer> Player;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Head;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> BodySegments;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> BodyJoints;
    UPROPERTY() TArray<TObjectPtr<AFuchimatoiRouteAnchor>> RouteAnchors;
    UPROPERTY() TArray<TObjectPtr<AKakonActor>> KakonActors;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> KakonMarkers;
    FTransform EncounterSpawn;
    float ActionTimeRemaining = 0.f;
    bool bPlayable = false;

    void TryTransition(EFuchimatoiActionState ExpectedState, EFuchimatoiActionState NewState);
    void SetActionState(EFuchimatoiActionState NewState);

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Nushi|Fuchimatoi|Coiling", meta=(AllowPrivateAccess="true"))
    float CoilingProgress = 0.f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Nushi|Fuchimatoi", meta=(AllowPrivateAccess="true"))
    EFuchimatoiActionState ActionState = EFuchimatoiActionState::Submerged;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Nushi|Fuchimatoi|Bite", meta=(AllowPrivateAccess="true"))
    FVector HeadProxyLocalLocation = FVector::ZeroVector;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Nushi|Fuchimatoi|Bite", meta=(AllowPrivateAccess="true"))
    FVector BiteTargetLocalLocation = FVector::ZeroVector;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Nushi|Fuchimatoi|Bite", meta=(AllowPrivateAccess="true"))
    bool bHasBiteTarget = false;
};
