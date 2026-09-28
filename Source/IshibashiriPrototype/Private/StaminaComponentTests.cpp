#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "KakonProgressTestListener.h"
#include "StaminaComponent.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStaminaComponentTest,
    "IshibashiriPrototype.Player.StaminaResource",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStaminaComponentTest::RunTest(const FString& Parameters)
{
    UStaminaComponent* Stamina = NewObject<UStaminaComponent>();
    TestEqual(TEXT("Maximum stamina starts at 100"), Stamina->GetMaxStamina(), 100.0f);
    TestEqual(TEXT("Current stamina starts at 100"), Stamina->GetCurrentStamina(), 100.0f);
    TestFalse(TEXT("Full stamina is not depleted"), Stamina->IsDepleted());

    Stamina->ConsumeStamina(25.0f);
    TestEqual(TEXT("Consumption subtracts stamina"), Stamina->GetCurrentStamina(), 75.0f);
    Stamina->ConsumeStamina(65.0f);
    Stamina->ConsumeStamina(30.0f);
    TestEqual(TEXT("Consumption clamps at zero"), Stamina->GetCurrentStamina(), 0.0f);
    TestTrue(TEXT("Zero stamina is depleted"), Stamina->IsDepleted());

    Stamina->RestoreStamina(50.0f);
    Stamina->ConsumeStamina(-10.0f);
    TestEqual(TEXT("Negative consumption is ignored"), Stamina->GetCurrentStamina(), 50.0f);
    Stamina->ConsumeStamina(10.0f);
    Stamina->RestoreStamina(20.0f);
    TestEqual(TEXT("Restoration adds stamina"), Stamina->GetCurrentStamina(), 60.0f);
    Stamina->RestoreStamina(-10.0f);
    TestEqual(TEXT("Negative restoration is ignored"), Stamina->GetCurrentStamina(), 60.0f);
    Stamina->RestoreStamina(30.0f);
    Stamina->RestoreStamina(30.0f);
    TestEqual(TEXT("Restoration clamps at the maximum"), Stamina->GetCurrentStamina(), 100.0f);

    Stamina->ConsumeStamina(70.0f);
    Stamina->RefillStamina();
    TestEqual(TEXT("Refill restores maximum stamina"), Stamina->GetCurrentStamina(), 100.0f);
    Stamina->ConsumeStamina(85.0f);
    Stamina->ResetStamina();
    TestEqual(TEXT("Reset restores maximum stamina"), Stamina->GetCurrentStamina(), 100.0f);

    UStaminaComponent* EventStamina = NewObject<UStaminaComponent>();
    UKakonProgressTestListener* ChangedListener = NewObject<UKakonProgressTestListener>();
    UKakonProgressTestListener* DepletedListener = NewObject<UKakonProgressTestListener>();
    EventStamina->OnStaminaChanged.AddDynamic(ChangedListener, &UKakonProgressTestListener::HandleAllPurified);
    EventStamina->OnStaminaDepleted.AddDynamic(DepletedListener, &UKakonProgressTestListener::HandleAllPurified);

    EventStamina->RestoreStamina(20.0f);
    EventStamina->ConsumeStamina(-10.0f);
    TestEqual(TEXT("No-op and invalid operations do not emit changed"), ChangedListener->AllPurifiedEventCount, 0);
    EventStamina->ConsumeStamina(80.0f);
    TestEqual(TEXT("A real value change emits changed once"), ChangedListener->AllPurifiedEventCount, 1);
    TestEqual(TEXT("Positive stamina does not emit depleted"), DepletedListener->AllPurifiedEventCount, 0);
    EventStamina->ConsumeStamina(30.0f);
    TestEqual(TEXT("Reaching zero emits changed"), ChangedListener->AllPurifiedEventCount, 2);
    TestEqual(TEXT("Reaching zero emits depleted once"), DepletedListener->AllPurifiedEventCount, 1);
    EventStamina->ConsumeStamina(10.0f);
    TestEqual(TEXT("Consumption at zero does not emit changed again"), ChangedListener->AllPurifiedEventCount, 2);
    TestEqual(TEXT("Consumption at zero does not emit depleted again"), DepletedListener->AllPurifiedEventCount, 1);
    EventStamina->RestoreStamina(50.0f);
    EventStamina->ConsumeStamina(50.0f);
    TestEqual(TEXT("Recovery and renewed depletion each emit changed"), ChangedListener->AllPurifiedEventCount, 4);
    TestEqual(TEXT("A renewed transition to zero emits depleted again"), DepletedListener->AllPurifiedEventCount, 2);

    UStaminaComponent* ConfiguredStamina = NewObject<UStaminaComponent>();
    ConfiguredStamina->ConsumeStamina(25.0f);
    ConfiguredStamina->SetMaxStamina(50.0f);
    TestEqual(TEXT("Lowering the maximum clamps current stamina"), ConfiguredStamina->GetCurrentStamina(), 50.0f);
    ConfiguredStamina->SetMaxStamina(-10.0f);
    TestEqual(TEXT("A negative maximum clamps to zero"), ConfiguredStamina->GetMaxStamina(), 0.0f);
    TestEqual(TEXT("Current stamina remains bounded by a zero maximum"), ConfiguredStamina->GetCurrentStamina(), 0.0f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStaminaNumericBoundariesTest,
    "IshibashiriPrototype.Player.StaminaNumericBoundaries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStaminaNumericBoundariesTest::RunTest(const FString& Parameters)
{
    UStaminaComponent* Stamina = NewObject<UStaminaComponent>();
    Stamina->ConsumeStamina(25.0f);
    UKakonProgressTestListener* ChangedListener = NewObject<UKakonProgressTestListener>();
    UKakonProgressTestListener* DepletedListener = NewObject<UKakonProgressTestListener>();
    Stamina->OnStaminaChanged.AddDynamic(ChangedListener, &UKakonProgressTestListener::HandleAllPurified);
    Stamina->OnStaminaDepleted.AddDynamic(DepletedListener, &UKakonProgressTestListener::HandleAllPurified);

    const float InvalidAmounts[] = {
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity()
    };
    for (float Amount : InvalidAmounts)
    {
        Stamina->ConsumeStamina(Amount);
        TestEqual(TEXT("Non-finite consumption leaves stamina unchanged"), Stamina->GetCurrentStamina(), 75.0f);
        Stamina->RestoreStamina(Amount);
        TestEqual(TEXT("Non-finite restoration leaves stamina unchanged"), Stamina->GetCurrentStamina(), 75.0f);
        Stamina->SetMaxStamina(Amount);
        TestEqual(TEXT("Non-finite maximum leaves configuration unchanged"), Stamina->GetMaxStamina(), 100.0f);
        TestEqual(TEXT("Non-finite maximum leaves stamina unchanged"), Stamina->GetCurrentStamina(), 75.0f);
    }
    TestEqual(TEXT("Rejected numbers do not emit changed"), ChangedListener->AllPurifiedEventCount, 0);
    TestEqual(TEXT("Rejected numbers do not emit depleted"), DepletedListener->AllPurifiedEventCount, 0);

    const float SmallMaximum = UE_SMALL_NUMBER * 0.5f;
    Stamina->SetMaxStamina(SmallMaximum);
    TestTrue(TEXT("A small positive maximum still holds stamina"), Stamina->GetCurrentStamina() == SmallMaximum);
    Stamina->ConsumeStamina(SmallMaximum);
    TestTrue(TEXT("The last small fraction is consumed exactly"), Stamina->GetCurrentStamina() == 0.0f);
    TestTrue(TEXT("Consuming the last fraction marks stamina depleted"), Stamina->IsDepleted());
    TestEqual(TEXT("Consuming the last fraction emits depleted once"), DepletedListener->AllPurifiedEventCount, 1);
    Stamina->ConsumeStamina(SmallMaximum);
    TestEqual(TEXT("Repeated consumption at zero emits no duplicate depletion"), DepletedListener->AllPurifiedEventCount, 1);

    Stamina->RestoreStamina(SmallMaximum);
    TestTrue(TEXT("A small restoration recovers from depletion"), Stamina->GetCurrentStamina() == SmallMaximum);
    TestFalse(TEXT("A restored fraction is not depleted"), Stamina->IsDepleted());
    Stamina->SetMaxStamina(SmallMaximum * 0.5f);
    TestTrue(TEXT("A tiny maximum reduction keeps current stamina bounded"),
        Stamina->GetCurrentStamina() == Stamina->GetMaxStamina());
    Stamina->SetMaxStamina(0.0f);
    TestTrue(TEXT("A zero maximum clears even a tiny remainder"), Stamina->GetCurrentStamina() == 0.0f);
    TestEqual(TEXT("A maximum reduction to zero emits depletion"), DepletedListener->AllPurifiedEventCount, 2);
    TestEqual(TEXT("Each real numeric change emits once"), ChangedListener->AllPurifiedEventCount, 5);

    return true;
}

#endif
