#include "CampaignPlayerController.h"
#include "CampaignGameMode.h"
void ACampaignPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction(TEXT("Attack"),IE_Pressed,this,&ACampaignPlayerController::Confirm);
    InputComponent->BindAction(TEXT("Jump"),IE_Pressed,this,&ACampaignPlayerController::Confirm);
}
void ACampaignPlayerController::Confirm(){if(auto* Mode=GetWorld()->GetAuthGameMode<ACampaignGameMode>())Mode->ConfirmCard();}
