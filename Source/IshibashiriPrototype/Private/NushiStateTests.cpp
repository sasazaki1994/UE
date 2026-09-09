#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "KakonActor.h"
#include "KakonProgressTestListener.h"
#include "NushiProgressComponent.h"
#include "NushiStateComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNushiStateComponentTest,
    "IshibashiriPrototype.Nushi.StateProgression",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNushiStateComponentTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AActor* Nushi = World->SpawnActor<AActor>();
    UNushiProgressComponent* Progress = NewObject<UNushiProgressComponent>(Nushi);
    UNushiStateComponent* State = NewObject<UNushiStateComponent>(Nushi);
    Nushi->AddInstanceComponent(Progress);
    Nushi->AddInstanceComponent(State);

    AKakonActor* Kakon[3] = {
        World->SpawnActor<AKakonActor>(),
        World->SpawnActor<AKakonActor>(),
        World->SpawnActor<AKakonActor>()
    };
    for (AKakonActor* Item : Kakon)
    {
        Progress->RegisterKakon(Item);
    }

    UKakonProgressTestListener* Listener = NewObject<UKakonProgressTestListener>();
    State->OnNushiStateChanged.AddDynamic(Listener, &UKakonProgressTestListener::HandleAllPurified);

    TestEqual(TEXT("Initial state is Dormant"), State->GetNushiState(), ENushiState::Dormant);
    State->ResetNushi();
    for (AKakonActor* Item : Kakon)
    {
        Item->ApplyShellDamage(Item->MaxShellHealth);
        Item->Purify();
    }
    TestEqual(TEXT("Full purification while Dormant does not make the Nushi Calm"), State->GetNushiState(), ENushiState::Dormant);
    TestEqual(TEXT("Ignored purification does not emit a state change"), Listener->AllPurifiedEventCount, 0);
    State->ResetNushi();

    State->StartEncounter();
    TestEqual(TEXT("Starting a dormant Nushi makes it Active"), State->GetNushiState(), ENushiState::Active);
    TestTrue(TEXT("Active query reflects the state"), State->IsActive());
    State->StartEncounter();
    TestEqual(TEXT("Starting twice emits one state change"), Listener->AllPurifiedEventCount, 1);

    for (int32 Index = 0; Index < 2; ++Index)
    {
        Kakon[Index]->ApplyShellDamage(Kakon[Index]->MaxShellHealth);
        TestTrue(TEXT("An exposed Kakon can be purified"), Kakon[Index]->Purify());
        TestEqual(TEXT("Partial purification advances progress"), Progress->GetPurifiedCount(), Index + 1);
        TestEqual(TEXT("Partial purification keeps the Nushi Active"), State->GetNushiState(), ENushiState::Active);
    }

    Kakon[2]->ApplyShellDamage(Kakon[2]->MaxShellHealth);
    TestTrue(TEXT("The third exposed Kakon can be purified"), Kakon[2]->Purify());
    TestEqual(TEXT("All three Kakon are counted"), Progress->GetPurifiedCount(), 3);
    TestEqual(TEXT("Full purification makes the Nushi Calm"), State->GetNushiState(), ENushiState::Calm);
    TestTrue(TEXT("Calm query reflects the state"), State->IsCalm());
    TestEqual(TEXT("Calm emits one additional state change"), Listener->AllPurifiedEventCount, 2);

    TestFalse(TEXT("A purified Kakon cannot be purified twice"), Kakon[2]->Purify());
    TestEqual(TEXT("Duplicate purification keeps the Nushi Calm"), State->GetNushiState(), ENushiState::Calm);
    TestEqual(TEXT("Duplicate purification does not emit a state change"), Listener->AllPurifiedEventCount, 2);

    State->ResetNushi();
    TestEqual(TEXT("Reset returns the Nushi to Dormant"), State->GetNushiState(), ENushiState::Dormant);
    TestEqual(TEXT("Reset clears purification progress"), Progress->GetPurifiedCount(), 0);
    for (AKakonActor* Item : Kakon)
    {
        TestEqual(TEXT("Reset covers every registered Kakon"), Item->GetState(), EKakonState::Covered);
    }

    State->StartEncounter();
    TestEqual(TEXT("A reset Nushi can start another encounter"), State->GetNushiState(), ENushiState::Active);
    for (AKakonActor* Item : Kakon)
    {
        Item->ApplyShellDamage(Item->MaxShellHealth);
        Item->Purify();
    }
    TestEqual(TEXT("A second full purification makes the Nushi Calm"), State->GetNushiState(), ENushiState::Calm);
    TestEqual(TEXT("Each transition emits exactly once across both cycles"), Listener->AllPurifiedEventCount, 5);

    World->DestroyWorld(false);
    return true;
}

#endif
