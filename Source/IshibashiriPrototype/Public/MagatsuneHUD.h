#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MagatsuneHUD.generated.h"
UCLASS()

class ISHIBASHIRIPROTOTYPE_API AMagatsuneHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;

private:
    TWeakObjectPtr<class AMagatsuneBoss> ObservedBoss;
    int32 PreviousPurifiedCount = 0;
    float PurificationNoticeRemaining = 0.f;
    float StaminaTailRemaining = 0.f;
};
