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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFuchimatoiBiteLungeMovementTest,
    "IshibashiriPrototype.Nushi.Fuchimatoi.BiteLungeMovement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFuchimatoiBiteLungeMovementTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AFuchimatoiBoss* Boss = World->SpawnActor<AFuchimatoiBoss>();
    const FVector Target(1000.f, 0.f, 0.f);

    TestEqual(TEXT("The head proxy starts at the local origin"),
        Boss->GetHeadProxyLocalLocation(), FVector::ZeroVector);
    TestFalse(TEXT("The bite target starts unset"), Boss->HasBiteTarget());

    Boss->AdvanceBiteLunge(1.f);
    TestEqual(TEXT("An unset target does not move the proxy"),
        Boss->GetHeadProxyLocalLocation(), FVector::ZeroVector);

    Boss->SetBiteTargetLocalLocation(Target);
    TestEqual(TEXT("The bite target can be set in local space"),
        Boss->GetBiteTargetLocalLocation(), Target);
    TestTrue(TEXT("Setting a target records that it is available"), Boss->HasBiteTarget());
    TestFalse(TEXT("A distant target has not been reached"), Boss->IsBiteTargetReached());

    Boss->BiteLungeSpeed = 100.f;
    Boss->AdvanceBiteLunge(1.f);
    TestEqual(TEXT("Submerged does not move the head proxy"),
        Boss->GetHeadProxyLocalLocation(), FVector::ZeroVector);

    Boss->BeginBiteWindup();
    Boss->AdvanceBiteLunge(1.f);
    TestEqual(TEXT("BiteWindup does not move the head proxy"),
        Boss->GetHeadProxyLocalLocation(), FVector::ZeroVector);

    Boss->BeginBiteLunge();
    Boss->AdvanceBiteLunge(1.f);
    TestTrue(TEXT("The first lunge advance moves at constant speed"),
        Boss->GetHeadProxyLocalLocation().Equals(FVector(100.f, 0.f, 0.f)));
    Boss->AdvanceBiteLunge(1.f);
    TestTrue(TEXT("The second lunge advance continues at constant speed"),
        Boss->GetHeadProxyLocalLocation().Equals(FVector(200.f, 0.f, 0.f)));
    Boss->AdvanceBiteLunge(1.f);
    TestTrue(TEXT("The third lunge advance continues at constant speed"),
        Boss->GetHeadProxyLocalLocation().Equals(FVector(300.f, 0.f, 0.f)));

    const FVector BeforeInvalidDelta = Boss->GetHeadProxyLocalLocation();
    Boss->AdvanceBiteLunge(0.f);
    Boss->AdvanceBiteLunge(-1.f);
    TestEqual(TEXT("Zero and negative delta time do not move the proxy"),
        Boss->GetHeadProxyLocalLocation(), BeforeInvalidDelta);

    Boss->BiteLungeSpeed = 0.f;
    Boss->AdvanceBiteLunge(1.f);
    TestEqual(TEXT("Zero speed does not move the proxy"),
        Boss->GetHeadProxyLocalLocation(), BeforeInvalidDelta);
    TestTrue(TEXT("Zero speed leaves a finite proxy location"),
        !Boss->GetHeadProxyLocalLocation().ContainsNaN());

    Boss->BiteLungeSpeed = 100.f;
    const FVector NearTarget(350.f, 0.f, 0.f);
    Boss->SetBiteTargetLocalLocation(NearTarget);
    Boss->AdvanceBiteLunge(1.f);
    TestEqual(TEXT("Lunge movement stops exactly at a nearby target"),
        Boss->GetHeadProxyLocalLocation(), NearTarget);
    TestTrue(TEXT("The reached target can be queried"), Boss->IsBiteTargetReached());
    TestEqual(TEXT("Reaching the target does not automatically snag the head"),
        Boss->GetActionState(), EFuchimatoiActionState::BiteLunge);

    Boss->SetBiteTargetLocalLocation(FVector(500.f, 0.f, 0.f));
    Boss->NotifyHeadSnagged();
    const FVector SnaggedLocation = Boss->GetHeadProxyLocalLocation();
    Boss->AdvanceBiteLunge(1.f);
    TestEqual(TEXT("A reported snag transitions the action state"),
        Boss->GetActionState(), EFuchimatoiActionState::Snagged);
    TestEqual(TEXT("Snagged does not move the head proxy"),
        Boss->GetHeadProxyLocalLocation(), SnaggedLocation);

    Boss->BeginCoiling();
    Boss->AdvanceBiteLunge(1.f);
    TestEqual(TEXT("Coiling does not move the head proxy"),
        Boss->GetHeadProxyLocalLocation(), SnaggedLocation);

    Boss->StartEncounter();
    Boss->ResetFuchimatoi();
    TestEqual(TEXT("Reset returns the action to Submerged"),
        Boss->GetActionState(), EFuchimatoiActionState::Submerged);
    TestEqual(TEXT("Reset returns the head proxy to the local origin"),
        Boss->GetHeadProxyLocalLocation(), FVector::ZeroVector);
    TestEqual(TEXT("Reset clears the bite target location"),
        Boss->GetBiteTargetLocalLocation(), FVector::ZeroVector);
    TestFalse(TEXT("Reset marks the bite target unset"), Boss->HasBiteTarget());
    TestEqual(TEXT("Reset delegates shared state to NushiBase"),
        Boss->GetNushiState(), ENushiState::Dormant);

    Boss->SetBiteTargetLocalLocation(FVector(200.f, 0.f, 0.f));
    Boss->BeginBiteWindup();
    Boss->BeginBiteLunge();
    Boss->AdvanceBiteLunge(1.f);
    TestEqual(TEXT("A lunge can move again after reset"),
        Boss->GetHeadProxyLocalLocation(), FVector(100.f, 0.f, 0.f));

    World->DestroyWorld(false);
    return true;
}

#endif
