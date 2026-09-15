#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "IshibashiriApproachHUD.generated.h"
UCLASS()
class ISHIBASHIRIPROTOTYPE_API AIshibashiriApproachHUD : public AHUD
{
    GENERATED_BODY()
public: virtual void DrawHUD() override;
};
