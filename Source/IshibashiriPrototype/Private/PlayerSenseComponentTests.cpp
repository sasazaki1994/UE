#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "PlayerSenseComponent.h"
#include "Engine/World.h"
#include "Tests/AutomationEditorCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBoundarySenseDirection,"IshibashiriPrototype.Sense.BoundarySenseDirection",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBoundarySenseDirection::RunTest(const FString&)
{
    UWorld* W=FAutomationEditorCommonUtils::CreateNewMap(); AActor* Player=W->SpawnActor<AActor>(); auto* Sense=NewObject<UPlayerSenseComponent>(Player); Sense->RegisterComponent();
    AActor* Target=W->SpawnActor<AActor>(); Sense->RegisterBoundaryTarget(Target); Sense->BeginBoundarySense();
    auto Read=[&](FVector P){Target->SetActorLocation(P);Sense->TickComponent(.1f,LEVELTICK_All,nullptr);return Sense->GetBoundaryReading().Strength;};
    const float Front=Read({100,0,0}),Right=Read({0,100,0}),Back=Read({-100,0,0});
    TestTrue(TEXT("front > right > back"),Front>Right&&Right>Back); TestTrue(TEXT("direction points right"),Read({0,100,0})>0&&Sense->GetBoundaryReading().Direction.Equals(FVector::RightVector,.01f)); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBoundarySensePhaseFilter,"IshibashiriPrototype.Sense.BoundarySensePhaseFilter",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBoundarySensePhaseFilter::RunTest(const FString&){UWorld* W=FAutomationEditorCommonUtils::CreateNewMap();AActor* P=W->SpawnActor<AActor>();auto* S=NewObject<UPlayerSenseComponent>(P);S->RegisterComponent();AActor* Locked=W->SpawnActor<AActor>();Locked->SetActorLocation({100,0,0});S->RegisterBoundaryTarget(Locked,false);S->BeginBoundarySense();S->TickComponent(.1f,LEVELTICK_All,nullptr);TestFalse(TEXT("locked target excluded"),S->GetBoundaryReading().Target.IsValid());S->SetBoundaryTargetAvailable(Locked,true);S->TickComponent(.1f,LEVELTICK_All,nullptr);TestTrue(TEXT("unlocked target selected"),S->GetBoundaryReading().Target==Locked);return true;}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCorruptionSenseWarningRiskReset,"IshibashiriPrototype.Sense.CorruptionSenseWarningRiskReset",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCorruptionSenseWarningRiskReset::RunTest(const FString&){UWorld* W=FAutomationEditorCommonUtils::CreateNewMap();AActor* P=W->SpawnActor<AActor>();auto* S=NewObject<UPlayerSenseComponent>(P);S->RegisterComponent();for(ECorruptionWarning V:{ECorruptionWarning::Danger,ECorruptionWarning::Transition,ECorruptionWarning::Safe}){S->BeginCorruptionSense();S->SetCorruptionWarning(V);TestEqual(TEXT("warning passed through"),S->GetCorruptionWarning(),V);S->EndCorruptionSense();}TestEqual(TEXT("held/tail recovery penalty"),S->GetRecoveryMultiplier(),.5f);S->TickComponent(2.01f,LEVELTICK_All,nullptr);TestEqual(TEXT("risk expires"),S->GetRecoveryMultiplier(),1.f);S->BeginBoundarySense();S->BeginCorruptionSense();S->ResetSense();TestFalse(TEXT("boundary reset"),S->IsBoundarySenseActive());TestFalse(TEXT("arm reset"),S->IsCorruptionSenseActive());TestFalse(TEXT("risk reset"),S->IsRecoveryRiskActive());TestFalse(TEXT("cached target reset"),S->GetBoundaryReading().Target.IsValid());return true;}
#endif
