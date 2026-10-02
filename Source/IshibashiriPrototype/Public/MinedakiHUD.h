#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MinedakiHUD.generated.h"
UCLASS()

class ISHIBASHIRIPROTOTYPE_API AMinedakiHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;

private:
    TWeakObjectPtr<class AMinedakiBoss> ObservedBoss;
    int32 PreviousPurifiedCount = 0;
    float PurificationNoticeRemaining = 0.f;
    float StaminaTailRemaining = 0.f;
};
