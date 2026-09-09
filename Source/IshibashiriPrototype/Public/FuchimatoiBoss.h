#pragma once

#include "CoreMinimal.h"
#include "NushiBase.h"
#include "FuchimatoiBoss.generated.h"

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

    UFUNCTION(BlueprintPure, Category="Nushi|Fuchimatoi")
    EFuchimatoiActionState GetActionState() const { return ActionState; }

    UFUNCTION(BlueprintPure, Category="Nushi|Fuchimatoi")
    bool IsSnagged() const { return ActionState == EFuchimatoiActionState::Snagged; }

    UFUNCTION(BlueprintPure, Category="Nushi|Fuchimatoi")
    bool IsCoiling() const { return ActionState == EFuchimatoiActionState::Coiling; }

    UPROPERTY(BlueprintAssignable, Category="Nushi|Fuchimatoi")
    FFuchimatoiActionStateChangedSignature OnFuchimatoiActionStateChanged;

private:
    void TryTransition(EFuchimatoiActionState ExpectedState, EFuchimatoiActionState NewState);
    void SetActionState(EFuchimatoiActionState NewState);

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Nushi|Fuchimatoi", meta=(AllowPrivateAccess="true"))
    EFuchimatoiActionState ActionState = EFuchimatoiActionState::Submerged;
};
