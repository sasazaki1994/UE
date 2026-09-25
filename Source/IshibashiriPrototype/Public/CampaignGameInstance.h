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
    IshibashiriApproach,
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

// Exactly two appearance stages are supported. Title/Completed return no stage
// rather than introducing a third appearance value.
UENUM(BlueprintType)
enum class EShirotsuraCorruptionStage : uint8
{
    Early,
    Advanced
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
    bool CompleteApproach();
    void RestartCampaign();
    bool ContinueCampaign();
    bool HasContinue() const { return bHasContinue; }
    ECampaignState GetContinueChapter() const { return ContinueChapter; }
    static bool IsResumableChapter(ECampaignState Chapter);

    bool IsCampaignActive() const { return bCampaignActive; }
    bool IsIshibashiriDemo() const { return bIshibashiriDemo; }
    bool IsPersistenceEnabled() const { return bPersistenceEnabled; }
    bool HasCompletedIshibashiriDemo() const { return bIshibashiriDemoCompleteLogged; }
    int32 GetLastCardIndex() const;

    ECampaignState GetCampaignState() const { return State; }

    static bool TryGetCorruptionStageForChapter(ECampaignState Chapter, EShirotsuraCorruptionStage& OutStage);
    bool GetCorruptionStageForEncounter(ECampaignState StandaloneEncounter, EShirotsuraCorruptionStage& OutStage) const;
    bool NotifyEncounterRetry(ECampaignState Encounter) const;

    bool IsCurrentEncounter(ECampaignState Encounter) const { return bCampaignActive && State == Encounter; }

    static bool IsEncounterState(ECampaignState Value);
    static void ResetSenseState(UPlayerSenseComponent* Sense);
    void TravelToCurrentChapter(UObject* WorldContext);
    double GetCampaignElapsedSeconds() const;

#if WITH_DEV_AUTOMATION_TESTS
    void SetCampaignStateForTest(ECampaignState Value)
    {
        bCampaignActive = true;
        State = Value;
    }
    void SetContinueForTest(ECampaignState Chapter)
    {
        bHasContinue = true;
        ContinueChapter = Chapter;
    }
    void SetIshibashiriDemoForTest()
    {
        bCampaignActive = true;
        bIshibashiriDemo = true;
        bPersistenceEnabled = false;
        State = ECampaignState::Title;
    }
#endif

private:
    void SaveChapter();
    void ClearChapterSave();
    void LoadChapterSave();
    void ResetChapterRuntime(UObject* WorldContext);
    UPROPERTY() ECampaignState State = ECampaignState::Title;
    bool bCampaignActive = false;
    bool bIshibashiriDemo = false;
    bool bPersistenceEnabled = false;
    bool bIshibashiriDemoCompleteLogged = false;
    bool bHasContinue = false;
    ECampaignState ContinueChapter = ECampaignState::Title;
    double CampaignStartSeconds = 0.0;
    double ChapterStartSeconds = 0.0;
};
