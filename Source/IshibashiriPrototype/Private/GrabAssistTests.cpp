#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ColossusClimbingComponent.h"
#include "IshibashiriBoss.h"
#include "PrototypeGameMode.h"
#include "PrototypePlayer.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrabAssistTest,
    "IshibashiriPrototype.Climbing.GrabAssist",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGrabAssistTest::RunTest(const FString& Parameters)
{
    // This is a deterministic actor/component fixture, not an input-device or
    // rendered playthrough. Use the real auth GameMode so its encounter gates
    // run, but do not StartPlay (which creates the full arena and local player).
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    FURL URL;
    URL.AddOption(TEXT("game=/Script/IshibashiriPrototype.PrototypeGameMode"));
    World->SetGameMode(URL);
    World->InitializeActorsForPlay(URL);
    APrototypeGameMode* Mode = World->GetAuthGameMode<APrototypeGameMode>();
    if (!TestNotNull(TEXT("Fixture has the actual encounter GameMode"), Mode))
    {
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
        return false;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    APrototypePlayer* Player = World->SpawnActor<APrototypePlayer>(APrototypePlayer::StaticClass(), FTransform::Identity, SpawnParams);
    AIshibashiriBoss* Boss = World->SpawnActor<AIshibashiriBoss>(AIshibashiriBoss::StaticClass(), FTransform::Identity, SpawnParams);
    Mode->Player = Player;
    Mode->Boss = Boss;
    Player->DispatchBeginPlay();
    Boss->DispatchBeginPlay();
    Boss->ConfigureEncounter(FTransform::Identity, Player);
    UColossusClimbingComponent* Climbing = Player->GetClimbing();
    UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
    // Keep the optional animation assets out of this fixture. The fallback must
    // enforce the same eligibility and complete through the same node-0 path.
    Player->GrabMotionWarpMontage = nullptr;
    Player->GrabMotionWarpAnimClass = nullptr;

    auto Place = [&](float Distance, float Angle = 0.f)
    {
        const FTransform Target = Climbing->MakeGrabWarpTarget(Boss);
        Player->SetActorLocation(Target.GetLocation() + FVector(Distance, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
        Player->SetActorRotation(FRotator(0.f, Target.Rotator().Yaw + Angle, 0.f));
    };
    auto Prepare = [&](float Distance = 100.f, float Angle = 0.f)
    {
        Mode->Result = EEncounterResult::Playing;
        Mode->Boss = Boss;
        Player->MaxHealth = 3;
        Player->ResetForEncounter(FTransform::Identity);
        Boss->ResetNushi();
        Boss->BeginFallRecoveryWindow(5.f);
        Place(Distance, Angle);
    };
    auto Tick = [&](float Dt)
    {
        Climbing->TickComponent(Dt, LEVELTICK_All, nullptr);
    };
    auto CheckCleared = [&](const TCHAR* Reason)
    {
        TestEqual(FString::Printf(TEXT("%s clears request lifetime"), Reason), Climbing->GrabBufferRemaining, 0.f);
        TestFalse(FString::Printf(TEXT("%s clears request target"), Reason), Climbing->BufferedGrabBoss.IsValid());
        TestFalse(FString::Printf(TEXT("%s cannot start an approach"), Reason), Climbing->IsGrabWarping());
        TestFalse(FString::Printf(TEXT("%s cannot mount"), Reason), Climbing->IsClimbing());
    };
    auto QueueFacingRequest = [&]()
    {
        Prepare(100.f, 150.f);
        Player->BeginGrab();
        Player->ReleaseGrab();
        TestTrue(TEXT("An eligible short press records a pending request"), Climbing->GrabBufferRemaining > 0.f);
        TestFalse(TEXT("A pending request does not bypass facing"), Climbing->IsGrabWarping());
        TestFalse(TEXT("A buffered refusal reports actionable feedback"), Climbing->GetGrabFeedback().IsEmpty());
    };

    TestEqual(TEXT("Default grab reach is 300 cm"), Climbing->GrabRange, 300.f);
    TestEqual(TEXT("Default warp limit is 300 cm"), Climbing->MaximumWarpDistance, 300.f);
    TestEqual(TEXT("Default facing limit is 120 degrees"), Climbing->MaximumWarpAngle, 120.f);
    TestEqual(TEXT("Default input buffer is 0.25 seconds"), Climbing->GrabBufferSeconds, .25f);

    Prepare(300.f);
    TestTrue(TEXT("Distance is measured to the authored hold"), FMath::IsNearlyEqual(Climbing->GetGrabDistance(Boss), 300.f, .001f));
    Player->BeginGrab();
    TestTrue(TEXT("Exactly 300 cm starts the fallback approach"), Climbing->IsFallbackGrabApproach());
    TestEqual(TEXT("Starting consumes the pending request"), Climbing->GrabBufferRemaining, 0.f);
    Prepare(300.1f);
    Player->BeginGrab();
    CheckCleared(TEXT("Outside 300 cm"));
    TestFalse(TEXT("Direct retry cannot bypass the distance limit"), Climbing->TryStartGrab(Boss));
    Climbing->MaximumWarpDistance = 180.f;
    Prepare(180.1f);
    Player->BeginGrab();
    CheckCleared(TEXT("Outside a lower configured warp cap"));
    TestFalse(TEXT("Fallback cannot bypass the lower warp cap"), Climbing->TryStartGrab(Boss));
    Prepare(180.f);
    Player->BeginGrab();
    TestTrue(TEXT("The lower warp cap is inclusive"), Climbing->IsGrabWarping());
    Climbing->MaximumWarpDistance = 300.f;

    for (float Angle : {-120.f, 120.f})
    {
        Prepare(100.f, Angle);
        Player->BeginGrab();
        TestTrue(FString::Printf(TEXT("Facing boundary %.0f degrees is accepted"), Angle), Climbing->IsGrabWarping());
    }
    Prepare(100.f, 120.1f);
    Player->BeginGrab();
    TestFalse(TEXT("Facing beyond 120 degrees cannot start either approach"), Climbing->IsGrabWarping());
    TestFalse(TEXT("Direct retry cannot bypass the facing limit"), Climbing->TryStartGrab(Boss));
    TestTrue(TEXT("An in-range facing refusal remains buffered"), Climbing->GrabBufferRemaining > 0.f);

    QueueFacingRequest();
    Place(100.f);
    Tick(.249f);
    TestTrue(TEXT("A released short press succeeds before the deadline"), Climbing->IsGrabWarping());
    TestFalse(TEXT("Buffered success does not invent a held grip"), Climbing->IsGripping());
    QueueFacingRequest();
    Place(100.f);
    Tick(.25f);
    CheckCleared(TEXT("Exactly 0.25 seconds"));
    Tick(.01f);
    CheckCleared(TEXT("A later frame cannot revive an expired request"));
    QueueFacingRequest();
    Place(100.f);
    Tick(.5f);
    CheckCleared(TEXT("A frame hitch past the deadline"));

    Prepare();
    Player->Attack();
    Player->Tick(Player->AttackDuration - .1f);
    Player->BeginGrab();
    Player->ReleaseGrab();
    TestTrue(TEXT("Grab during an attack reaches the buffer"), Climbing->GrabBufferRemaining > 0.f);
    TestFalse(TEXT("Grab does not interrupt an active attack"), Climbing->IsGrabWarping());
    Player->Tick(.11f);
    Tick(.11f);
    TestTrue(TEXT("Attack recovery within the buffer starts the approach"), Climbing->IsGrabWarping());

    Prepare();
    Player->Dodge();
    Player->Tick(Player->DodgeDuration - .1f);
    Place(100.f);
    Player->BeginGrab();
    Player->ReleaseGrab();
    TestTrue(TEXT("Route Grab during a dodge reaches the buffer"), Climbing->GrabBufferRemaining > 0.f);
    TestFalse(TEXT("Grab does not interrupt an active dodge"), Climbing->IsGrabWarping());
    Player->Tick(.11f);
    Place(100.f);
    Tick(.11f);
    TestTrue(TEXT("Dodge recovery within the buffer starts the approach"), Climbing->IsGrabWarping());

    Prepare();
    Movement->SetMovementMode(MOVE_Falling);
    Player->BeginGrab();
    Player->ReleaseGrab();
    TestFalse(TEXT("Falling cannot immediately grab"), Climbing->IsGrabWarping());
    Movement->SetMovementMode(MOVE_Walking);
    Tick(.1f);
    TestTrue(TEXT("Landing within the buffer permits the pending grab"), Climbing->IsGrabWarping());
    Prepare();
    Climbing->Stamina = Climbing->MinimumGrabStamina - 1.f;
    Player->BeginGrab();
    Player->ReleaseGrab();
    TestFalse(TEXT("Low stamina cannot immediately grab"), Climbing->IsGrabWarping());
    Tick(.08f);
    Tick(.02f);
    TestTrue(TEXT("Ground stamina recovery can satisfy the pending grab"), Climbing->IsGrabWarping());

    QueueFacingRequest();
    Climbing->Reset();
    Place(100.f);
    Tick(.01f);
    CheckCleared(TEXT("Reset"));
    QueueFacingRequest();
    Climbing->Detach(false);
    Place(100.f);
    Tick(.01f);
    CheckCleared(TEXT("Detach"));
    QueueFacingRequest();
    Mode->Result = EEncounterResult::Defeat;
    Place(100.f);
    Tick(.01f);
    CheckCleared(TEXT("Inactive encounter"));
    Mode->Result = EEncounterResult::Playing;
    Tick(.01f);
    CheckCleared(TEXT("Reactivated encounter cannot revive the request"));
    QueueFacingRequest();
    Place(300.1f);
    Tick(.01f);
    CheckCleared(TEXT("Leaving grab range"));
    Place(100.f);
    Tick(.01f);
    CheckCleared(TEXT("Returning to range cannot revive the request"));
    QueueFacingRequest();
    Boss->BeginFallRecoveryWindow(.01f);
    Boss->Tick(.02f);
    Tick(.01f);
    CheckCleared(TEXT("Kneel window ending"));

    QueueFacingRequest();
    AIshibashiriBoss* Replacement = World->SpawnActor<AIshibashiriBoss>(AIshibashiriBoss::StaticClass(), FTransform::Identity, SpawnParams);
    Replacement->DispatchBeginPlay();
    Replacement->BeginFallRecoveryWindow(5.f);
    Mode->Boss = Replacement;
    Place(100.f);
    Tick(.01f);
    CheckCleared(TEXT("A different boss becoming the current target"));
    Mode->Boss = Boss;
    Tick(.01f);
    CheckCleared(TEXT("Restoring the old boss cannot revive the request"));
    Replacement->Destroy();

    Prepare();
    Boss->ResetNushi();
    Player->BeginGrab();
    CheckCleared(TEXT("Press outside Kneel"));
    Boss->BeginFallRecoveryWindow(5.f);
    Tick(.01f);
    CheckCleared(TEXT("Entering Kneel after an ineligible press"));
    Prepare();
    Mode->Result = EEncounterResult::Victory;
    Climbing->GrabPressed();
    CheckCleared(TEXT("Press after victory"));
    Prepare();
    Player->MaxHealth = 0;
    Player->ResetForEncounter(FTransform::Identity);
    Place(100.f);
    Climbing->GrabPressed();
    CheckCleared(TEXT("Dead player"));
    TestFalse(TEXT("Direct retry cannot bypass the health gate"), Climbing->TryStartGrab(Boss));
    Prepare();
    Player->MaxHealth = 1;
    Player->ResetForEncounter(FTransform::Identity);
    Place(100.f, 150.f);
    Player->BeginGrab();
    TestTrue(TEXT("A live player can queue before fatal damage"), Climbing->GrabBufferRemaining > 0.f);
    TestTrue(TEXT("Fatal damage reaches the normal damage path"), Player->ReceiveChargeHit(Boss->GetActorLocation()));
    TestEqual(TEXT("Fatal damage reduced health to zero"), Player->GetHealth(), 0);
    Tick(.01f);
    CheckCleared(TEXT("Death while buffered"));

    // Sample observable state transitions at several update rates. Repeated
    // presses must not restart the approach or complete the node twice.
    for (float Hz : {30.f, 60.f, 120.f})
    {
        QueueFacingRequest();
        const float Dt = 1.f / Hz;
        Tick(Dt);
        Place(100.f);
        int32 MountTransitions = 0;
        bool bWasClimbing = false;
        for (int32 Frame = 0; Frame < FMath::CeilToInt(Hz); ++Frame)
        {
            Tick(Dt);
            const bool bNowClimbing = Climbing->IsClimbing();
            if (bNowClimbing && !bWasClimbing) ++MountTransitions;
            bWasClimbing = bNowClimbing;
            if (Climbing->IsGrabWarping() || Climbing->IsClimbing()) Player->BeginGrab();
        }
        TestEqual(FString::Printf(TEXT("%.0f Hz completes exactly one mount"), Hz), MountTransitions, 1);
        TestEqual(FString::Printf(TEXT("%.0f Hz completes at authored node 0"), Hz), Climbing->GetNode(), 0);
        TestFalse(TEXT("Completed approach no longer owns a warp"), Climbing->IsGrabWarping());
        TestTrue(TEXT("The boss records the completed mount"), Boss->IsMountCommitted());
        Climbing->Detach(false);
        Movement->SetMovementMode(MOVE_Walking);
        Place(100.f);
        Tick(.1f);
        CheckCleared(TEXT("Held/repeated presses cannot remount after detach"));
    }

    // Destroy the buffered participant last so the shared fixture needs no
    // replacement actor and pending-kill validity follows the real UE path.
    QueueFacingRequest();
    TestTrue(TEXT("Buffered boss can be destroyed"), Boss->Destroy());
    Tick(.01f);
    CheckCleared(TEXT("Destroyed target"));

    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}

#endif
