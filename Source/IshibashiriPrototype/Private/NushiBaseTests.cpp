#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "KakonActor.h"
#include "NushiBase.h"
#include "NushiProgressComponent.h"
#include "NushiStateComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNushiBaseTest,
    "IshibashiriPrototype.Nushi.CommonBase",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNushiBaseTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    ANushiBase* Nushi = World->SpawnActor<ANushiBase>();
    UNushiProgressComponent* Progress = Nushi->GetNushiProgressComponent();
    UNushiStateComponent* State = Nushi->GetNushiStateComponent();

    TestNotNull(TEXT("NushiBase contains a NushiProgressComponent"), Progress);
    TestNotNull(TEXT("NushiBase contains a NushiStateComponent"), State);
    TestEqual(TEXT("NushiBase starts Dormant"), Nushi->GetNushiState(), ENushiState::Dormant);

    AKakonActor* Kakon[3] = {
        World->SpawnActor<AKakonActor>(),
        World->SpawnActor<AKakonActor>(),
        World->SpawnActor<AKakonActor>()
    };
    for (AKakonActor* Item : Kakon)
    {
        Nushi->RegisterKakon(Item);
    }
    Nushi->RegisterKakon(Kakon[0]);
    TestEqual(TEXT("NushiBase delegates unique Kakon registration"), Progress->GetRegisteredKakonCount(), 3);

    Nushi->StartEncounter();
    TestEqual(TEXT("Starting the encounter makes the Nushi Active"), Nushi->GetNushiState(), ENushiState::Active);

    for (int32 Index = 0; Index < 2; ++Index)
    {
        Kakon[Index]->ApplyShellDamage(Kakon[Index]->MaxShellHealth);
        TestTrue(TEXT("An exposed Kakon can be purified"), Kakon[Index]->Purify());
        TestEqual(TEXT("Partial purification advances through the shared progress component"), Progress->GetPurifiedCount(), Index + 1);
        TestEqual(TEXT("Partial purification keeps the Nushi Active"), Nushi->GetNushiState(), ENushiState::Active);
    }

    Kakon[2]->ApplyShellDamage(Kakon[2]->MaxShellHealth);
    TestTrue(TEXT("The final exposed Kakon can be purified"), Kakon[2]->Purify());
    TestEqual(TEXT("Full purification reaches three of three"), Progress->GetPurifiedCount(), 3);
    TestEqual(TEXT("Full purification makes the Nushi Calm"), Nushi->GetNushiState(), ENushiState::Calm);

    Nushi->ResetNushi();
    TestEqual(TEXT("Reset returns the Nushi to Dormant"), Nushi->GetNushiState(), ENushiState::Dormant);
    TestEqual(TEXT("Reset clears the shared purification progress"), Progress->GetPurifiedCount(), 0);
    for (AKakonActor* Item : Kakon)
    {
        TestEqual(TEXT("Reset returns each Kakon to Covered"), Item->GetState(), EKakonState::Covered);
    }

    Nushi->StartEncounter();
    TestEqual(TEXT("A reset Nushi can start another encounter"), Nushi->GetNushiState(), ENushiState::Active);
    for (AKakonActor* Item : Kakon)
    {
        Item->ApplyShellDamage(Item->MaxShellHealth);
        TestTrue(TEXT("Each Kakon can be purified in the next encounter"), Item->Purify());
    }
    TestEqual(TEXT("The second encounter reaches three of three"), Progress->GetPurifiedCount(), 3);
    TestEqual(TEXT("The second encounter can make the Nushi Calm"), Nushi->GetNushiState(), ENushiState::Calm);

    World->DestroyWorld(false);
    return true;
}

#endif
