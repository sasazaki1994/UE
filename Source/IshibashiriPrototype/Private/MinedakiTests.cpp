#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "MinedakiBoss.h"
#include "MinedakiPlayer.h"
#include "KakonActor.h"
#include "GrabComponent.h"
#include "StaminaComponent.h"
#include "NushiEncounterManager.h"
#include "NushiProgressComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include <limits>
namespace
{
struct FMinedakiWorld
{
    UWorld* World;
    FMinedakiWorld() { World=UWorld::CreateWorld(EWorldType::Game,false); GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World); World->InitializeActorsForPlay(FURL()); }
    ~FMinedakiWorld() { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); }
    AMinedakiBoss* Boss() { auto* B=World->SpawnActor<AMinedakiBoss>(); B->DispatchBeginPlay(); return B; }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinedakiLifecycle,"IshibashiriPrototype.Nushi.Minedaki.ActionLifecycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMinedakiLifecycle::RunTest(const FString& Parameters)
{
    FMinedakiWorld Fixture; auto* B=Fixture.Boss(); auto* P=Fixture.World->SpawnActor<AMinedakiPlayer>();
    auto* M=Fixture.World->SpawnActor<ANushiEncounterManager>(); M->SetNushi(B); P->ConfigureBoss(B);
    for(int32 FPS:{30,60,120})
    {
        M->ResetEncounter(); M->StartEncounter();
        TestEqual(TEXT("Three common progress slots"),B->GetNushiProgressComponent()->GetRegisteredKakonCount(),3);
        TestFalse(TEXT("Ground first Kakon is covered"),B->GetKakon(0)->Purify());
        P->SetActorLocation(B->GetRouteWorld(0));
        TestTrue(TEXT("Shared grab attaches"),P->GetGrab()->TryGrab(B->GetGrabFrame(),1000));
        for(int32 Node=0;Node<=4;++Node)
        { P->GetGrab()->SetRelativeGrabTransform(FTransform(FRotator(0,180,0),B->GetRouteLocal(Node))); B->NotifyRouteNode(Node); }
        TestEqual(TEXT("Back route triggers preparation"),B->GetActionState(),EMinedakiActionState::PreparingClimb);
        B->AdvanceWallClimb(-1); B->AdvanceWallClimb(std::numeric_limits<float>::quiet_NaN());
        TestEqual(TEXT("Invalid delta is ignored"),B->GetClimbTime(),0.f);
        const FTransform Relative=P->GetGrab()->GetRelativeGrabTransform();
        float PeakPitch=0; bool SawWall=false, SawShake=false, SawLedge=false;
        for(int32 Frame=0;Frame<FPS*13;++Frame)
        {
            B->AdvanceWallClimb(1.f/FPS); P->GetGrab()->TickComponent(1.f/FPS,LEVELTICK_All,nullptr);
            const FTransform Actual=P->GetActorTransform().GetRelativeTransform(B->GetGrabFrame()->GetActorTransform());
            if(!Actual.Equals(Relative,.01f)) { AddError(TEXT("Local transform drift during posture change")); break; }
            PeakPitch=FMath::Max(PeakPitch,static_cast<float>(B->GetBodyRoot()->GetRelativeRotation().Pitch));
            SawWall|=B->GetActionState()==EMinedakiActionState::ClimbingWall;
            SawShake|=B->GetActionState()==EMinedakiActionState::Shaking;
            SawLedge|=B->GetActionState()==EMinedakiActionState::LedgeTransition;
        }
        TestTrue(TEXT("All climb stages and seventy-degree pitch observed"),SawWall && SawShake && SawLedge && PeakPitch>=69);
        TestEqual(TEXT("Upper platform reached exactly once"),B->Telemetry.UpperReached,1);
        TestEqual(TEXT("Shake occurs exactly once"),B->Telemetry.Shakes,1);
        TestTrue(TEXT("Boss moved up twenty meters"),FMath::IsNearlyEqual(B->GetActorLocation().Z,2000.,.01));
        TestTrue(TEXT("First Kakon exposed after wall traversal"),B->GetKakon(0)->Purify());
        TestFalse(TEXT("Duplicate purification rejected"),B->GetKakon(0)->Purify());
        TestEqual(TEXT("Common progress one"),B->GetNushiProgressComponent()->GetPurifiedCount(),1);
        TestEqual(TEXT("Not calm"),B->GetNushiState(),ENushiState::Active);
        TestEqual(TEXT("Not encounter completed"),M->GetEncounterState(),ENushiEncounterState::Running);
        for(int32 I=1;I<3;++I) { TestFalse(TEXT("Future Kakon cannot purify"),B->GetKakon(I)->Purify()); TestTrue(TEXT("Future Kakon hidden"),B->GetKakon(I)->IsHidden()); }
        P->ResetForEncounter(); M->ResetEncounter();
        TestEqual(TEXT("Reset progress"),B->GetNushiProgressComponent()->GetPurifiedCount(),0);
        TestTrue(TEXT("Reset body orientation"),B->GetBodyRoot()->GetRelativeRotation().IsNearlyZero());
        TestTrue(TEXT("Reset actor position"),B->GetActorLocation().IsNearlyZero());
    }
    M->StartEncounter(); B->NotifyRouteNode(4); B->AdvanceWallClimb(1000);
    TestEqual(TEXT("Large delta crosses shake once"),B->Telemetry.Shakes,1);
    TestEqual(TEXT("Large delta completes climb"),B->GetActionState(),EMinedakiActionState::UpperPlatform);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinedakiTransform,"IshibashiriPrototype.Nushi.Minedaki.LocalTransformFollow",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMinedakiTransform::RunTest(const FString& Parameters)
{
    // No mesh or asset required: character, scene hierarchy and shared UGrab only.
    FMinedakiWorld Fixture; auto* Parent=Fixture.World->SpawnActor<AActor>();
    auto* Root=NewObject<USceneComponent>(Parent); Parent->SetRootComponent(Root); Root->RegisterComponent();
    auto* Body=NewObject<USceneComponent>(Parent); Body->SetupAttachment(Root); Body->RegisterComponent();
    auto* Anchor=Fixture.World->SpawnActor<AActor>(); auto* AnchorRoot=NewObject<USceneComponent>(Anchor);
    Anchor->SetRootComponent(AnchorRoot); AnchorRoot->RegisterComponent(); Anchor->AttachToComponent(Body,FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    auto* P=Fixture.World->SpawnActor<ACharacter>(); auto* Grab=NewObject<UGrabComponent>(P); Grab->RegisterComponent();
    P->SetActorTransform(FTransform(FRotator(12,35,8),FVector(120,70,450)));
    TestTrue(TEXT("Grab"),Grab->TryGrab(Anchor,1000)); const FTransform Relative=Grab->GetRelativeGrabTransform();
    float MaxError=0, MaxAngle=0;
    for(int32 FPS:{30,60,120})
    {
        for(int32 Frame=0;Frame<=FPS*3;++Frame)
        {
            float T=static_cast<float>(Frame)/(FPS*3);
            Parent->SetActorTransform(FTransform(FRotator(0,20*T,0),FVector(-800*T,300*T,2000*T)));
            Body->SetRelativeRotation(FRotator(70*T,0,24*FMath::Sin(T*PI)));
            Grab->TickComponent(1.f/FPS,LEVELTICK_All,nullptr);
            FTransform Actual=P->GetActorTransform().GetRelativeTransform(Anchor->GetActorTransform());
            MaxError=FMath::Max(MaxError,static_cast<float>(FVector::Dist(Actual.GetLocation(),Relative.GetLocation())));
            MaxAngle=FMath::Max(MaxAngle,static_cast<float>(FMath::RadiansToDegrees(Actual.GetRotation().AngularDistance(Relative.GetRotation()))));
        }
    }
    TestTrue(TEXT("Translation local error below 0.01cm"),MaxError<.01f);
    TestTrue(TEXT("Rotation local error below 0.01degree"),MaxAngle<.01f);
    AddInfo(FString::Printf(TEXT("Peak local drift %.8f cm, %.8f degrees; endpoint Pitch70 Yaw20"),MaxError,MaxAngle));
    Anchor->Destroy(); Grab->TickComponent(.016f,LEVELTICK_All,nullptr);
    TestFalse(TEXT("Destroyed target safely releases"),Grab->IsGrabbing());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinedakiExhaustion,"IshibashiriPrototype.Nushi.Minedaki.ExhaustionRetry",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMinedakiExhaustion::RunTest(const FString& Parameters)
{
    FMinedakiWorld Fixture; auto* B=Fixture.Boss(); auto* P=Fixture.World->SpawnActor<AMinedakiPlayer>();
    P->ConfigureBoss(B); B->StartEncounter();
    P->GetGrab()->TryGrab(B->GetGrabFrame(),1000); P->GetStamina()->ConsumeStamina(100);
    P->Tick(.016f);
    TestTrue(TEXT("Stamina exhaustion releases and marks fall"),!P->IsMounted() && P->HasFallen() && B->Telemetry.Exhaustions==1);
    P->ResetForEncounter(); B->ResetNushi();
    TestTrue(TEXT("Retry restores stamina and failure flags"),!P->HasFallen() && P->GetStamina()->GetCurrentStamina()==100 && B->Telemetry.Exhaustions==0);
    return true;
}
#endif
