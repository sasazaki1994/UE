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
};
