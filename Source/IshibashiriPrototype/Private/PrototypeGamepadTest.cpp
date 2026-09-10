#include "PrototypeGamepadTest.h"
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

APrototypeGamepadTest::APrototypeGamepadTest()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void APrototypeGamepadTest::BeginPlay()
{
    Super::BeginPlay();
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestRun="), RunId);
    int32 TestFPS = 60;
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestFPS="), TestFPS);
    FApp::SetUseFixedTimeStep(true);
    FApp::SetFixedDeltaTime(1.0 / FMath::Clamp(TestFPS, 15, 240));
    Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (!Require(Mode && Mode->GetPlayer() && Mode->GetBoss(), TEXT("GameMode spawns gamepad test participants"))) return;
    Mode->GetPlayer()->bUseRouteClimbing = false;
    Mode->GetBoss()->SetActorTickEnabled(false);
    BaselineLocation = Mode->GetPlayer()->GetActorLocation();
    AddTickPrerequisiteComponent(Mode->GetPlayer()->GetGrabComponent());
    UE_LOG(LogTemp, Display, TEXT("PROTOTYPE_GAMEPAD_TEST_BEGIN %s"), *RunId);
}

bool APrototypeGamepadTest::Require(bool Condition, const TCHAR* Message)
{
    if (!Condition)
    {
        UE_LOG(LogTemp, Error, TEXT("PROTOTYPE_TEST_FAIL %s: %s"), *RunId, Message);
        Finish(false);
    }
    return Condition;
}

void APrototypeGamepadTest::SendKey(const FKey& Key, EInputEvent Event, float Value)
{
    APlayerController* Controller = Cast<APlayerController>(Mode->GetPlayer()->GetController());
#if UE_VERSION_OLDER_THAN(5, 6, 0)
    Controller->InputKey(FInputKeyParams(Key, Event, Value));
#else
    Controller->InputKey(FInputKeyEventArgs::CreateSimulated(Key, Event, Value));
#endif
}

void APrototypeGamepadTest::SendAxis(const FKey& Key, float Value) { AxisValues.Add(Key, Value); }
void APrototypeGamepadTest::Next(EPhase NewPhase) { Phase = NewPhase; Elapsed = 0.f; }

