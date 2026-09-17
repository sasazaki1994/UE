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
#include "GameFramework/CharacterMovementComponent.h"
#include <limits>

namespace
{
    struct FMinedakiWorld
    {
        UWorld* World;

        FMinedakiWorld()
        {
            World = UWorld::CreateWorld(EWorldType::Game, false);
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
            World->InitializeActorsForPlay(FURL());
        }

        ~FMinedakiWorld()
        {
            World->DestroyWorld(false);
            GEngine->DestroyWorldContext(World);
        }

        AMinedakiBoss* Boss()
        {
            auto* B = World->SpawnActor<AMinedakiBoss>();
            B->DispatchBeginPlay();
            return B;
        }
    };
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinedakiLifecycle, "IshibashiriPrototype.Nushi.Minedaki.ActionLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMinedakiLifecycle::RunTest(const FString& Parameters)
{
    FMinedakiWorld Fixture;
    auto* B = Fixture.Boss();
    auto* P = Fixture.World->SpawnActor<AMinedakiPlayer>();
    auto* M = Fixture.World->SpawnActor<ANushiEncounterManager>();
    M->SetNushi(B);
    P->ConfigureBoss(B);
    for (int32 FPS : {30, 60, 120})
    {
        M->ResetEncounter();
        M->StartEncounter();
        TestEqual(TEXT("Three common progress slots"), B->GetNushiProgressComponent()->GetRegisteredKakonCount(), 3);
        TestFalse(TEXT("Ground first Kakon is covered"), B->GetKakon(0)->Purify());
        P->SetActorLocation(B->GetRouteWorld(0));
        TestTrue(TEXT("Shared grab attaches"), P->GetGrab()->TryGrab(B->GetGrabFrame(), 1000));
        for (int32 Node = 0; Node <= 4; ++Node)
        {
            P->GetGrab()->SetRelativeGrabTransform(FTransform(FRotator(0, 180, 0), B->GetRouteLocal(Node)));
            B->NotifyRouteNode(Node);
        }
        TestEqual(TEXT("Back route triggers preparation"), B->GetActionState(), EMinedakiActionState::PreparingClimb);
        B->AdvanceWallClimb(-1);
        B->AdvanceWallClimb(std::numeric_limits<float>::quiet_NaN());
        TestEqual(TEXT("Invalid delta is ignored"), B->GetClimbTime(), 0.f);
        const FTransform Relative = P->GetGrab()->GetRelativeGrabTransform();
        float PeakPitch = 0;
        bool SawWall = false, SawShake = false, SawLedge = false;
        for (int32 Frame = 0; Frame < FPS * 13; ++Frame)
        {
            B->AdvanceWallClimb(1.f / FPS);
            P->GetGrab()->TickComponent(1.f / FPS, LEVELTICK_All, nullptr);
            const FTransform Actual = P->GetActorTransform().GetRelativeTransform(B->GetGrabFrame()->GetActorTransform());
            if (!Actual.Equals(Relative, .01f))
            {
                AddError(TEXT("Local transform drift during posture change"));
                break;
            }
            PeakPitch = FMath::Max(PeakPitch, static_cast<float>(B->GetBodyRoot()->GetRelativeRotation().Pitch));
            SawWall |= B->GetActionState() == EMinedakiActionState::ClimbingWall;
            SawShake |= B->GetActionState() == EMinedakiActionState::Shaking;
            SawLedge |= B->GetActionState() == EMinedakiActionState::LedgeTransition;
        }
        TestTrue(TEXT("All climb stages and seventy-degree pitch observed"), SawWall && SawShake && SawLedge && PeakPitch >= 69);
        TestEqual(TEXT("Upper platform reached exactly once"), B->Telemetry.UpperReached, 1);
        TestEqual(TEXT("Shake occurs exactly once"), B->Telemetry.Shakes, 1);
        TestTrue(TEXT("Boss moved up twenty meters"), FMath::IsNearlyEqual(B->GetActorLocation().Z, 2000., .01));
        TestTrue(TEXT("First Kakon exposed after wall traversal"), B->GetKakon(0)->Purify());
        TestFalse(TEXT("Duplicate purification rejected"), B->GetKakon(0)->Purify());
        TestEqual(TEXT("Common progress one"), B->GetNushiProgressComponent()->GetPurifiedCount(), 1);
        TestEqual(TEXT("Not calm"), B->GetNushiState(), ENushiState::Active);
        TestEqual(TEXT("Not encounter completed"), M->GetEncounterState(), ENushiEncounterState::Running);
        for (int32 I = 1; I < 3; ++I)
        {
            TestFalse(TEXT("Future Kakon cannot purify"), B->GetKakon(I)->Purify());
            TestTrue(TEXT("Future Kakon hidden"), B->GetKakon(I)->IsHidden());
        }
        P->ResetForEncounter();
        M->ResetEncounter();
        TestEqual(TEXT("Reset progress"), B->GetNushiProgressComponent()->GetPurifiedCount(), 0);
        TestTrue(TEXT("Reset body orientation"), B->GetBodyRoot()->GetRelativeRotation().IsNearlyZero());
        TestTrue(TEXT("Reset actor position"), B->GetActorLocation().IsNearlyZero());
    }
    M->StartEncounter();
    B->NotifyRouteNode(4);
    B->AdvanceWallClimb(1000);
    TestEqual(TEXT("Large delta crosses shake once"), B->Telemetry.Shakes, 1);
    TestEqual(TEXT("Large delta completes climb"), B->GetActionState(), EMinedakiActionState::UpperPlatform);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinedakiTransform, "IshibashiriPrototype.Nushi.Minedaki.LocalTransformFollow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMinedakiTransform::RunTest(const FString& Parameters)
{
    // No mesh or asset required: character, scene hierarchy and shared UGrab only.
    FMinedakiWorld Fixture;
    auto* Parent = Fixture.World->SpawnActor<AActor>();
    auto* Root = NewObject<USceneComponent>(Parent);
    Parent->SetRootComponent(Root);
    Root->RegisterComponent();
    auto* Body = NewObject<USceneComponent>(Parent);
    Body->SetupAttachment(Root);
    Body->RegisterComponent();
    auto* Anchor = Fixture.World->SpawnActor<AActor>();
    auto* AnchorRoot = NewObject<USceneComponent>(Anchor);
    Anchor->SetRootComponent(AnchorRoot);
    AnchorRoot->RegisterComponent();
    Anchor->AttachToComponent(Body, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    auto* P = Fixture.World->SpawnActor<ACharacter>();
    auto* Grab = NewObject<UGrabComponent>(P);
    Grab->RegisterComponent();
    P->SetActorTransform(FTransform(FRotator(12, 35, 8), FVector(120, 70, 450)));
    TestTrue(TEXT("Grab"), Grab->TryGrab(Anchor, 1000));
    const FTransform Relative = Grab->GetRelativeGrabTransform();
    float MaxError = 0, MaxAngle = 0;
    for (int32 FPS : {30, 60, 120})
    {
        for (int32 Frame = 0; Frame <= FPS * 3; ++Frame)
        {
            float T = static_cast<float>(Frame) / (FPS * 3);
            Parent->SetActorTransform(FTransform(FRotator(0, 20 * T, 0), FVector(-800 * T, 300 * T, 2000 * T)));
            Body->SetRelativeRotation(FRotator(70 * T, 0, 24 * FMath::Sin(T * PI)));
            Grab->TickComponent(1.f / FPS, LEVELTICK_All, nullptr);
            FTransform Actual = P->GetActorTransform().GetRelativeTransform(Anchor->GetActorTransform());
            MaxError = FMath::Max(MaxError, static_cast<float>(FVector::Dist(Actual.GetLocation(), Relative.GetLocation())));
            MaxAngle = FMath::Max(
                MaxAngle, static_cast<float>(FMath::RadiansToDegrees(Actual.GetRotation().AngularDistance(Relative.GetRotation()))));
        }
    }
    TestTrue(TEXT("Translation local error below 0.01cm"), MaxError < .01f);
    TestTrue(TEXT("Rotation local error below 0.01degree"), MaxAngle < .01f);
    AddInfo(FString::Printf(TEXT("Peak local drift %.8f cm, %.8f degrees; endpoint Pitch70 Yaw20"), MaxError, MaxAngle));
    Anchor->Destroy();
    Grab->TickComponent(.016f, LEVELTICK_All, nullptr);
    TestFalse(TEXT("Destroyed target safely releases"), Grab->IsGrabbing());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinedakiExhaustion, "IshibashiriPrototype.Nushi.Minedaki.ExhaustionRetry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMinedakiExhaustion::RunTest(const FString& Parameters)
{
    FMinedakiWorld Fixture;
    auto* B = Fixture.Boss();
    auto* P = Fixture.World->SpawnActor<AMinedakiPlayer>();
    P->ConfigureBoss(B);
    B->StartEncounter();
    P->GetGrab()->TryGrab(B->GetGrabFrame(), 1000);
    P->GetStamina()->ConsumeStamina(100);
    P->Tick(.016f);
    TestTrue(TEXT("Stamina exhaustion releases and marks fall"), !P->IsMounted() && P->HasFallen() && B->Telemetry.Exhaustions == 1);
    P->ResetForEncounter();
    B->ResetNushi();
    TestTrue(TEXT("Retry restores stamina and failure flags"),
        !P->HasFallen() && P->GetStamina()->GetCurrentStamina() == 100 && B->Telemetry.Exhaustions == 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinedakiFullEncounter, "IshibashiriPrototype.Nushi.Minedaki.FullEncounterLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMinedakiFullEncounter::RunTest(const FString& Parameters)
{
    FMinedakiWorld F;
    auto* B = F.Boss();
    auto* M = F.World->SpawnActor<ANushiEncounterManager>();
    M->SetNushi(B);
    M->StartEncounter();
    B->NotifyRouteNode(4);
    B->AdvanceWallClimb(1000);
    for (int32 I = 0; I < 3; ++I)
    {
        TestTrue(TEXT("Current Kakon exposed"), B->GetKakon(I)->GetState() == EKakonState::Exposed);
        TestTrue(TEXT("Purification accepted"), B->GetKakon(I)->Purify());
        if (I < 2)
            for (int32 N = 0; N < 200; ++N) B->Tick(.02f);
    }
    TestEqual(TEXT("Progress 3/3"), B->GetNushiProgressComponent()->GetPurifiedCount(), 3);
    TestEqual(TEXT("Nushi Calm"), B->GetNushiState(), ENushiState::Calm);
    TestEqual(TEXT("Encounter Completed"), M->GetEncounterState(), ENushiEncounterState::Completed);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinedakiBodyRoute, "IshibashiriPrototype.Nushi.Minedaki.BodyRouteTransition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMinedakiBodyRoute::RunTest(const FString& Parameters)
{
    FMinedakiWorld F;
    auto* B = F.Boss();
    B->StartEncounter();
    B->NotifyRouteNode(4);
    B->AdvanceWallClimb(1000);
    TestFalse(TEXT("Arm route initially closed"), B->IsRouteNodeEnabled(8));
    // This isolates transform following after attach, not ground grab reach. High route
    // points are more than 1000cm from the actor origin used by the shared Grab component.
    auto* P = F.World->SpawnActor<ACharacter>();
    auto* G = NewObject<UGrabComponent>(P);
    G->RegisterComponent();
    P->SetActorLocation(B->GetRouteWorld(6));
    TestTrue(TEXT("Shared grab"),
        G->TryGrab(B->GetGrabFrame(), FVector::Dist(P->GetActorLocation(), B->GetGrabFrame()->GetActorLocation()) + 1.f));
    const FTransform Relative = G->GetRelativeGrabTransform();
    B->GetKakon(0)->Purify();
    for (int32 N = 0; N < 200; ++N)
    {
        B->Tick(.02f);
        G->TickComponent(.02f, LEVELTICK_All, nullptr);
    }
    TestTrue(TEXT("Body posture opens arm route"), B->IsRouteNodeEnabled(8));
    TestTrue(TEXT("Relative transform survives body transition"),
        P->GetActorTransform().GetRelativeTransform(B->GetGrabFrame()->GetActorTransform()).Equals(Relative, .01f));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinedakiFallRecovery, "IshibashiriPrototype.Nushi.Minedaki.FallRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMinedakiFallRecovery::RunTest(const FString& Parameters)
{
    FMinedakiWorld F;
    auto* B = F.Boss();
    auto* P = F.World->SpawnActor<AMinedakiPlayer>();
    P->ConfigureBoss(B);
    B->ConfigurePlayer(P);
    B->StartEncounter();
    B->NotifyRouteNode(4);
    B->AdvanceWallClimb(1000);
    for (int32 Progress = 1; Progress <= 2; ++Progress)
    {
        if (Progress == 1) B->GetKakon(0)->Purify();
        else B->GetKakon(1)->Purify();
        for (int32 N = 0; N < 200; ++N) B->Tick(.02f);
        P->SetActorLocation(B->GetRouteWorld(B->GetRecoveryNode()));
        TestTrue(TEXT("Recovery fixture attaches before fall"),
            P->GetGrab()->TryGrab(B->GetGrabFrame(), FVector::Dist(P->GetActorLocation(), B->GetGrabFrame()->GetActorLocation()) + 1.f));
        P->Fall();
        TestEqual(TEXT("Fall preserves progress"), B->GetNushiProgressComponent()->GetPurifiedCount(), Progress);
        P->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        P->Tick(.02f);
        TestTrue(TEXT("Recovery anchor becomes active"), P->IsRecovering());
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinedakiRetryReset, "IshibashiriPrototype.Nushi.Minedaki.RetryReset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMinedakiRetryReset::RunTest(const FString& Parameters)
{
    FMinedakiWorld F;
    auto* B = F.Boss();
    auto* M = F.World->SpawnActor<ANushiEncounterManager>();
    M->SetNushi(B);
    const int32 Registered = B->GetNushiProgressComponent()->GetRegisteredKakonCount();
    for (int32 Cycle = 0; Cycle < 3; ++Cycle)
    {
        M->StartEncounter();
        B->NotifyRouteNode(4);
        B->AdvanceWallClimb(1000);
        B->GetKakon(0)->Purify();
        M->ResetEncounter();
        TestEqual(TEXT("Retry progress zero"), B->GetNushiProgressComponent()->GetPurifiedCount(), 0);
        TestEqual(TEXT("No registered actor growth"), B->GetNushiProgressComponent()->GetRegisteredKakonCount(), Registered);
        TestEqual(TEXT("Route reset"), B->GetActionState(), EMinedakiActionState::Grounded);
    }
    return true;
}
#endif
