#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FuchimatoiHUD.generated.h"
UCLASS()

class ISHIBASHIRIPROTOTYPE_API AFuchimatoiHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;

private:
    TWeakObjectPtr<class AFuchimatoiBoss> ObservedBoss;
    int32 PreviousPurifiedCount = 0;
    float PurificationNoticeRemaining = 0.f;
    float StaminaTailRemaining = 0.f;
};
