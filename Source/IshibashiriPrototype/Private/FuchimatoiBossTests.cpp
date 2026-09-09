#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "FuchimatoiBoss.h"
#include "KakonProgressTestListener.h"
#include "NushiStateComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFuchimatoiBossTest,
    "IshibashiriPrototype.Nushi.Fuchimatoi.ActionLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFuchimatoiBossTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AFuchimatoiBoss* Boss = World->SpawnActor<AFuchimatoiBoss>();
    UKakonProgressTestListener* StateListener = NewObject<UKakonProgressTestListener>();
    Boss->OnFuchimatoiActionStateChanged.AddDynamic(
        StateListener, &UKakonProgressTestListener::HandleAllPurified);

    TestEqual(TEXT("Fuchimatoi starts Submerged"), Boss->GetActionState(), EFuchimatoiActionState::Submerged);
    TestFalse(TEXT("A submerged Fuchimatoi is not snagged"), Boss->IsSnagged());
    TestFalse(TEXT("A submerged Fuchimatoi is not coiling"), Boss->IsCoiling());

    Boss->BeginCoiling();
    Boss->NotifyHeadSnagged();
    TestEqual(TEXT("Submerged rejects Coiling and Snagged"), Boss->GetActionState(), EFuchimatoiActionState::Submerged);
    TestEqual(TEXT("Illegal transitions emit no notifications"), StateListener->AllPurifiedEventCount, 0);

    Boss->BeginBiteWindup();
    Boss->BeginBiteWindup();
    TestEqual(TEXT("Bite windup begins from Submerged"), Boss->GetActionState(), EFuchimatoiActionState::BiteWindup);
    TestEqual(TEXT("Duplicate windup emits once"), StateListener->AllPurifiedEventCount, 1);

    Boss->NotifyHeadSnagged();
    TestEqual(TEXT("BiteWindup rejects Snagged"), Boss->GetActionState(), EFuchimatoiActionState::BiteWindup);
    Boss->BeginBiteLunge();
    TestEqual(TEXT("Bite lunge begins after windup"), Boss->GetActionState(), EFuchimatoiActionState::BiteLunge);

    Boss->NotifyHeadSnagged();
    Boss->NotifyHeadSnagged();
    TestEqual(TEXT("The head becomes Snagged after a lunge"), Boss->GetActionState(), EFuchimatoiActionState::Snagged);
    TestTrue(TEXT("Snagged can be queried"), Boss->IsSnagged());
    TestEqual(TEXT("Duplicate snag notification emits once"), StateListener->AllPurifiedEventCount, 3);

    Boss->BeginBiteLunge();
    TestEqual(TEXT("Snagged rejects BiteLunge"), Boss->GetActionState(), EFuchimatoiActionState::Snagged);
    Boss->BeginCoiling();
    TestEqual(TEXT("Coiling begins after Snagged"), Boss->GetActionState(), EFuchimatoiActionState::Coiling);
    TestTrue(TEXT("Coiling can be queried"), Boss->IsCoiling());

    Boss->ReturnToSubmerged();
    TestEqual(TEXT("A completed cycle returns to Submerged"), Boss->GetActionState(), EFuchimatoiActionState::Submerged);

    Boss->StartEncounter();
    Boss->BeginBiteWindup();
    Boss->BeginBiteLunge();
    Boss->NotifyHeadSnagged();
    Boss->BeginCoiling();
    Boss->ResetFuchimatoi();
    TestEqual(TEXT("Reset returns the action to Submerged"), Boss->GetActionState(), EFuchimatoiActionState::Submerged);
    TestEqual(TEXT("Reset delegates shared state to NushiBase"), Boss->GetNushiState(), ENushiState::Dormant);

    Boss->BeginBiteWindup();
    Boss->BeginBiteLunge();
    Boss->NotifyHeadSnagged();
    Boss->BeginCoiling();
    TestEqual(TEXT("The action cycle can replay after reset"), Boss->GetActionState(), EFuchimatoiActionState::Coiling);

    World->DestroyWorld(false);
    return true;
}

#endif
