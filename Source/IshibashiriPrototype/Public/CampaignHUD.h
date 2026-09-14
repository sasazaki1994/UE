#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CampaignHUD.generated.h"
UCLASS()
class ISHIBASHIRIPROTOTYPE_API ACampaignHUD : public AHUD
{
    GENERATED_BODY()
public: virtual void DrawHUD() override;
};
