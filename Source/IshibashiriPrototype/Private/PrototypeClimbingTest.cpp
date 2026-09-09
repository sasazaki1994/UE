#include "PrototypeClimbingTest.h"
#include "GrabComponent.h"
#include "IshibashiriBoss.h"
#include "PrototypeGameMode.h"
#include "PrototypePlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "InputKeyEventArgs.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/EngineVersionComparison.h"
#include "Misc/Parse.h"

APrototypeClimbingTest::APrototypeClimbingTest()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void APrototypeClimbingTest::BeginPlay()
{
    Super::BeginPlay();
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestRun="), RunId);
    int32 TestFPS = 60;
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestFPS="), TestFPS);
    FApp::SetUseFixedTimeStep(true);
    FApp::SetFixedDeltaTime(1.0 / FMath::Clamp(TestFPS, 15, 240));
    Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (!Require(Mode && Mode->GetPlayer() && Mode->GetBoss(), TEXT("GameMode spawns climbing participants"))) return;
    Mode->GetPlayer()->bUseRouteClimbing = false;
    Mode->GetBoss()->SetActorTickEnabled(false);
    Mode->GetPlayer()->SetActorLocation(Mode->GetBoss()->GetActorLocation() + FVector(-260.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
    AddTickPrerequisiteComponent(Mode->GetPlayer()->GetGrabComponent());
    UE_LOG(LogTemp, Display, TEXT("PROTOTYPE_CLIMBING_TEST_BEGIN %s"), *RunId);
}

bool APrototypeClimbingTest::Require(bool Condition, const TCHAR* Message)
{
    if (!Condition)
    {
        UE_LOG(LogTemp, Error, TEXT("PROTOTYPE_TEST_FAIL %s: %s"), *RunId, Message);
        Finish(false);
    }
    return Condition;
}

void APrototypeClimbingTest::SendKey(const FKey& Key, EInputEvent Event)
{
    APlayerController* Controller = Cast<APlayerController>(Mode->GetPlayer()->GetController());
#if UE_VERSION_OLDER_THAN(5, 6, 0)
    Controller->InputKey(FInputKeyParams(Key, Event, Event == IE_Released ? 0.0 : 1.0));
#else
    Controller->InputKey(FInputKeyEventArgs::CreateSimulated(Key, Event, Event == IE_Released ? 0.f : 1.f));
#endif
}

void APrototypeClimbingTest::Next(EPhase NewPhase) { Phase = NewPhase; Elapsed = 0.f; }

void APrototypeClimbingTest::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Mode || Phase == EPhase::Done) return;
    Elapsed += DeltaSeconds;
    TotalElapsed += DeltaSeconds;
    if (!Require(TotalElapsed < 12.f, TEXT("Climbing test completes within twelve simulated seconds"))) return;
    APrototypePlayer* Player = Mode->GetPlayer();
    AIshibashiriBoss* Boss = Mode->GetBoss();
    UGrabComponent* Grab = Player->GetGrabComponent();

    switch (Phase)
    {
    case EPhase::Grab:
        if (Elapsed <= DeltaSeconds + SMALL_NUMBER) { SendKey(EKeys::E, IE_Pressed); break; }
        if (!Require(Grab->IsGrabbing() && Player->GetCharacterMovement()->MovementMode == MOVE_None,
            TEXT("E press starts grab and disables CharacterMovement"))) return;
        Baseline = Grab->GetRelativeGrabTransform();
        SendKey(EKeys::W, IE_Pressed);
        Next(EPhase::Up);
        break;
    case EPhase::Up:
        if (Elapsed < 0.25f) break;
        SendKey(EKeys::W, IE_Released);
        if (!Require(Grab->GetRelativeGrabTransform().GetLocation().Z > Baseline.GetLocation().Z + 20.f
            && Grab->IsGrabbing() && Player->GetActorTransform().Equals(Grab->GetRelativeGrabTransform() * Boss->GetActorTransform(), 2.f),
            TEXT("W changes local Z upward, moves the player, and preserves grab"))) return;
        Baseline = Grab->GetRelativeGrabTransform();
        SendKey(EKeys::S, IE_Pressed);
        Next(EPhase::Down);
        break;
    case EPhase::Down:
        if (Elapsed < 0.25f) break;
        SendKey(EKeys::S, IE_Released);
        if (!Require(Grab->GetRelativeGrabTransform().GetLocation().Z < Baseline.GetLocation().Z - 20.f,
            TEXT("S changes local Z in the opposite direction"))) return;
        Baseline = Grab->GetRelativeGrabTransform();
        SendKey(EKeys::A, IE_Pressed);
        Next(EPhase::Left);
        break;
    case EPhase::Left:
        if (Elapsed < 0.25f) break;
        SendKey(EKeys::A, IE_Released);
        if (!Require(Grab->GetRelativeGrabTransform().GetLocation().Y < Baseline.GetLocation().Y - 20.f,
            TEXT("A changes target-local Y toward the left"))) return;
        Baseline = Grab->GetRelativeGrabTransform();
        SendKey(EKeys::D, IE_Pressed);
        Next(EPhase::Right);
        break;
    case EPhase::Right:
        if (Elapsed < 0.25f) break;
        SendKey(EKeys::D, IE_Released);
        if (!Require(Grab->GetRelativeGrabTransform().GetLocation().Y > Baseline.GetLocation().Y + 20.f,
            TEXT("D changes target-local Y opposite to A"))) return;
        Baseline = Grab->GetRelativeGrabTransform();
        {
            FTransform OutsideBounds = Baseline;
            OutsideBounds.SetLocation(FVector(5000.f, -5000.f, 5000.f));
            Grab->SetRelativeGrabTransform(OutsideBounds);
            Grab->Climb(1.f, -1.f, 1.f);
        }
        Next(EPhase::Clamp);
        break;
    case EPhase::Clamp:
        if (!Require(Grab->GetRelativeGrabTransform().GetLocation().Equals(FVector(650.f, -350.f, 650.f), 0.1f),
            TEXT("Climbing clamps all target-local axes to configured bounds"))) return;
        Grab->SetRelativeGrabTransform(Baseline);
        Boss->SetActorRotation(Boss->GetActorRotation() + FRotator(20.f, 90.f, 15.f), ETeleportType::TeleportPhysics);
        Baseline = Grab->GetRelativeGrabTransform();
        SendKey(EKeys::W, IE_Pressed);
        Next(EPhase::RotatedUp);
        break;
    case EPhase::RotatedUp:
        if (Elapsed < 0.25f) break;
        SendKey(EKeys::W, IE_Released);
        if (!Require(Grab->GetRelativeGrabTransform().GetLocation().Z > Baseline.GetLocation().Z + 20.f
            && Player->GetActorTransform().Equals(Grab->GetRelativeGrabTransform() * Boss->GetActorTransform(), 2.f)
            && !Player->GetActorLocation().ContainsNaN(),
            TEXT("Rotated boss climbing remains local-space and produces a finite world transform"))) return;
        Baseline = Grab->GetRelativeGrabTransform();
        Baseline.SetLocation(FVector(-600.f, Baseline.GetLocation().Y, Baseline.GetLocation().Z));
        Grab->SetRelativeGrabTransform(Baseline);
        BossAIStart = Boss->GetActorLocation();
        Boss->SetActorTickEnabled(true);
        SendKey(EKeys::D, IE_Pressed);
        Next(EPhase::BossAI);
        break;
    case EPhase::BossAI:
        if (Elapsed < 0.4f) break;
        SendKey(EKeys::D, IE_Released);
        if (!Require(FVector::Dist(Boss->GetActorLocation(), BossAIStart) > 20.f
            && Grab->GetRelativeGrabTransform().GetLocation().Y > Baseline.GetLocation().Y + 20.f
            && Grab->IsGrabbing() && !Player->GetActorLocation().ContainsNaN()
            && Player->GetActorTransform().Equals(Grab->GetRelativeGrabTransform() * Boss->GetActorTransform(), 2.f),
            TEXT("Real Chase Tick and local climbing run together without losing follow"))) return;
        SendKey(EKeys::E, IE_Released);
        ReleasedRelativeLocation = Grab->GetRelativeGrabTransform().GetLocation();
        Next(EPhase::Release);
        break;
    case EPhase::Release:
        if (Elapsed < 0.1f) break;
        if (!Require(!Grab->IsGrabbing() && !Grab->GetGrabTarget()
            && Player->GetCharacterMovement()->MovementMode != MOVE_None
            && Grab->GetRelativeGrabTransform().GetLocation().Equals(ReleasedRelativeLocation, 0.1f),
            TEXT("E release ends climbing, restores movement, and stops relative updates"))) return;
        Boss->SetActorTickEnabled(false);
        Player->SetActorLocation(Boss->GetActorLocation() + FVector(-260.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
        SendKey(EKeys::E, IE_Pressed);
        SendKey(EKeys::W, IE_Pressed);
        Next(EPhase::Regrab);
        break;
    case EPhase::Regrab:
        if (Elapsed < 0.15f) break;
        if (!Require(Grab->IsGrabbing(), TEXT("Player can climb after re-grabbing"))) return;
        SendKey(EKeys::W, IE_Released);
        SendKey(EKeys::R, IE_Pressed);
        SendKey(EKeys::R, IE_Released);
        Next(EPhase::Retry);
        break;
    case EPhase::Retry:
        if (Elapsed < 0.15f) break;
        if (!Require(!Grab->IsGrabbing() && !Grab->GetGrabTarget()
            && Player->GetCharacterMovement()->MovementMode != MOVE_None
            && FVector::Dist(Player->GetActorLocation(), FVector(-1150.f, -750.f, 92.f)) < 2.f
            && FVector::Dist(Boss->GetActorLocation(), FVector(650.f, 0.f, 352.f)) < 2.f,
            TEXT("R clears climbing and restores both encounter transforms and movement"))) return;
        Finish(true);
        break;
    default: break;
    }
}

void APrototypeClimbingTest::Finish(bool Success)
{
    Phase = EPhase::Done;
    if (Success) { UE_LOG(LogTemp, Display, TEXT("PROTOTYPE_TEST_PASS %s"), *RunId); }
    else { UE_LOG(LogTemp, Error, TEXT("PROTOTYPE_TEST_ABORT %s"), *RunId); }
    FApp::SetUseFixedTimeStep(false);
    FPlatformMisc::RequestExitWithStatus(true, Success ? 0 : 1);
}