void APrototypeGamepadTest::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Mode || Phase == EPhase::Done) return;
    Elapsed += DeltaSeconds;
    TotalElapsed += DeltaSeconds;
    if (!Require(TotalElapsed < 15.f, TEXT("Gamepad test completes within fifteen simulated seconds"))) return;
    APrototypePlayer* Player = Mode->GetPlayer();
    AIshibashiriBoss* Boss = Mode->GetBoss();
    UGrabComponent* Grab = Player->GetGrabComponent();
    APlayerController* Controller = Cast<APlayerController>(Player->GetController());

    switch (Phase)
    {
    case EPhase::DeadZone:
        SendAxis(EKeys::Gamepad_LeftY, 0.15f);
        if (Elapsed < 0.3f) break;
        SendAxis(EKeys::Gamepad_LeftY, 0.f);
        if (!Require(FVector::Dist2D(Player->GetActorLocation(), BaselineLocation) < 1.f,
            TEXT("Left stick values below the configured dead zone do not move the player"))) return;
        BaselineLocation = Player->GetActorLocation();
        Next(EPhase::MoveForward);
        break;
    case EPhase::MoveForward:
        SendAxis(EKeys::Gamepad_LeftY, 1.f);
        if (Elapsed < 0.35f) break;
        SendAxis(EKeys::Gamepad_LeftY, 0.f);
        if (!Require(Player->GetActorLocation().X > BaselineLocation.X + 20.f,
            TEXT("Left stick up moves forward and returning the axis to zero stops input"))) return;
        BaselineLocation = Player->GetActorLocation();
        Next(EPhase::MoveRight);
        break;
    case EPhase::MoveRight:
        SendAxis(EKeys::Gamepad_LeftX, 1.f);
        if (Elapsed < 0.35f) break;
        SendAxis(EKeys::Gamepad_LeftX, 0.f);
        if (!Require(Player->GetActorLocation().Y > BaselineLocation.Y + 20.f, TEXT("Left stick right moves right"))) return;
        BaselineRotation = Controller->GetControlRotation();
        Next(EPhase::CameraYaw);
        break;
    case EPhase::CameraYaw:
        SendAxis(EKeys::Gamepad_RightX, 1.f);
        if (Elapsed < 0.25f) break;
        SendAxis(EKeys::Gamepad_RightX, 0.f);
        if (!Require(Controller->GetControlRotation().Yaw > BaselineRotation.Yaw + 10.f,
            TEXT("Right stick right increases camera yaw with frame-rate-independent rate input"))) return;
        BaselineRotation = Controller->GetControlRotation();
        Next(EPhase::CameraPitchUp);
        break;
    case EPhase::CameraPitchUp:
        SendAxis(EKeys::Gamepad_RightY, 1.f);
        if (Elapsed < 0.15f) break;
        SendAxis(EKeys::Gamepad_RightY, 0.f);
        if (!Require(Controller->GetControlRotation().Pitch < BaselineRotation.Pitch - 5.f,
            TEXT("Right stick up decreases control pitch and looks upward"))) return;
        BaselineRotation = Controller->GetControlRotation();
        Next(EPhase::CameraPitchDown);
        break;
    case EPhase::CameraPitchDown:
        SendAxis(EKeys::Gamepad_RightY, -1.f);
        if (Elapsed < 0.15f) break;
        SendAxis(EKeys::Gamepad_RightY, 0.f);
        if (!Require(Controller->GetControlRotation().Pitch > BaselineRotation.Pitch + 5.f,
            TEXT("Right stick down increases control pitch and looks downward"))) return;
        SendKey(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed);
        Next(EPhase::Jump);
        break;
    case EPhase::Jump:
        if (Elapsed < 0.1f) break;
        if (!Require(Player->GetCharacterMovement()->IsFalling() && Player->GetVelocity().Z > 0.f, TEXT("A starts jump"))) return;
        SendKey(EKeys::Gamepad_FaceButton_Bottom, IE_Released, 0.f);
        Next(EPhase::WaitForLanding);
        break;
    case EPhase::WaitForLanding:
        if (Player->GetCharacterMovement()->IsFalling()) break;
        SendKey(EKeys::Gamepad_FaceButton_Right, IE_Pressed);
        Next(EPhase::Dodge);
        break;
    case EPhase::Dodge:
        if (Elapsed < 0.1f && !Require(Player->IsDodging(), TEXT("B starts the existing dodge"))) return;
        if (Elapsed < 0.35f) break;
        SendKey(EKeys::Gamepad_FaceButton_Left, IE_Pressed);
        Next(EPhase::Attack);
        break;
    case EPhase::Attack:
        if (!Require(Player->IsAttacking(), TEXT("X starts the existing attack"))) return;
        Player->SetActorLocation(Boss->GetActorLocation() + FVector(-260.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
        SendKey(EKeys::Gamepad_RightShoulder, IE_Pressed);
        Next(EPhase::Grab);
        break;
    case EPhase::Grab:
        if (!Require(Grab->IsGrabbing() && Player->GetCharacterMovement()->MovementMode == MOVE_None,
            TEXT("RB press starts the existing hold-to-grab path"))) return;
        BaselineGrab = Grab->GetRelativeGrabTransform();
        Next(EPhase::ClimbQuarter);
        break;
    case EPhase::ClimbQuarter:
        SendAxis(EKeys::Gamepad_LeftY, 0.25f);
        if (Elapsed < 0.4f) break;
        SendAxis(EKeys::Gamepad_LeftY, 0.f);
        QuarterDistance = Grab->GetRelativeGrabTransform().GetLocation().Z - BaselineGrab.GetLocation().Z;
        if (!Require(QuarterDistance > 0.f, TEXT("Quarter left-stick input climbs slowly above the dead zone"))) return;
        BaselineGrab = Grab->GetRelativeGrabTransform();
        Next(EPhase::ClimbHalf);
        break;
    case EPhase::ClimbHalf:
        SendAxis(EKeys::Gamepad_LeftY, 0.5f);
        if (Elapsed < 0.4f) break;
        SendAxis(EKeys::Gamepad_LeftY, 0.f);
        HalfDistance = Grab->GetRelativeGrabTransform().GetLocation().Z - BaselineGrab.GetLocation().Z;
        if (!Require(HalfDistance > QuarterDistance * 1.5f, TEXT("Half stick climbs faster than quarter stick"))) return;
        BaselineGrab = Grab->GetRelativeGrabTransform();
        Next(EPhase::ClimbFull);
        break;
    case EPhase::ClimbFull:
        SendAxis(EKeys::Gamepad_LeftX, 1.f);
        if (Elapsed < 0.4f) break;
        SendAxis(EKeys::Gamepad_LeftX, 0.f);
        if (!Require(Grab->GetRelativeGrabTransform().GetLocation().Y > BaselineGrab.GetLocation().Y + HalfDistance
            && Grab->IsGrabbing(), TEXT("Full left-stick right climbs right faster and preserves grab"))) return;
        BaselineGrab = Grab->GetRelativeGrabTransform();
        BaselineGrab.SetLocation(FVector(-600.f, BaselineGrab.GetLocation().Y, BaselineGrab.GetLocation().Z));
        Grab->SetRelativeGrabTransform(BaselineGrab);
        BossStart = Boss->GetActorLocation();
        Boss->SetActorTickEnabled(true);
        Next(EPhase::BossFollow);
        break;
    case EPhase::BossFollow:
        SendAxis(EKeys::Gamepad_LeftY, 0.5f);
        if (Elapsed < 0.4f) break;
        SendAxis(EKeys::Gamepad_LeftY, 0.f);
        if (!Require(FVector::Dist(Boss->GetActorLocation(), BossStart) > 10.f && Grab->IsGrabbing()
            && Player->GetActorTransform().Equals(Grab->GetRelativeGrabTransform() * Boss->GetActorTransform(), 2.f),
            TEXT("Gamepad climbing preserves follow while the real boss tick moves"))) return;
        SendKey(EKeys::Gamepad_RightShoulder, IE_Released, 0.f);
        Next(EPhase::Release);
        break;
    case EPhase::Release:
        if (!Require(!Grab->IsGrabbing() && Player->GetCharacterMovement()->MovementMode != MOVE_None,
            TEXT("RB release immediately releases and restores movement"))) return;
        Boss->SetActorTickEnabled(false);
        Player->SetActorLocation(Boss->GetActorLocation() + FVector(-260.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
        SendKey(EKeys::Gamepad_RightShoulder, IE_Pressed);
        Next(EPhase::RetryGrab);
        break;
    case EPhase::RetryGrab:
        if (!Require(Grab->IsGrabbing(), TEXT("RB can grab again before retry"))) return;
        SendAxis(EKeys::Gamepad_LeftY, 1.f);
        Next(EPhase::RetryInput);
        break;
    case EPhase::RetryInput:
        // IE_Axis values accumulate within a frame: 1 followed by 0 is still 1.
        // Release on the following frame before retry so this models a centered stick.
        SendAxis(EKeys::Gamepad_LeftY, 0.f);
        SendKey(EKeys::Gamepad_FaceButton_Top, IE_Pressed);
        SendKey(EKeys::Gamepad_FaceButton_Top, IE_Released, 0.f);
        Next(EPhase::Retry);
        break;
    case EPhase::Retry:
        if (Elapsed < 0.1f) break;
        UE_LOG(LogTemp, Display, TEXT("GAMEPAD_RETRY player=%s boss=%s grab=%d movement=%d velocity=%s"),
            *Player->GetActorLocation().ToString(), *Boss->GetActorLocation().ToString(),
            Grab->IsGrabbing(), int32(Player->GetCharacterMovement()->MovementMode), *Player->GetVelocity().ToString());
        if (!Require(!Grab->IsGrabbing() && Player->GetCharacterMovement()->MovementMode != MOVE_None
            && FVector::Dist(Player->GetActorLocation(), FVector(-1150.f, -750.f, 92.f)) < 2.f
            && FVector::Dist(Boss->GetActorLocation(), FVector(650.f, 0.f, 352.f)) < 2.f,
            TEXT("Y retry clears climbing and resets player, boss, and movement"))) return;
        Finish(true);
        break;
    default: break;
    }
    // Emit one current sample per axis per frame. Sending 1 then 0 in one frame
    // adds the samples in UE, and failing to send the centered axes leaves stale input.
    for (const auto& Axis : AxisValues) SendKey(Axis.Key, IE_Axis, Axis.Value);
}

void APrototypeGamepadTest::Finish(bool Success)
{
    Phase = EPhase::Done;
    if (Success) { UE_LOG(LogTemp, Display, TEXT("PROTOTYPE_TEST_PASS %s"), *RunId); }
    else { UE_LOG(LogTemp, Error, TEXT("PROTOTYPE_TEST_ABORT %s"), *RunId); }
    FApp::SetUseFixedTimeStep(false);
    FPlatformMisc::RequestExitWithStatus(true, Success ? 0 : 1);
}
