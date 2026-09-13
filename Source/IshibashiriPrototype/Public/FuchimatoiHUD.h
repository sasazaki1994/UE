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
};
