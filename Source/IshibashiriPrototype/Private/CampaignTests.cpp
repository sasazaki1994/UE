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
    TestEqual(TEXT("retry owns no campaign transition"), C->GetCampaignState(), ECampaignState::Minedaki);
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
