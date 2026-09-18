#include "CampaignGameInstance.h"
#include "PlayerSenseComponent.h"
#include "Misc/AutomationTest.h"

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
