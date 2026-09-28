#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PrototypeHUD.generated.h"

UENUM(BlueprintType)
enum class EHUDButtonPromptMode : uint8 { Off, Contextual, Always };

UCLASS()
class ISHIBASHIRIPROTOTYPE_API APrototypeHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

    // Presentation-only seams for the HUD specification. They deliberately do
    // not participate in encounter state, input eligibility, or persistence.
    UPROPERTY(EditAnywhere, Category="HUD|Accessibility") EHUDButtonPromptMode ButtonPrompts = EHUDButtonPromptMode::Contextual;
    UPROPERTY(EditAnywhere, Category="HUD|Accessibility", meta=(ClampMin="0.75", ClampMax="1.50")) float UserInterfaceScale = 1.f;
    UPROPERTY(EditAnywhere, Category="HUD|Accessibility") bool bShowDangerText = false;

private:
    TWeakObjectPtr<class AIshibashiriBoss> ObservedBoss;
    int32 PreviousPurifiedCount = 0;
    int32 PreviousPosture = 0;
    float PurificationNoticeRemaining = 0.f;
    float FullStaminaTailRemaining = 0.f;
    bool bPreviousBuckWarning = false;
    bool bSawDodge = false;
    bool bSawCounter = false;
    bool bSawGrab = false;
    bool bSawCling = false;
    bool bSawPurify = false;
};
