#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "MagatsuneBoss.h"
#include "MagatsunePlayer.h"
#include "GrabComponent.h"
#include "StaminaComponent.h"
#include "KakonActor.h"
#include "NushiEncounterManager.h"
#include "NushiProgressComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
namespace{struct FMagatsuneWorld{UWorld* W;FMagatsuneWorld(){W=UWorld::CreateWorld(EWorldType::Game,false);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);W->InitializeActorsForPlay(FURL());}~FMagatsuneWorld(){W->DestroyWorld(false);GEngine->DestroyWorldContext(W);}AMagatsuneBoss* Boss(){auto* B=W->SpawnActor<AMagatsuneBoss>();B->DispatchBeginPlay();return B;}};void Advance(AMagatsuneBoss* B,float Seconds,float Dt=.02f){for(float T=0;T<Seconds;T+=Dt)B->Tick(Dt);}}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagatsuneLifecycle,"IshibashiriPrototype.Nushi.Magatsune.MagatsuneLifecycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMagatsuneLifecycle::RunTest(const FString&){FMagatsuneWorld F;auto* B=F.Boss();auto* M=F.W->SpawnActor<ANushiEncounterManager>();M->SetNushi(B);M->StartEncounter();TestEqual(TEXT("0/3"),B->GetNushiProgressComponent()->GetPurifiedCount(),0);for(int I=0;I<3;++I){TestTrue(TEXT("exposed Kakon purifies"),B->GetKakon(I)->Purify());Advance(B,3);}TestEqual(TEXT("3/3"),B->GetNushiProgressComponent()->GetPurifiedCount(),3);TestEqual(TEXT("Calm"),B->GetNushiState(),ENushiState::Calm);TestEqual(TEXT("Completed"),M->GetEncounterState(),ENushiEncounterState::Completed);return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagatsuneFollow,"IshibashiriPrototype.Nushi.Magatsune.RootTransformFollow",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMagatsuneFollow::RunTest(const FString&){FMagatsuneWorld F;auto* B=F.Boss();auto* P=F.W->SpawnActor<ACharacter>();auto* G=NewObject<UGrabComponent>(P);G->RegisterComponent();P->SetActorLocation(B->GetRouteWorld(0));TestTrue(TEXT("shared grab"),G->TryGrab(B->GetGrabFrame(),1000));const FTransform R=G->GetRelativeGrabTransform();for(int FPS:{30,60,120})for(int N=0;N<FPS*4;++N){B->Tick(1.f/FPS);G->TickComponent(1.f/FPS,LEVELTICK_All,nullptr);TestTrue(TEXT("relative transform maintained"),P->GetActorTransform().GetRelativeTransform(B->GetGrabFrame()->GetActorTransform()).Equals(R,.01f));}return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagatsuneRoute,"IshibashiriPrototype.Nushi.Magatsune.RoutePhaseTransition",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMagatsuneRoute::RunTest(const FString&){FMagatsuneWorld F;auto* B=F.Boss();B->StartEncounter();TestFalse(TEXT("phase2 closed"),B->IsRouteNodeEnabled(4));B->GetKakon(0)->Purify();Advance(B,3);TestTrue(TEXT("phase2 open"),B->IsRouteNodeEnabled(4));TestFalse(TEXT("final closed"),B->IsRouteNodeEnabled(8));B->GetKakon(1)->Purify();Advance(B,3);TestTrue(TEXT("final open"),B->IsRouteNodeEnabled(8));return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagatsuneRecovery,"IshibashiriPrototype.Nushi.Magatsune.FallRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMagatsuneRecovery::RunTest(const FString&){FMagatsuneWorld F;auto* B=F.Boss();auto* P=F.W->SpawnActor<AMagatsunePlayer>();P->ConfigureBoss(B);B->ConfigurePlayer(P);B->StartEncounter();for(int I=0;I<2;++I){B->GetKakon(I)->Purify();Advance(B,3);P->SetActorLocation(B->GetRouteWorld(B->GetRecoveryNode()));P->GetGrab()->TryGrab(B->GetGrabFrame(),1000);P->Fall();TestEqual(TEXT("fall retains progress"),B->GetNushiProgressComponent()->GetPurifiedCount(),I+1);}return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagatsuneRetry,"IshibashiriPrototype.Nushi.Magatsune.RetryReset",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMagatsuneRetry::RunTest(const FString&){FMagatsuneWorld F;auto* B=F.Boss();auto* M=F.W->SpawnActor<ANushiEncounterManager>();M->SetNushi(B);const int32 Registered=B->GetNushiProgressComponent()->GetRegisteredKakonCount();for(int C=0;C<3;++C){M->StartEncounter();B->GetKakon(0)->Purify();M->ResetEncounter();TestEqual(TEXT("progress reset"),B->GetNushiProgressComponent()->GetPurifiedCount(),0);TestEqual(TEXT("route reset"),B->GetPhase(),EMagatsunePhase::SurfaceRoot);TestEqual(TEXT("no Kakon growth"),B->GetNushiProgressComponent()->GetRegisteredKakonCount(),Registered);TestTrue(TEXT("transform reset"),B->GetMovingRoot()->GetRelativeTransform().Equals(FTransform::Identity));}return true;}
#endif
