#include "PrototypeGrabTest.h"
#include "GrabComponent.h"
#include "IshibashiriBoss.h"
#include "PrototypeGameMode.h"
#include "PrototypePlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "HAL/PlatformMisc.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/EngineVersionComparison.h"
#include "Misc/Parse.h"

APrototypeGrabTest::APrototypeGrabTest()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void APrototypeGrabTest::BeginPlay()
{
    Super::BeginPlay();
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestRun="), RunId);
    int32 TestFPS = 60;
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestFPS="), TestFPS);
    FApp::SetUseFixedTimeStep(true);
    FApp::SetFixedDeltaTime(1.0 / FMath::Clamp(TestFPS, 15, 240));
    Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (!Require(Mode && Mode->GetPlayer() && Mode->GetBoss(), TEXT("GameMode spawns grab participants"))) return;
    Mode->GetBoss()->SetActorTickEnabled(false);
    APrototypePlayer* Player = Mode->GetPlayer();
    AIshibashiriBoss* Boss = Mode->GetBoss();
    Player->SetActorLocation(Boss->GetActorLocation() + FVector(-260.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
    AddTickPrerequisiteComponent(Player->GetGrabComponent());
    UE_LOG(LogTemp, Display, TEXT("PROTOTYPE_GRAB_TEST_BEGIN %s"), *RunId);
}

bool APrototypeGrabTest::Require(bool Condition, const TCHAR* Message)
{
    if (!Condition)
    {
        UE_LOG(LogTemp, Error, TEXT("PROTOTYPE_TEST_FAIL %s: %s"), *RunId, Message);
        Finish(false);
    }
    return Condition;
}

void APrototypeGrabTest::SendKey(const FKey& Key, EInputEvent Event)
{
    APlayerController* Controller = Cast<APlayerController>(Mode->GetPlayer()->GetController());
#if UE_VERSION_OLDER_THAN(5, 6, 0)
    Controller->InputKey(FInputKeyParams(Key, Event, Event == IE_Released ? 0.0 : 1.0));
#else
    Controller->InputKey(FInputKeyEventArgs::CreateSimulated(Key, Event, Event == IE_Released ? 0.f : 1.f));
#endif
}

void APrototypeGrabTest::Next(EPhase NewPhase) { Phase = NewPhase; Elapsed = 0.f; }

void APrototypeGrabTest::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Mode || Phase == EPhase::Done) return;
    Elapsed += DeltaSeconds;
    TotalElapsed += DeltaSeconds;
    if (!Require(TotalElapsed < 8.f, TEXT("Grab test completes within eight simulated seconds"))) return;
    APrototypePlayer* Player = Mode->GetPlayer();
    AIshibashiriBoss* Boss = Mode->GetBoss();
    UGrabComponent* Grab = Player->GetGrabComponent();

    switch (Phase)
    {
    case EPhase::Press:
        if (Elapsed <= DeltaSeconds + SMALL_NUMBER)
        {
            SendKey(EKeys::E, IE_Pressed);
            break;
        }
        if (!Require(Grab->IsGrabbing() && Grab->GetGrabTarget() == Boss
            && Grab->GetGrabStartWorldLocation().Equals(Player->GetActorLocation(), 1.f)
            && Player->GetCharacterMovement()->MovementMode == MOVE_None,
            TEXT("Bound E press grabs the nearby boss and disables CharacterMovement"))) return;
        ExpectedRelative = Grab->GetRelativeGrabTransform();
        Boss->SetActorLocation(Boss->GetActorLocation() + FVector(180.f, 110.f, 45.f), false, nullptr, ETeleportType::TeleportPhysics);
        Next(EPhase::FollowTranslation);
        break;
    case EPhase::FollowTranslation:
        if (Elapsed < 0.05f) break;
        if (!Require(Player->GetActorTransform().Equals(ExpectedRelative * Boss->GetActorTransform(), 1.f),
            TEXT("Player preserves boss-relative transform after translation"))) return;
        Boss->SetActorRotation(Boss->GetActorRotation() + FRotator(0.f, 90.f, 0.f), ETeleportType::TeleportPhysics);
        Next(EPhase::FollowRotation);
        break;
    case EPhase::FollowRotation:
        if (Elapsed < 0.05f) break;
        if (!Require(Player->GetActorTransform().Equals(ExpectedRelative * Boss->GetActorTransform(), 1.f),
            TEXT("Player preserves boss-relative transform after rotation"))) return;
        // A normal grab is inside the boss's chase stopping distance. Move the
        // held offset outside it so the real AI Tick, rather than this test,
        // translates the boss and exercises the production tick ordering.
        ExpectedRelative.SetLocation(FVector(-600.f, 0.f, 0.f));
        Grab->SetRelativeGrabTransform(ExpectedRelative);
        BossAIStartLocation = Boss->GetActorLocation();
        Boss->SetActorTickEnabled(true);
        Next(EPhase::FollowBossAI);
        break;
    case EPhase::FollowBossAI:
        if (Elapsed < 0.3f) break;
        if (!Require(Boss->GetState() == EIshibashiriState::Chase
            && FVector::Dist2D(Boss->GetActorLocation(), BossAIStartLocation) > 20.f,
            TEXT("Real boss Chase AI translates the grabbed player target"))) return;
        if (!Require(Grab->IsGrabbing()
            && Player->GetActorTransform().Equals(ExpectedRelative * Boss->GetActorTransform(), 1.f)
            && !Player->GetActorLocation().ContainsNaN(),
            TEXT("Player follows movement from the real boss Tick without transform corruption"))) return;
        SendKey(EKeys::E, IE_Released);
        Next(EPhase::Release);
        break;
    case EPhase::Release:
        if (!Require(!Grab->IsGrabbing() && !Grab->GetGrabTarget()
            && Player->GetCharacterMovement()->MovementMode != MOVE_None,
            TEXT("Bound E release clears target and restores movement or falling"))) return;
        Player->SetActorLocation(Boss->GetActorLocation() + FVector(-260.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
        SendKey(EKeys::E, IE_Pressed);
        Next(EPhase::Regrab);
        break;
    case EPhase::Regrab:
        if (!Require(Grab->IsGrabbing(), TEXT("Player can grab again after release"))) return;
        SendKey(EKeys::R, IE_Pressed);
        SendKey(EKeys::R, IE_Released);
        RetryLocation = FVector(-650.f, 0.f, 92.f);
        Next(EPhase::Retry);
        break;
    case EPhase::Retry:
        if (Elapsed < 0.1f) break;
        if (!Require(!Grab->IsGrabbing() && !Grab->GetGrabTarget()
            && Player->GetCharacterMovement()->MovementMode != MOVE_None
            && FVector::Dist(Player->GetActorLocation(), RetryLocation) < 2.f,
            TEXT("R retry clears grab, restores movement and prevents old-target pulling"))) return;
        Finish(true);
        break;
    default: break;
    }
}

void APrototypeGrabTest::Finish(bool Success)
{
    Phase = EPhase::Done;
    UE_LOG(LogTemp, Success ? Display : Error, TEXT("%s %s"), Success ? TEXT("PROTOTYPE_TEST_PASS") : TEXT("PROTOTYPE_TEST_ABORT"), *RunId);
    FApp::SetUseFixedTimeStep(false);
    FPlatformMisc::RequestExitWithStatus(true, Success ? 0 : 1);
}
