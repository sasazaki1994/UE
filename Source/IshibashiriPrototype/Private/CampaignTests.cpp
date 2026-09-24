#include "CampaignGameInstance.h"
#include "CampaignSaveGame.h"
#include "CampaignGameMode.h"
#include "PlayerSenseComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCampaignOrder, "IshibashiriPrototype.Campaign.CampaignOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCampaignOrder::RunTest(const FString&)
{
    auto* C = NewObject<UCampaignGameInstance>();
    C->RestartCampaign();
    C->AdvanceCardChapter();
    TestEqual(TEXT("Prologue first"), C->GetCampaignState(), ECampaignState::Prologue);
    C->AdvanceCardChapter();
    TestEqual(TEXT("Approach"), C->GetCampaignState(), ECampaignState::IshibashiriApproach);
    TestTrue(TEXT("Approach gate advances"), C->CompleteApproach());
    TestEqual(TEXT("Ishibashiri"), C->GetCampaignState(), ECampaignState::Ishibashiri);
    for (ECampaignState E : {ECampaignState::Ishibashiri, ECampaignState::Fuchimatoi, ECampaignState::Minedaki, ECampaignState::Magatsune})
    {
        TestTrue(TEXT("matching completion advances"), C->CompleteEncounter(E));
        if (E != ECampaignState::Magatsune) TestTrue(TEXT("interlude advances"), C->AdvanceCardChapter());
    }
    TestEqual(TEXT("Ending last"), C->GetCampaignState(), ECampaignState::Ending);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompletionAdvance, "IshibashiriPrototype.Campaign.EncounterCompletionAdvance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompletionAdvance::RunTest(const FString&)
{
    auto* C = NewObject<UCampaignGameInstance>();
    C->SetCampaignStateForTest(ECampaignState::Ishibashiri);
    TestFalse(TEXT("wrong completion rejected"), C->CompleteEncounter(ECampaignState::Fuchimatoi));
    TestEqual(TEXT("chapter retained"), C->GetCampaignState(), ECampaignState::Ishibashiri);
    TestTrue(TEXT("actual completion accepted"), C->CompleteEncounter(ECampaignState::Ishibashiri));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRetryNoAdvance, "IshibashiriPrototype.Campaign.RetryDoesNotAdvance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRetryNoAdvance::RunTest(const FString&)
{
    auto* C = NewObject<UCampaignGameInstance>();
    C->SetCampaignStateForTest(ECampaignState::Minedaki);
    EShirotsuraCorruptionStage Before;
    EShirotsuraCorruptionStage After;
    TestTrue(TEXT("Minedaki retry enters the shared reset hook"), C->GetCorruptionStageForEncounter(ECampaignState::Minedaki, Before));
    TestTrue(TEXT("actual retry hook accepts the current encounter"), C->NotifyEncounterRetry(ECampaignState::Minedaki));
    TestTrue(TEXT("stage remains available after retry"), C->GetCorruptionStageForEncounter(ECampaignState::Minedaki, After));
    TestEqual(TEXT("retry owns no campaign transition"), C->GetCampaignState(), ECampaignState::Minedaki);
    TestEqual(TEXT("retry retains corruption stage"), After, Before);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCorruptionStages, "IshibashiriPrototype.Campaign.CorruptionStages",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCorruptionStages::RunTest(const FString&)
{
    auto* C = NewObject<UCampaignGameInstance>();
    EShirotsuraCorruptionStage Stage = EShirotsuraCorruptionStage::Advanced;
    TestFalse(TEXT("Title has no appearance stage"), UCampaignGameInstance::TryGetCorruptionStageForChapter(ECampaignState::Title, Stage));
    TestFalse(TEXT("Completed has no appearance stage"),
        UCampaignGameInstance::TryGetCorruptionStageForChapter(ECampaignState::Completed, Stage));

    const ECampaignState EarlyChapters[] = {ECampaignState::Prologue, ECampaignState::IshibashiriApproach, ECampaignState::Ishibashiri,
        ECampaignState::Interlude1, ECampaignState::Fuchimatoi};
    for (const ECampaignState Chapter : EarlyChapters)
    {
        TestTrue(TEXT("early chapter resolves"), UCampaignGameInstance::TryGetCorruptionStageForChapter(Chapter, Stage));
        TestEqual(TEXT("chapter uses Early"), Stage, EShirotsuraCorruptionStage::Early);
    }
    const ECampaignState AdvancedChapters[] = {ECampaignState::Interlude2, ECampaignState::Minedaki, ECampaignState::Interlude3,
        ECampaignState::Magatsune, ECampaignState::Ending};
    for (const ECampaignState Chapter : AdvancedChapters)
    {
        TestTrue(TEXT("advanced chapter resolves"), UCampaignGameInstance::TryGetCorruptionStageForChapter(Chapter, Stage));
        TestEqual(TEXT("chapter uses Advanced"), Stage, EShirotsuraCorruptionStage::Advanced);
    }

    C->RestartCampaign();
    C->AdvanceCardChapter();
    TestTrue(TEXT("new campaign Prologue resolves"), C->GetCorruptionStageForEncounter(ECampaignState::Magatsune, Stage));
    TestEqual(TEXT("new campaign is Early"), Stage, EShirotsuraCorruptionStage::Early);
    C->SetCampaignStateForTest(ECampaignState::Fuchimatoi);
    TestTrue(TEXT("Fuchimatoi completion enters Interlude2"), C->CompleteEncounter(ECampaignState::Fuchimatoi));
    TestEqual(TEXT("completion changes chapter first"), C->GetCampaignState(), ECampaignState::Interlude2);
    TestTrue(TEXT("Interlude2 stage resolves"), C->GetCorruptionStageForEncounter(ECampaignState::Fuchimatoi, Stage));
    TestEqual(TEXT("Interlude2 is Advanced"), Stage, EShirotsuraCorruptionStage::Advanced);
    C->SetCampaignStateForTest(ECampaignState::Ending);
    TestTrue(TEXT("Ending stage resolves"), C->GetCorruptionStageForEncounter(ECampaignState::Magatsune, Stage));
    TestEqual(TEXT("Ending retains Advanced"), Stage, EShirotsuraCorruptionStage::Advanced);

    C = NewObject<UCampaignGameInstance>();
    for (const ECampaignState Encounter : {ECampaignState::Ishibashiri, ECampaignState::Fuchimatoi})
    {
        TestTrue(TEXT("standalone early encounter resolves"), C->GetCorruptionStageForEncounter(Encounter, Stage));
        TestEqual(TEXT("standalone encounter is Early"), Stage, EShirotsuraCorruptionStage::Early);
    }
    for (const ECampaignState Encounter : {ECampaignState::Minedaki, ECampaignState::Magatsune})
    {
        TestTrue(TEXT("standalone advanced encounter resolves"), C->GetCorruptionStageForEncounter(Encounter, Stage));
        TestEqual(TEXT("standalone encounter is Advanced"), Stage, EShirotsuraCorruptionStage::Advanced);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSenseChapterReset, "IshibashiriPrototype.Campaign.SenseResetBetweenChapters",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSenseChapterReset::RunTest(const FString&)
{
    auto* S = NewObject<UPlayerSenseComponent>();
    S->BeginBoundarySense();
    S->BeginCorruptionSense();
    S->EndCorruptionSense();
    UCampaignGameInstance::ResetSenseState(S);
    TestFalse(TEXT("Boundary off"), S->IsBoundarySenseActive());
    TestFalse(TEXT("Corruption off"), S->IsCorruptionSenseActive());
    TestEqual(TEXT("risk tail zero"), S->GetRiskRemaining(), 0.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FResumeChapterBoundary, "IshibashiriPrototype.Campaign.ResumeChapterBoundary",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResumeChapterBoundary::RunTest(const FString&)
{
    auto* C = NewObject<UCampaignGameInstance>();
    TestFalse(TEXT("Title is not a checkpoint"), UCampaignGameInstance::IsResumableChapter(ECampaignState::Title));
    TestFalse(TEXT("Completed is not a checkpoint"), UCampaignGameInstance::IsResumableChapter(ECampaignState::Completed));
    TestFalse(TEXT("No checkpoint cannot continue"), C->ContinueCampaign());
    C->SetContinueForTest(ECampaignState::Minedaki);
    TestTrue(TEXT("Chapter boundary can continue"), C->ContinueCampaign());
    TestEqual(TEXT("Continue resumes selected chapter"), C->GetCampaignState(), ECampaignState::Minedaki);
    EShirotsuraCorruptionStage Stage;
    TestTrue(TEXT("Continued chapter resolves appearance"), C->GetCorruptionStageForEncounter(ECampaignState::Minedaki, Stage));
    TestEqual(TEXT("Advanced appearance survives resume"), Stage, EShirotsuraCorruptionStage::Advanced);
    TestFalse(TEXT("Continue is only available on Title"), C->ContinueCampaign());
    C->RestartCampaign();
    C->SetContinueForTest(ECampaignState::Completed);
    TestFalse(TEXT("Invalid saved chapter is rejected"), C->ContinueCampaign());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignNewGameConfirmationTest, "IshibashiriPrototype.Campaign.NewGameOverwriteConfirmation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCampaignNewGameConfirmationTest::RunTest(const FString&)
{
    FCampaignNewGameConfirmation Confirmation;
    TestTrue(TEXT("No save starts on first input"), Confirmation.RequestStart(false));
    TestFalse(TEXT("Save blocks first input"), Confirmation.RequestStart(true));
    TestTrue(TEXT("First input enters confirmation"), Confirmation.IsPending());
    Confirmation.Cancel();
    TestFalse(TEXT("Cancel clears confirmation"), Confirmation.IsPending());
    TestFalse(TEXT("Start after cancel requires confirmation again"), Confirmation.RequestStart(true));
    TestTrue(TEXT("Second consecutive input starts"), Confirmation.RequestStart(true));
    Confirmation.Cancel(); // Continue uses the same reset without blocking its action.
    TestFalse(TEXT("Continue clears transient confirmation"), Confirmation.IsPending());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignSaveRoundTrip, "IshibashiriPrototype.Campaign.SaveRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCampaignSaveRoundTrip::RunTest(const FString&)
{
    const FString Slot = FString::Printf(TEXT("MagabaraiTest_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
    UCampaignSaveGame* Save = NewObject<UCampaignSaveGame>();
    Save->Chapter = ECampaignState::Minedaki;
    const bool bSaved = UGameplayStatics::SaveGameToSlot(Save, Slot, 0);
    TestTrue(TEXT("Separate test slot is writable"), bSaved);
    UCampaignSaveGame* Loaded = bSaved ? Cast<UCampaignSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0)) : nullptr;
    TestNotNull(TEXT("Chapter save survives serialization"), Loaded);
    if (Loaded)
    {
        TestEqual(TEXT("Version retained"), Loaded->Version, UCampaignSaveGame::CurrentVersion);
        TestEqual(TEXT("Chapter retained"), Loaded->Chapter, ECampaignState::Minedaki);
    }
    if (bSaved) TestTrue(TEXT("Test slot cleaned"), UGameplayStatics::DeleteGameInSlot(Slot, 0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignLifecycle, "IshibashiriPrototype.Campaign.FullCampaignStateLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCampaignLifecycle::RunTest(const FString&)
{
    auto* C = NewObject<UCampaignGameInstance>();
    C->RestartCampaign();
    C->AdvanceCardChapter();
    C->AdvanceCardChapter();
    C->CompleteApproach();
    C->CompleteEncounter(ECampaignState::Ishibashiri);
    C->AdvanceCardChapter();
    C->CompleteEncounter(ECampaignState::Fuchimatoi);
    C->AdvanceCardChapter();
    C->CompleteEncounter(ECampaignState::Minedaki);
    C->AdvanceCardChapter();
    C->CompleteEncounter(ECampaignState::Magatsune);
    C->AdvanceCardChapter();
    TestEqual(TEXT("Title through Ending reaches Completed"), C->GetCampaignState(), ECampaignState::Completed);
    return true;
}
#endif
