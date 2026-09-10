#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "FuchimatoiBoss.h"
#include "KakonProgressTestListener.h"
#include "NushiStateComponent.h"

#include <limits>

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFuchimatoiCoilingProgressTest,
    "IshibashiriPrototype.Nushi.Fuchimatoi.CoilingProgress",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFuchimatoiCoilingProgressTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AFuchimatoiBoss* Boss = World->SpawnActor<AFuchimatoiBoss>();
    UKakonProgressTestListener* StateListener = NewObject<UKakonProgressTestListener>();
    Boss->OnFuchimatoiActionStateChanged.AddDynamic(
        StateListener, &UKakonProgressTestListener::HandleAllPurified);

    TestEqual(TEXT("Coiling progress starts at zero"), Boss->GetCoilingProgress(), 0.f);
    TestEqual(TEXT("Default coiling duration is two seconds"), Boss->CoilingDuration, 2.f);
    TestFalse(TEXT("Initial coiling is incomplete"), Boss->IsCoilingComplete());
    TestFalse(TEXT("Coiling keeps actor tick disabled"), Boss->PrimaryActorTick.bCanEverTick);

    Boss->AdvanceCoiling(1.f);
    TestEqual(TEXT("Submerged does not advance coiling"), Boss->GetCoilingProgress(), 0.f);
    Boss->BeginBiteWindup();
    Boss->AdvanceCoiling(1.f);
    TestEqual(TEXT("BiteWindup does not advance coiling"), Boss->GetCoilingProgress(), 0.f);
    Boss->BeginBiteLunge();
    Boss->AdvanceCoiling(1.f);
    TestEqual(TEXT("BiteLunge does not advance coiling"), Boss->GetCoilingProgress(), 0.f);
    Boss->NotifyHeadSnagged();
    Boss->AdvanceCoiling(1.f);
    TestEqual(TEXT("Snagged does not advance coiling"), Boss->GetCoilingProgress(), 0.f);
    Boss->BeginCoiling();
    TestEqual(TEXT("Coiling begins at zero"), Boss->GetCoilingProgress(), 0.f);
    const int32 CoilingEventCount = StateListener->AllPurifiedEventCount;

    Boss->AdvanceCoiling(0.5f);
    TestEqual(TEXT("First half second advances a quarter"), Boss->GetCoilingProgress(), 0.25f);
    Boss->BeginCoiling();
    TestEqual(TEXT("Duplicate BeginCoiling preserves progress"), Boss->GetCoilingProgress(), 0.25f);
    Boss->AdvanceCoiling(0.5f);
    TestEqual(TEXT("Second half second advances to halfway"), Boss->GetCoilingProgress(), 0.5f);
    TestFalse(TEXT("Partial coiling is incomplete"), Boss->IsCoilingComplete());
    Boss->AdvanceCoiling(10.f);
    TestEqual(TEXT("Coiling never overshoots one"), Boss->GetCoilingProgress(), 1.f);
    TestTrue(TEXT("Full progress reports completion"), Boss->IsCoilingComplete());
    TestEqual(TEXT("Completion keeps the Coiling action state"),
        Boss->GetActionState(), EFuchimatoiActionState::Coiling);
    TestEqual(TEXT("Advancement and completion emit no state changes"),
        StateListener->AllPurifiedEventCount, CoilingEventCount);
    Boss->AdvanceCoiling(1.f);
    TestEqual(TEXT("Further advancement stays at one"), Boss->GetCoilingProgress(), 1.f);

    Boss->ReturnToSubmerged();
    TestEqual(TEXT("Explicit return still works after completion"),
        Boss->GetActionState(), EFuchimatoiActionState::Submerged);
    Boss->BeginCoiling();
    TestEqual(TEXT("Illegal BeginCoiling preserves completed progress"), Boss->GetCoilingProgress(), 1.f);
    Boss->BeginBiteWindup();
    Boss->BeginBiteLunge();
    Boss->NotifyHeadSnagged();
    Boss->BeginCoiling();
    TestEqual(TEXT("A new cycle without reset starts at zero"), Boss->GetCoilingProgress(), 0.f);
    TestFalse(TEXT("A new cycle clears completion"), Boss->IsCoilingComplete());
    Boss->AdvanceCoiling(0.5f);
    Boss->ReturnToSubmerged();
    Boss->AdvanceCoiling(1.f);
    TestEqual(TEXT("Explicit return freezes partial progress"), Boss->GetCoilingProgress(), 0.25f);

    // Reset a coiling actor with both shared state and bite data populated.
    Boss->StartEncounter();
    Boss->SetBiteTargetLocalLocation(FVector(100.f, 0.f, 0.f));
    Boss->BeginBiteWindup();
    Boss->BeginBiteLunge();
    Boss->AdvanceBiteLunge(1.f);
    Boss->NotifyHeadSnagged();
    Boss->BeginCoiling();
    Boss->AdvanceCoiling(0.5f);
    Boss->ResetFuchimatoi();
    TestEqual(TEXT("Reset clears partial coiling progress"), Boss->GetCoilingProgress(), 0.f);
    TestFalse(TEXT("Reset clears coiling completion"), Boss->IsCoilingComplete());
    TestEqual(TEXT("Reset preserves action reset"), Boss->GetActionState(), EFuchimatoiActionState::Submerged);
    TestEqual(TEXT("Reset preserves shared Nushi reset"), Boss->GetNushiState(), ENushiState::Dormant);
    TestEqual(TEXT("Reset preserves head proxy reset"), Boss->GetHeadProxyLocalLocation(), FVector::ZeroVector);
    TestEqual(TEXT("Reset preserves bite target reset"), Boss->GetBiteTargetLocalLocation(), FVector::ZeroVector);
    TestFalse(TEXT("Reset preserves unset bite target"), Boss->HasBiteTarget());

    Boss->BeginBiteWindup();
    Boss->BeginBiteLunge();
    Boss->NotifyHeadSnagged();
    Boss->BeginCoiling();
    Boss->CoilingDuration = 4.f;
    Boss->AdvanceCoiling(1.f);
    TestEqual(TEXT("Replay uses the configured duration from zero"), Boss->GetCoilingProgress(), 0.25f);
    Boss->AdvanceCoiling(3.f);
    TestTrue(TEXT("Replay completes at exactly the configured duration"), Boss->IsCoilingComplete());
    Boss->ResetFuchimatoi();
    TestEqual(TEXT("Reset clears completed progress"), Boss->GetCoilingProgress(), 0.f);
    TestFalse(TEXT("Reset clears completed flag"), Boss->IsCoilingComplete());

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFuchimatoiCoilingInvalidInputTest,
    "IshibashiriPrototype.Nushi.Fuchimatoi.CoilingInvalidInput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFuchimatoiCoilingInvalidInputTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AFuchimatoiBoss* Boss = World->SpawnActor<AFuchimatoiBoss>();
    Boss->BeginBiteWindup();
    Boss->BeginBiteLunge();
    Boss->NotifyHeadSnagged();
    Boss->BeginCoiling();
    Boss->AdvanceCoiling(0.5f);

    const float InvalidInputs[] = {
        0.f, -1.f, std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()
    };
    // Validate each invalid value independently, both during progress and after completion.
    for (const float ExpectedProgress : {0.25f, 1.f})
    {
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(InvalidInputs); ++Index)
        {
            Boss->AdvanceCoiling(InvalidInputs[Index]);
            TestEqual(FString::Printf(TEXT("Invalid delta %d preserves progress %g"), Index, ExpectedProgress),
                Boss->GetCoilingProgress(), ExpectedProgress);
            TestTrue(TEXT("Invalid delta leaves finite progress"), FMath::IsFinite(Boss->GetCoilingProgress()));

            Boss->CoilingDuration = InvalidInputs[Index];
            Boss->AdvanceCoiling(1.f);
            TestEqual(FString::Printf(TEXT("Invalid duration %d preserves progress %g"), Index, ExpectedProgress),
                Boss->GetCoilingProgress(), ExpectedProgress);
            TestTrue(TEXT("Invalid duration leaves finite progress"), FMath::IsFinite(Boss->GetCoilingProgress()));
            Boss->CoilingDuration = 2.f;
        }
        TestEqual(TEXT("Invalid inputs preserve completion status"), Boss->IsCoilingComplete(), ExpectedProgress == 1.f);
        TestEqual(TEXT("Invalid inputs keep Coiling state"), Boss->GetActionState(), EFuchimatoiActionState::Coiling);
        Boss->AdvanceCoiling(2.f);
        TestEqual(TEXT("Valid advancement after invalid inputs completes safely"), Boss->GetCoilingProgress(), 1.f);
    }

    Boss->ResetFuchimatoi();
    Boss->BeginBiteWindup();
    Boss->BeginBiteLunge();
    Boss->NotifyHeadSnagged();
    Boss->BeginCoiling();
    Boss->CoilingDuration = std::numeric_limits<float>::min();
    Boss->AdvanceCoiling(std::numeric_limits<float>::max());
    TestEqual(TEXT("Extreme finite inputs clamp safely to one"), Boss->GetCoilingProgress(), 1.f);
    TestTrue(TEXT("Extreme finite inputs complete coiling"), Boss->IsCoilingComplete());
    TestEqual(TEXT("Extreme finite inputs keep Coiling state"), Boss->GetActionState(), EFuchimatoiActionState::Coiling);

    World->DestroyWorld(false);
    return true;
}

#endif
