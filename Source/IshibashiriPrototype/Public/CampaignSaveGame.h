#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CampaignGameInstance.h"
#include "CampaignSaveGame.generated.h"

// A chapter boundary is persistent; the encounter is reset on resume.
UCLASS()
class ISHIBASHIRIPROTOTYPE_API UCampaignSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    static constexpr int32 CurrentVersion = 1;
    UPROPERTY(SaveGame) int32 Version = 1;
    UPROPERTY(SaveGame) ECampaignState Chapter = ECampaignState::Prologue;
};
