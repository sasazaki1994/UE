#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "KakonActor.h"
#include "KakonProgressTestListener.h"
#include "NushiProgressComponent.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKakonActorTest,
    "IshibashiriPrototype.Kakon.ActorStateTransitions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKakonActorTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AKakonActor* Kakon = World->SpawnActor<AKakonActor>();
    UKakonProgressTestListener* Listener = NewObject<UKakonProgressTestListener>();
    Kakon->OnPurified.AddDynamic(Listener, &UKakonProgressTestListener::HandlePurified);

    TestEqual(TEXT("Initial state is Covered"), Kakon->GetState(), EKakonState::Covered);
    TestEqual(TEXT("Initial shell health uses the editable maximum"), Kakon->GetCurrentShellHealth(), Kakon->MaxShellHealth);

    const float StartingHealth = Kakon->GetCurrentShellHealth();
    Kakon->ApplyShellDamage(25.f);
    TestEqual(TEXT("Shell damage reduces health"), Kakon->GetCurrentShellHealth(), StartingHealth - 25.f);
    TestFalse(TEXT("Covered Kakon cannot be purified"), Kakon->Purify());
    TestEqual(TEXT("Failed purification leaves Kakon Covered"), Kakon->GetState(), EKakonState::Covered);

    Kakon->ApplyShellDamage(StartingHealth);
    TestEqual(TEXT("Zero shell health exposes Kakon"), Kakon->GetState(), EKakonState::Exposed);
    TestTrue(TEXT("Exposed Kakon can be purified"), Kakon->Purify());
    TestEqual(TEXT("Successful purification changes state"), Kakon->GetState(), EKakonState::Purified);
    TestFalse(TEXT("Purified Kakon cannot be purified twice"), Kakon->Purify());
    TestEqual(TEXT("Purification notification fires once"), Listener->PurifiedEventCount, 1);

    Kakon->MaxShellHealth = 60.f;
    Kakon->ResetKakon();
    TestEqual(TEXT("Reset restores Covered state"), Kakon->GetState(), EKakonState::Covered);
    TestEqual(TEXT("Reset restores current shell health"), Kakon->GetCurrentShellHealth(), 60.f);
    Kakon->ApplyShellDamage(60.f);
    TestTrue(TEXT("Kakon can be purified after reset"), Kakon->Purify());
    TestEqual(TEXT("A new cycle emits one new notification"), Listener->PurifiedEventCount, 2);

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNushiProgressComponentTest,
    "IshibashiriPrototype.Kakon.NushiProgress",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNushiProgressComponentTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AKakonActor* Kakon[3] = {
        World->SpawnActor<AKakonActor>(),
        World->SpawnActor<AKakonActor>(),
        World->SpawnActor<AKakonActor>()
    };
    UNushiProgressComponent* Progress = NewObject<UNushiProgressComponent>();
    UKakonProgressTestListener* Listener = NewObject<UKakonProgressTestListener>();
    Progress->OnAllPurified.AddDynamic(Listener, &UKakonProgressTestListener::HandleAllPurified);

    Progress->RegisterKakon(nullptr);
    for (AKakonActor* Item : Kakon) Progress->RegisterKakon(Item);
    Progress->RegisterKakon(Kakon[0]);
    TestEqual(TEXT("Three unique Kakon are registered"), Progress->GetRegisteredKakonCount(), 3);

    for (int32 Index = 0; Index < 3; ++Index)
    {
        Kakon[Index]->ApplyShellDamage(Kakon[Index]->MaxShellHealth);
        Kakon[Index]->Purify();
        TestEqual(TEXT("Purified count advances once per Kakon"), Progress->GetPurifiedCount(), Index + 1);
    }
    TestTrue(TEXT("Three of three is all purified"), Progress->IsAllPurified());
    TestEqual(TEXT("All-purified notification fires once"), Listener->AllPurifiedEventCount, 1);
    Kakon[2]->Purify();
    TestEqual(TEXT("Duplicate purification cannot produce four of three"), Progress->GetPurifiedCount(), 3);
    TestEqual(TEXT("Duplicate purification does not repeat all-purified"), Listener->AllPurifiedEventCount, 1);

    Progress->ResetProgress();
    TestEqual(TEXT("Reset returns progress to zero of three"), Progress->GetPurifiedCount(), 0);
    TestFalse(TEXT("Reset clears all-purified state"), Progress->IsAllPurified());
    for (AKakonActor* Item : Kakon)
    {
        TestEqual(TEXT("Reset covers each registered Kakon"), Item->GetState(), EKakonState::Covered);
        Item->ApplyShellDamage(Item->MaxShellHealth);
        Item->Purify();
    }
    TestEqual(TEXT("Second cycle reaches three of three"), Progress->GetPurifiedCount(), 3);
    TestEqual(TEXT("All-purified fires once in each cycle"), Listener->AllPurifiedEventCount, 2);

    World->DestroyWorld(false);
    return true;
}

#endif
