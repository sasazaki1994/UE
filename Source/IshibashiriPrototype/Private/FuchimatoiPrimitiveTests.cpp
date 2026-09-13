#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "FuchimatoiBoss.h"
#include "FuchimatoiArena.h"
#include "FuchimatoiPlayer.h"
#include "FuchimatoiRouteAnchor.h"
#include "KakonActor.h"
#include "NushiEncounterManager.h"
#include "NushiProgressComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFuchimatoiPrimitiveIntegrationTest,
    "IshibashiriPrototype.Nushi.Fuchimatoi.PrimitiveIntegration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFuchimatoiPrimitiveIntegrationTest::RunTest(const FString& Parameters)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    AFuchimatoiArena* Arena=World->SpawnActor<AFuchimatoiArena>();
    Arena->DispatchBeginPlay();
    AFuchimatoiPlayer* Player=World->SpawnActor<AFuchimatoiPlayer>();
    AFuchimatoiBoss* Boss=World->SpawnActor<AFuchimatoiBoss>();
    Boss->SetActorTransform(Arena->GetBossSpawn());
    Boss->ConfigureEncounter(Arena,Player); Player->ConfigureBoss(Boss);
    ANushiEncounterManager* Manager=World->SpawnActor<ANushiEncounterManager>();
    Manager->SetNushi(Boss);
    UNushiProgressComponent* Progress=Boss->GetNushiProgressComponent();
    Manager->ResetEncounter();
    TestEqual(TEXT("Three actual Kakon registered"),Progress->GetRegisteredKakonCount(),3);
    TestEqual(TEXT("Ten dedicated route nodes"),AFuchimatoiBoss::RouteNodeCount,10);
    for(int32 Node=0;Node<10;++Node)
    {
        AFuchimatoiRouteAnchor* Anchor=Boss->GetRouteAnchor(Node);
        TestNotNull(TEXT("Route anchor exists"),Anchor);
        if(Anchor) TestEqual(TEXT("Only rock ledge and pillar are terrain"),Anchor->IsRock(),Node==4||Node==6);
    }
    TestTrue(TEXT("Primitive serpent initially about 20-30m"),Boss->GetBodyLength()>2000 && Boss->GetBodyLength()<3000);
    const FVector OriginalAnchor=Boss->GetRouteAnchor(3)->GetActorLocation();
    const FVector OriginalRock=Boss->GetRouteAnchor(4)->GetActorLocation();
    Boss->AddActorWorldOffset(FVector(40,70,10));
    TestTrue(TEXT("Snake anchor follows actor-local transform"),Boss->GetRouteAnchor(3)->GetActorLocation().Equals(OriginalAnchor+FVector(40,70,10),.01f));
    TestTrue(TEXT("Rock anchor stays on terrain"),Boss->GetRouteAnchor(4)->GetActorLocation().Equals(OriginalRock,.01f));
    Manager->ResetEncounter();
    for(int32 Cycle=0;Cycle<2;++Cycle)
    {
        Manager->StartEncounter();
        TestEqual(TEXT("Manager starts the second Nushi"),Boss->GetNushiState(),ENushiState::Active);
        Boss->BeginBiteWindup();
        Boss->SetBiteTargetLocalLocation(FVector(1200,0,0));
        Boss->BeginBiteLunge(); Boss->AdvanceBiteLunge(.5f);
        TestTrue(TEXT("Existing head proxy movement is reused"),Boss->GetHeadProxyLocalLocation().X>0);
        Boss->NotifyHeadSnagged();
        TestTrue(TEXT("First Kakon purifies once"),Boss->GetKakon(0)->Purify());
        TestFalse(TEXT("Duplicate Kakon purification rejected"),Boss->GetKakon(0)->Purify());
        TestTrue(TEXT("First Kakon enters existing Coiling state"),Boss->IsCoiling());
        TestEqual(TEXT("Progress is one of three"),Progress->GetPurifiedCount(),1);
        Boss->AdvanceCoiling(Boss->CoilingDuration);
        TestTrue(TEXT("Coiling progress completes"),Boss->IsCoilingComplete());
        Boss->GetKakon(1)->Purify(); Boss->GetKakon(2)->Purify();
        TestEqual(TEXT("All three shared Kakon calms Nushi"),Boss->GetNushiState(),ENushiState::Calm);
        TestEqual(TEXT("Manager observes completion"),Manager->GetEncounterState(),ENushiEncounterState::Completed);
        Manager->ResetEncounter();
        TestEqual(TEXT("Virtual Reset clears progress"),Progress->GetPurifiedCount(),0);
        TestEqual(TEXT("Virtual Reset restores Submerged"),Boss->GetActionState(),EFuchimatoiActionState::Submerged);
        TestEqual(TEXT("Virtual Reset restores Dormant"),Boss->GetNushiState(),ENushiState::Dormant);
        TestFalse(TEXT("Virtual Reset clears target"),Boss->HasBiteTarget());
        TestEqual(TEXT("Virtual Reset clears coil"),Boss->GetCoilingProgress(),0.f);
        TestTrue(TEXT("Virtual Reset restores snake route"),Boss->GetRouteAnchor(3)->GetActorLocation().Equals(OriginalAnchor,.01f));
        for(int32 I=0;I<3;++I) TestEqual(TEXT("Each Kakon exposed again"),Boss->GetKakon(I)->GetState(),EKakonState::Exposed);
    }
    World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
    return true;
}
#endif
