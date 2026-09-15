#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CampaignPlayerController.generated.h"
UCLASS()
class ISHIBASHIRIPROTOTYPE_API ACampaignPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    ACampaignPlayerController();
    virtual void PlayerTick(float DeltaTime) override;
protected:
    virtual void SetupInputComponent() override;
private:
    void Confirm();
    float AutoConfirmSeconds=0.f;
    bool bCompleted=false;
};
