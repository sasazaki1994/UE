#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CampaignGameInstance.generated.h"

class UPlayerSenseComponent;

UENUM()
enum class ECampaignState : uint8
{
    Title,
    Prologue,
    Ishibashiri,
    Interlude1,
    Fuchimatoi,
    Interlude2,
    Minedaki,
    Interlude3,
    Magatsune,
    Ending,
    Completed
};

// Process-local chapter state only. Encounter progress remains owned by each Nushi.
UCLASS()
class ISHIBASHIRIPROTOTYPE_API UCampaignGameInstance : public UGameInstance
{
    GENERATED_BODY()
public:
    virtual void Init() override;
    void StartCampaign();
    bool AdvanceCardChapter();
    bool CompleteEncounter(ECampaignState Encounter);
    void RestartCampaign();
    bool IsCampaignActive() const { return bCampaignActive; }
    ECampaignState GetCampaignState() const { return State; }
    bool IsCurrentEncounter(ECampaignState Encounter) const { return bCampaignActive && State == Encounter; }
    static bool IsEncounterState(ECampaignState Value);
    static void ResetSenseState(UPlayerSenseComponent* Sense);
    void TravelToCurrentChapter(UObject* WorldContext);
    double GetCampaignElapsedSeconds() const;

#if WITH_DEV_AUTOMATION_TESTS
    void SetCampaignStateForTest(ECampaignState Value) { bCampaignActive=true; State=Value; }
#endif

private:
    void ResetChapterRuntime(UObject* WorldContext);
    UPROPERTY() ECampaignState State=ECampaignState::Title;
    bool bCampaignActive=false;
    double CampaignStartSeconds=0.0;
    double ChapterStartSeconds=0.0;
};
