#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "IshibashiriBoss.h"
#include "KakonActor.h"
#include "KakonProgressTestListener.h"
#include "NushiEncounterManager.h"
#include "NushiProgressComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIshibashiriNushiIntegrationTest,
    "IshibashiriPrototype.Nushi.IshibashiriIntegration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIshibashiriNushiIntegrationTest::RunTest(const FString& Parameters)
{
    // Exercise the actual boss without a map, player or renderer. Its normal
    // constructor still loads existing visuals; the shellless test below is asset-free.
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    AIshibashiriBoss* Boss = World->SpawnActor<AIshibashiriBoss>();
    Boss->DispatchBeginPlay();
    ANushiEncounterManager* Manager = World->SpawnActor<ANushiEncounterManager>();
    Manager->SetNushi(Boss);
    UNushiProgressComponent* Progress = Boss->GetNushiProgressComponent();
    UKakonProgressTestListener* Completed = NewObject<UKakonProgressTestListener>();
    UKakonProgressTestListener* Reset = NewObject<UKakonProgressTestListener>();
    UKakonProgressTestListener* Started = NewObject<UKakonProgressTestListener>();
    UKakonProgressTestListener* Purified = NewObject<UKakonProgressTestListener>();
    Manager->OnEncounterCompleted.AddDynamic(Completed, &UKakonProgressTestListener::HandleAllPurified);
    Manager->OnEncounterReset.AddDynamic(Reset, &UKakonProgressTestListener::HandleAllPurified);
    Manager->OnEncounterStarted.AddDynamic(Started, &UKakonProgressTestListener::HandleAllPurified);
    Progress->OnAllPurified.AddDynamic(Purified, &UKakonProgressTestListener::HandleAllPurified);

    const FVector AuthoredCores[] = {{218,-155,611}, {0,60,784}, {-122,305,688}};
    const FTransform Spawn(FRotator(0, 123, 0), FVector(250, -400, 352));
    Boss->ConfigureEncounter(Spawn, nullptr);
    Manager->ResetEncounter();
    TestEqual(TEXT("Exactly three authored Kakon are registered"), Progress->GetRegisteredKakonCount(), 3);
    TestEqual(TEXT("Initial reset is Dormant"), Boss->GetNushiState(), ENushiState::Dormant);
    TestEqual(TEXT("Initial reset has no lifecycle reset notification"), Reset->AllPurifiedEventCount, 0);

    for (int32 Cycle = 0; Cycle < 2; ++Cycle)
    {
        Manager->StartEncounter();
        Manager->StartEncounter();
        TestEqual(TEXT("Start activates the actual Ishibashiri"), Boss->GetNushiState(), ENushiState::Active);
        TestEqual(TEXT("Start runs the manager"), Manager->GetEncounterState(), ENushiEncounterState::Running);
        for (int32 I = 0; I < 3; ++I)
        {
            AKakonActor* Kakon = Boss->GetCoreKakon(I);
            if (!TestNotNull(TEXT("The authored Kakon child exists"), Kakon))
            {
                World->DestroyWorld(false);
                GEngine->DestroyWorldContext(World);
                return false;
            }
            TestTrue(TEXT("Kakon follows the original coordinate through the creature frame"),
                Kakon->GetActorLocation().Equals(Boss->GetClimbFrame().TransformPosition(AuthoredCores[I]), .01f));
            TestEqual(TEXT("Ishibashiri cores need no extra shell gameplay"), Kakon->GetState(), EKakonState::Exposed);
            TestTrue(TEXT("Registered Kakon purifies"), Kakon->Purify());
            TestFalse(TEXT("Purification cannot fire twice"), Kakon->Purify());
            TestEqual(TEXT("Progress advances once per core"), Progress->GetPurifiedCount(), I + 1);
            TestEqual(TEXT("HUD progress reads the shared source"), Boss->GetPurifiedCount(), I + 1);
            TestEqual(TEXT("Existing combat health reduction is preserved"), Boss->GetHealth(), 2 - I);
            TestEqual(TEXT("Lifecycle follows shared progress"), Manager->GetEncounterState(),
                I == 2 ? ENushiEncounterState::Completed : ENushiEncounterState::Running);
        }
        TestTrue(TEXT("Progress reports all purified"), Progress->IsAllPurified());
        TestEqual(TEXT("All purification calms the shared state"), Boss->GetNushiState(), ENushiState::Calm);
        TestEqual(TEXT("Legacy presentation derives Calmed from shared state"), Boss->GetState(), EIshibashiriState::Calmed);
        TestEqual(TEXT("Completion fires once per cycle"), Completed->AllPurifiedEventCount, Cycle + 1);
        TestEqual(TEXT("All purified fires once per cycle"), Purified->AllPurifiedEventCount, Cycle + 1);
        TestEqual(TEXT("Start fires once per cycle"), Started->AllPurifiedEventCount, Cycle + 1);

        Boss->SetActorLocation(FVector(1000, 2000, 3000));
        Manager->ResetEncounter();
        TestEqual(TEXT("Reset returns the manager to Idle"), Manager->GetEncounterState(), ENushiEncounterState::Idle);
        TestEqual(TEXT("Reset returns shared state to Dormant"), Boss->GetNushiState(), ENushiState::Dormant);
        TestEqual(TEXT("Reset returns progress to zero of three"), Progress->GetPurifiedCount(), 0);
        TestFalse(TEXT("Reset clears all-purified flag"), Progress->IsAllPurified());
        TestEqual(TEXT("Reset keeps three registrations"), Progress->GetRegisteredKakonCount(), 3);
        TestEqual(TEXT("Virtual reset restores boss health"), Boss->GetHealth(), Boss->MaxHealth);
        TestEqual(TEXT("Virtual reset restores Chase"), Boss->GetState(), EIshibashiriState::Chase);
        TestTrue(TEXT("Virtual reset restores the configured spawn"), Boss->GetActorTransform().Equals(Spawn));
        TestEqual(TEXT("One lifecycle reset notification per cycle"), Reset->AllPurifiedEventCount, Cycle + 1);
    }

    // Combat victory uses the same lifecycle without claiming any Kakon was purified.
    Manager->StartEncounter();
    Boss->GetNushiStateComponent()->CalmNushi();
    Boss->GetNushiStateComponent()->CalmNushi();
    TestEqual(TEXT("Alternate combat completion reaches the manager"), Manager->GetEncounterState(), ENushiEncounterState::Completed);
    TestEqual(TEXT("Alternate completion does not fabricate progress"), Boss->GetPurifiedCount(), 0);
    TestEqual(TEXT("Alternate completion fires once"), Completed->AllPurifiedEventCount, 3);
    TestEqual(TEXT("Alternate completion emits no all-purified event"), Purified->AllPurifiedEventCount, 2);
    Manager->ResetEncounter();
    Manager->StartEncounter();
    TestEqual(TEXT("Can start again after combat completion"), Boss->GetNushiState(), ENushiState::Active);

    Boss->GetCoreKakon(0)->Purify();
    Boss->GetNushiStateComponent()->CalmNushi();
    TestEqual(TEXT("Alternate completion retains partial purification truth"), Boss->GetPurifiedCount(), 1);
    TestFalse(TEXT("Partial progress is not all purified on alternate completion"), Progress->IsAllPurified());

    // Purification completion must not depend on the legacy combat health reaching zero.
    Boss->MaxHealth = 5;
    Manager->ResetEncounter();
    Manager->StartEncounter();
    for (int32 I = 0; I < 3; ++I) Boss->GetCoreKakon(I)->Purify();
    TestEqual(TEXT("All purification completes even with combat health remaining"), Boss->GetHealth(), 2);
    TestEqual(TEXT("Shared progress owns full purification completion"), Manager->GetEncounterState(), ENushiEncounterState::Completed);

    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShelllessKakonLifecycleTest,
    "IshibashiriPrototype.Nushi.ShelllessKakonLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShelllessKakonLifecycleTest::RunTest(const FString& Parameters)
{
    // Only common actors/components: no Ishibashiri constructor or 3D assets.
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    World->InitializeActorsForPlay(FURL());
    ANushiBase* Nushi = World->SpawnActor<ANushiBase>();
    ANushiEncounterManager* Manager = World->SpawnActor<ANushiEncounterManager>();
    Manager->SetNushi(Nushi);
    AKakonActor* Kakon[3];
    for (AKakonActor*& Item : Kakon)
    {
        Item = World->SpawnActor<AKakonActor>();
        Item->MaxShellHealth = 0.f;
        Item->ResetKakon();
        Nushi->RegisterKakon(Item);
    }
    UNushiProgressComponent* Progress = Nushi->GetNushiProgressComponent();
    TestEqual(TEXT("Three shellless Kakon registered"), Progress->GetRegisteredKakonCount(), 3);
    for (int32 Cycle = 0; Cycle < 2; ++Cycle)
    {
        Manager->StartEncounter();
        TestEqual(TEXT("Start activates common state"), Nushi->GetNushiState(), ENushiState::Active);
        for (int32 I = 0; I < 3; ++I)
        {
            TestEqual(TEXT("Shellless Kakon are exposed on every cycle"), Kakon[I]->GetState(), EKakonState::Exposed);
            TestTrue(TEXT("Shellless Kakon can purify without shell damage"), Kakon[I]->Purify());
            TestEqual(TEXT("Progress advances one of three at a time"), Progress->GetPurifiedCount(), I + 1);
        }
        TestEqual(TEXT("Full purification calms common state"), Nushi->GetNushiState(), ENushiState::Calm);
        TestEqual(TEXT("Calm completes the manager"), Manager->GetEncounterState(), ENushiEncounterState::Completed);
        Manager->ResetEncounter();
        TestEqual(TEXT("Reset clears progress"), Progress->GetPurifiedCount(), 0);
        TestEqual(TEXT("Reset restores Dormant"), Nushi->GetNushiState(), ENushiState::Dormant);
        TestEqual(TEXT("Reset restores Idle"), Manager->GetEncounterState(), ENushiEncounterState::Idle);
    }
    World->DestroyWorld(false);
    return true;
}

#endif
