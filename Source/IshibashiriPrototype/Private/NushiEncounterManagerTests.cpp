#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "KakonActor.h"
#include "KakonProgressTestListener.h"
#include "NushiBase.h"
#include "NushiEncounterManager.h"
#include "NushiProgressComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNushiEncounterManagerTest,
    "IshibashiriPrototype.Nushi.EncounterManager",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNushiEncounterManagerTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    ANushiEncounterManager* Manager = World->SpawnActor<ANushiEncounterManager>();
    ANushiBase* Nushi = World->SpawnActor<ANushiBase>();
    UNushiProgressComponent* Progress = Nushi->GetNushiProgressComponent();
    UKakonProgressTestListener* StartedListener = NewObject<UKakonProgressTestListener>();
    UKakonProgressTestListener* CompletedListener = NewObject<UKakonProgressTestListener>();
    UKakonProgressTestListener* ResetListener = NewObject<UKakonProgressTestListener>();

    Manager->OnEncounterStarted.AddDynamic(StartedListener, &UKakonProgressTestListener::HandleAllPurified);
    Manager->OnEncounterCompleted.AddDynamic(CompletedListener, &UKakonProgressTestListener::HandleAllPurified);
    Manager->OnEncounterReset.AddDynamic(ResetListener, &UKakonProgressTestListener::HandleAllPurified);

    TestEqual(TEXT("An encounter manager starts Idle"), Manager->GetEncounterState(), ENushiEncounterState::Idle);
    Manager->SetNushi(nullptr);
    Manager->StartEncounter();
    TestNull(TEXT("A null Nushi is accepted safely"), Manager->GetNushi());
    TestEqual(TEXT("A null Nushi cannot start an encounter"), Manager->GetEncounterState(), ENushiEncounterState::Idle);
    TestEqual(TEXT("A failed start emits no notification"), StartedListener->AllPurifiedEventCount, 0);

    Manager->SetNushi(Nushi);
    Manager->SetNushi(Nushi);
    TestEqual(TEXT("The configured Nushi can be queried"), Manager->GetNushi(), Nushi);

    AKakonActor* Kakon[3] = {
        World->SpawnActor<AKakonActor>(),
        World->SpawnActor<AKakonActor>(),
        World->SpawnActor<AKakonActor>()
    };
    for (AKakonActor* Item : Kakon)
    {
        Nushi->RegisterKakon(Item);
    }

    Manager->StartEncounter();
    Manager->StartEncounter();
    TestEqual(TEXT("Start changes the encounter to Running"), Manager->GetEncounterState(), ENushiEncounterState::Running);
    TestEqual(TEXT("Start activates the Nushi"), Nushi->GetNushiState(), ENushiState::Active);
    TestEqual(TEXT("Duplicate starts emit one notification"), StartedListener->AllPurifiedEventCount, 1);

    for (int32 Index = 0; Index < 2; ++Index)
    {
        Kakon[Index]->ApplyShellDamage(Kakon[Index]->MaxShellHealth);
        TestTrue(TEXT("A partially completed Kakon can be purified"), Kakon[Index]->Purify());
        TestEqual(TEXT("Partial purification advances by one"), Progress->GetPurifiedCount(), Index + 1);
        TestEqual(TEXT("Partial purification keeps the Nushi Active"), Nushi->GetNushiState(), ENushiState::Active);
        TestEqual(TEXT("Partial purification keeps the encounter Running"), Manager->GetEncounterState(), ENushiEncounterState::Running);
    }

    Kakon[2]->ApplyShellDamage(Kakon[2]->MaxShellHealth);
    TestTrue(TEXT("The final Kakon can be purified"), Kakon[2]->Purify());
    TestEqual(TEXT("Full purification reaches three of three"), Progress->GetPurifiedCount(), 3);
    TestEqual(TEXT("Full purification calms the Nushi"), Nushi->GetNushiState(), ENushiState::Calm);
    TestEqual(TEXT("A Calm Nushi completes a running encounter"), Manager->GetEncounterState(), ENushiEncounterState::Completed);
    TestEqual(TEXT("Completion emits once"), CompletedListener->AllPurifiedEventCount, 1);
    TestFalse(TEXT("A purified Kakon cannot be purified twice"), Kakon[2]->Purify());
    Manager->StartEncounter();
    TestEqual(TEXT("Duplicate completion does not emit again"), CompletedListener->AllPurifiedEventCount, 1);
    TestEqual(TEXT("Completed encounters cannot start directly"), StartedListener->AllPurifiedEventCount, 1);

    Manager->ResetEncounter();
    TestEqual(TEXT("Reset returns the encounter to Idle"), Manager->GetEncounterState(), ENushiEncounterState::Idle);
    TestEqual(TEXT("Reset returns the Nushi to Dormant"), Nushi->GetNushiState(), ENushiState::Dormant);
    TestEqual(TEXT("Reset clears purification progress"), Progress->GetPurifiedCount(), 0);
    TestEqual(TEXT("Reset emits once"), ResetListener->AllPurifiedEventCount, 1);
    for (AKakonActor* Item : Kakon)
    {
        TestEqual(TEXT("Reset covers each Kakon"), Item->GetState(), EKakonState::Covered);
    }

    Manager->StartEncounter();
    for (AKakonActor* Item : Kakon)
    {
        Item->ApplyShellDamage(Item->MaxShellHealth);
        TestTrue(TEXT("A reset Kakon can be purified again"), Item->Purify());
    }
    TestEqual(TEXT("A replay can complete"), Manager->GetEncounterState(), ENushiEncounterState::Completed);
    TestEqual(TEXT("Replay emits one new start"), StartedListener->AllPurifiedEventCount, 2);
    TestEqual(TEXT("Replay emits one new completion"), CompletedListener->AllPurifiedEventCount, 2);

    World->DestroyWorld(false);
    return true;
}

#endif
