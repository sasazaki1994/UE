#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ColossusClimbingComponent.h"
#include "GrabComponent.h"
#include "IshibashiriBoss.h"
#include "PrototypePlayer.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRouteClimbingLifecycleTest,
    "IshibashiriPrototype.Climbing.RouteLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRouteClimbingLifecycleTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    APrototypePlayer* Player = World->SpawnActor<APrototypePlayer>();
    Player->DispatchBeginPlay();
    UColossusClimbingComponent* Climbing = Player->GetClimbing();
    UCharacterMovementComponent* Movement = Player->GetCharacterMovement();

    // Isolate interruption recovery from the combat/mount input integration
    // suite. A mounted fixture owns disabled movement and the boss prerequisite.
    auto Mount = [&]()
    {
        AIshibashiriBoss* Boss = World->SpawnActor<AIshibashiriBoss>();
        Climbing->Boss = Boss;
        Climbing->Node = 0;
        Climbing->Destination = 1;
        Climbing->Progress = .5f;
        Climbing->bGripHeld = true;
        Climbing->IKWeight = 1.f;
        Climbing->AddTickPrerequisiteActor(Boss);
        Player->GetCapsuleComponent()->IgnoreActorWhenMoving(Boss, true);
        Movement->DisableMovement();
        Movement->bOrientRotationToMovement = false;
        return Boss;
    };
    auto CheckReleased = [&]()
    {
        TestFalse(TEXT("Interrupted route clears climbing"), Climbing->IsClimbing());
        TestFalse(TEXT("Interrupted route clears grip"), Climbing->IsGripping());
        TestEqual(TEXT("Interrupted route clears node"), Climbing->GetNode(), INDEX_NONE);
        TestEqual(TEXT("Interrupted route clears destination"), Climbing->GetDestination(), INDEX_NONE);
        TestEqual(TEXT("Interrupted route restores falling"), Movement->MovementMode, MOVE_Falling);
        TestTrue(TEXT("Interrupted route restores movement orientation"), Movement->bOrientRotationToMovement);
    };

    AIshibashiriBoss* Boss = Mount();
    TestTrue(TEXT("Mounted fixture starts attached"), Climbing->IsClimbing());
    TestTrue(TEXT("Target is destroyed while attached"), Boss->Destroy());
    TestFalse(TEXT("Pending-kill boss is not exposed as climbing"), Climbing->IsClimbing());
    TestFalse(TEXT("Pending-kill boss cannot be an IK target"), Climbing->IsIKVerticalSlice());
    TestFalse(TEXT("Pending-kill boss cannot be purified"), Climbing->TryPurify());
    // No encounter GameMode exists here: lifecycle cleanup must precede that guard.
    Climbing->TickComponent(.1f, LEVELTICK_All, nullptr);
    CheckReleased();

    Boss = Mount();
    Boss->Destroy();
    // Model the reflected actor reference being cleared by garbage collection
    // before the component gets its next tick; the node still owns movement.
    Climbing->Boss = nullptr;
    Climbing->TickComponent(.1f, LEVELTICK_All, nullptr);
    CheckReleased();

    Boss = Mount();
    Boss->Destroy();
    Climbing->Boss = nullptr;
    Climbing->Reset();
    CheckReleased();
    TestEqual(TEXT("Reset clears residual IK"), Climbing->GetIKWeight(), 0.f);

    Boss = World->SpawnActor<AIshibashiriBoss>();
    Climbing->StartFallbackGrabApproach(Boss);
    TestEqual(TEXT("Approach temporarily uses flying"), Movement->MovementMode, MOVE_Flying);
    Boss->Destroy();
    Climbing->TickComponent(.1f, LEVELTICK_All, nullptr);
    TestFalse(TEXT("Destroyed approach target cancels warp"), Climbing->IsGrabWarping());
    TestFalse(TEXT("Destroyed approach target clears fallback"), Climbing->IsFallbackGrabApproach());
    TestEqual(TEXT("Interrupted approach restores falling"), Movement->MovementMode, MOVE_Falling);

    Mount();
    Climbing->DestroyComponent();
    CheckReleased();
    TestEqual(TEXT("Component destruction clears residual IK"), Climbing->GetIKWeight(), 0.f);

    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenericGrabLifecycleTest,
    "IshibashiriPrototype.Climbing.GenericGrabLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenericGrabLifecycleTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    ACharacter* Character = World->SpawnActor<ACharacter>();
    UGrabComponent* Grab = NewObject<UGrabComponent>(Character);
    Character->AddInstanceComponent(Grab);
    Grab->RegisterComponent();
    Character->DispatchBeginPlay();
    auto SpawnTarget = [&]()
    {
        AActor* Target = World->SpawnActor<AActor>();
        USceneComponent* Root = NewObject<USceneComponent>(Target);
        Target->SetRootComponent(Root);
        Root->RegisterComponent();
        return Target;
    };
    TestFalse(TEXT("A character cannot become its own grab target"), Grab->TryGrab(Character, 100.f));
    AActor* Target = SpawnTarget();
    TestTrue(TEXT("Nearby target can be grabbed"), Grab->TryGrab(Target, 100.f));
    TestEqual(TEXT("Grab disables movement"), Character->GetCharacterMovement()->MovementMode, MOVE_None);
    Target->Destroy();
    Grab->TickComponent(.1f, LEVELTICK_All, nullptr);
    TestFalse(TEXT("Destroyed target releases generic grab"), Grab->IsGrabbing());
    TestNull(TEXT("Destroyed target reference is cleared"), Grab->GetGrabTarget());
    TestEqual(TEXT("Destroyed target restores falling"), Character->GetCharacterMovement()->MovementMode, MOVE_Falling);

    Target = SpawnTarget();
    TestTrue(TEXT("Can grab again after target destruction"), Grab->TryGrab(Target, 100.f));
    Grab->DestroyComponent();
    TestFalse(TEXT("Removing active component releases grab"), Grab->IsGrabbing());
    TestNull(TEXT("Removing active component clears target"), Grab->GetGrabTarget());
    TestEqual(TEXT("Removing active component restores falling"), Character->GetCharacterMovement()->MovementMode, MOVE_Falling);
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}

#endif
