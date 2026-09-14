#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CampaignPlayerController.generated.h"
UCLASS()
class ISHIBASHIRIPROTOTYPE_API ACampaignPlayerController : public APlayerController
{
    GENERATED_BODY()
protected: virtual void SetupInputComponent() override;
private: void Confirm();
};
