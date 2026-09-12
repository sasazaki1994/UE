#include "PrototypeSmokeTest.h"
#include "PrototypeGameMode.h"
#include "PrototypePlayer.h"
#include "IshibashiriBoss.h"
#include "NushiEncounterManager.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "InputKeyEventArgs.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/App.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/EngineVersionComparison.h"
#include "UnrealClient.h"

namespace
{
    const FKey MovementKeys[] = { EKeys::A, EKeys::W, EKeys::S, EKeys::D };
    const FVector MovementDirections[] = { -FVector::RightVector, FVector::ForwardVector,
        -FVector::ForwardVector, FVector::RightVector };

    int32 CountActors(UWorld* World)
    {
        int32 Count = 0;
        for (TActorIterator<AActor> It(World); It; ++It) ++Count;
        return Count;
    }
}

APrototypeSmokeTest::APrototypeSmokeTest()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void APrototypeSmokeTest::BeginPlay()
{
    Super::BeginPlay();
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestRun="), RunId);
    bCaptureScreenshots = FParse::Param(FCommandLine::Get(), TEXT("PrototypeCapture"));
    int32 TestFPS = 60;
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestFPS="), TestFPS);
    FApp::SetUseFixedTimeStep(true);
    FApp::SetFixedDeltaTime(1.0 / FMath::Clamp(TestFPS, 15, 240));
    Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (!Require(Mode && Mode->GetPlayer() && Mode->GetBoss(), TEXT("GameMode spawns player and boss"))) return;
    APrototypePlayer* Player = Mode->GetPlayer();
    if (!Require(Player->HasImportedVisuals() && Mode->GetBoss()->HasImportedVisuals(), TEXT("Imported warrior/boar meshes and all combat clips are loaded"))) return;
    APlayerController* Controller = Cast<APlayerController>(Player->GetController());
    if (!Require(Controller && Controller->GetPawn() == Player && Controller->GetHUD(), TEXT("Local player possesses character and has a HUD"))) return;
    if (!Require(Player->GetHealth() == 3 && Mode->GetBoss()->GetHealth() == 3, TEXT("Initial HP is 3/3"))) return;
    Mode->GetBoss()->SetActorTickEnabled(false);
    StartPosition = Player->GetActorLocation();
    AddTickPrerequisiteActor(Mode->GetBoss());
    AddTickPrerequisiteComponent(Player->GetCharacterMovement());
    UE_LOG(LogTemp, Display, TEXT("PROTOTYPE_TEST_BEGIN %s"), *RunId);
    if (FParse::Param(FCommandLine::Get(), TEXT("PrototypeCameraTest")))
    {
        // Exercise the current arena/model without the old small-boar combat fixture.
        PlaceAtWall();
        Next(EPhase::CameraWall);
    }
}

bool APrototypeSmokeTest::Require(bool bCondition, const TCHAR* Message)
{
    if (!bCondition)
    {
        UE_LOG(LogTemp, Error, TEXT("PROTOTYPE_TEST_FAIL %s: %s"), *RunId, Message);
        Finish(false);
    }
    return bCondition;
}

void APrototypeSmokeTest::Next(EPhase NewPhase)
{
    Phase = NewPhase;
    Elapsed = 0.f;
    LastBossState = -1;
    bInputSent = false;
}

void APrototypeSmokeTest::SendKey(const FKey& Key, EInputEvent Event, float Amount)
{
    APlayerController* Controller = Cast<APlayerController>(Mode->GetPlayer()->GetController());
#if UE_VERSION_OLDER_THAN(5, 6, 0)
    if (Event == IE_Axis)
        Controller->InputKey(FInputKeyParams(Key, static_cast<double>(Amount), GetWorld()->GetDeltaSeconds(), 1));
    else
        Controller->InputKey(FInputKeyParams(Key, Event, static_cast<double>(Amount)));
#else
    Controller->InputKey(FInputKeyEventArgs::CreateSimulated(Key, Event, Amount));
#endif
}

void APrototypeSmokeTest::TapKey(const FKey& Key)
{
    SendKey(Key, IE_Pressed);
    // Release after a real PlayerController input tick. Releasing Space in the
    // same frame would cancel Jump before CharacterMovement can consume it.
    PendingKeyReleases.AddUnique(Key);
}

void APrototypeSmokeTest::PlaceAtWall()
{
    const float Yaw = WallIndex * 90.f;
    APrototypePlayer* Player = Mode->GetPlayer();
    Player->GetCharacterMovement()->StopMovementImmediately();
    Player->SetActorLocation(-FRotator(0.f, Yaw, 0.f).Vector() * (Mode->ArenaHalfExtent - 150.f) + FVector(0.f, 0.f, 92.f),
        false, nullptr, ETeleportType::TeleportPhysics);
    Player->GetController()->SetControlRotation(FRotator(-18.f, Yaw, 0.f));
}

bool APrototypeSmokeTest::CheckRaisedCamera()
{
    if (!CheckPlayerFraming()) return false;
    APrototypePlayer* Player = Mode->GetPlayer();
    APlayerController* Controller = Cast<APlayerController>(Player->GetController());
    const FVector View = Controller->PlayerCameraManager->GetCameraLocation();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(TestRaisedCamera), false, Player);
    Params.AddIgnoredActor(Mode->GetBoss());
    return Require(Player->IsUsingRaisedCamera() && FVector::Dist(View, Player->GetActorLocation()) > 500.f
        && !Mode->GetBoss()->GetComponentsBoundingBox(true).ExpandBy(12.f).IsInside(View)
        && !GetWorld()->OverlapBlockingTestByChannel(View, FQuat::Identity, ECC_Camera,
            FCollisionShape::MakeSphere(10.f), Params), TEXT("Raised camera clears the player, boss and arena walls"));
}

bool APrototypeSmokeTest::CheckPlayerFraming()
{
    const APrototypePlayer* Player = Mode->GetPlayer();
    const APlayerCameraManager* Camera = Cast<APlayerController>(Player->GetController())->PlayerCameraManager;
    const FMinimalViewInfo& View = Camera->GetCameraCacheView();
    const FVector Local = View.Rotation.UnrotateVector(Player->GetActorLocation() - View.Location);
    const float HalfWidth = Local.X * FMath::Tan(FMath::DegreesToRadians(View.FOV * 0.5f));
    const float HalfHeight = HalfWidth / FMath::Max(1.f, View.AspectRatio);
    return Require(Local.X > 0.f && FMath::Abs(Local.Y) < HalfWidth * 0.85f
        && FMath::Abs(Local.Z) < HalfHeight * 0.85f,
        TEXT("Camera keeps the player inside the view with room around them"));
}

void APrototypeSmokeTest::Capture(const TCHAR* Name)
{
    if (!bCaptureScreenshots) return;
    PendingCapture = Name;
    // Teleports in this test happen after camera update. Let the camera catch up.
    CaptureFramesRemaining = 3;
}

void APrototypeSmokeTest::AimAtBoss()
{
    APrototypePlayer* Player = Mode->GetPlayer();
    AIshibashiriBoss* Boss = Mode->GetBoss();
    // Approach from the arena center, so a boss stopped at a wall is still reachable.
    FVector Offset = (-Boss->GetActorLocation()).GetSafeNormal2D() * 300.f;
    if (Offset.IsNearlyZero()) Offset = FVector(-300.f, 0.f, 0.f);
    const FVector Location(Boss->GetActorLocation().X + Offset.X, Boss->GetActorLocation().Y + Offset.Y, 92.f);
    Player->GetCharacterMovement()->StopMovementImmediately();
    Player->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
    const FRotator Aim = (Boss->GetActorLocation() - Location).GetSafeNormal2D().Rotation();
    Player->SetActorRotation(Aim);
    if (Player->GetController()) Player->GetController()->SetControlRotation(Aim);
}

void APrototypeSmokeTest::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (Phase == EPhase::Done || !Mode) return;
    for (const FKey& Key : PendingKeyReleases) SendKey(Key, IE_Released, 0.f);
    PendingKeyReleases.Reset();
    if (CaptureFramesRemaining > 0 && --CaptureFramesRemaining == 0)
    {
        const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots/Prototype") / RunId);
        IFileManager::Get().MakeDirectory(*Directory, true);
        // Canvas HUD is part of the viewport; Slate/window capture is unnecessary.
        FScreenshotRequest::RequestScreenshot(Directory / (PendingCapture + TEXT(".png")), false, false);
    }
    Elapsed += DeltaSeconds;
    TotalElapsed += DeltaSeconds;
    if (!Require(TotalElapsed < 65.f, TEXT("Test completes within 65 simulated seconds"))) return;
    APrototypePlayer* Player = Mode->GetPlayer();
    AIshibashiriBoss* Boss = Mode->GetBoss();

    switch (Phase)
    {
    case EPhase::InputCamera:
        if (!bInputSent)
        {
            StartViewRotation = Player->GetController()->GetControlRotation();
            SendKey(EKeys::MouseX, IE_Axis, 20.f);
            SendKey(EKeys::MouseY, IE_Axis, 20.f);
            bInputSent = true;
        }
        else if (Elapsed >= 0.1f)
        {
            const FRotator Change = (Player->GetController()->GetControlRotation() - StartViewRotation).GetNormalized();
            UE_LOG(LogTemp, Display, TEXT("PROTOTYPE_MOUSE_DELTA yaw=%.2f pitch=%.2f"), Change.Yaw, Change.Pitch);
            // SceneViewport emits positive MouseY when the mouse moves upward.
            // DefaultInput's -1 mapping combines with the legacy controller's -2.5 pitch scale.
            if (!Require(Change.Yaw > 0.f && Change.Pitch > 0.f, TEXT("Mouse right/up events turn the view right/up"))) return;
            Mode->RetryEncounter();
            StartPosition = Player->GetActorLocation();
            SendKey(MovementKeys[MovementIndex], IE_Pressed);
            Next(EPhase::Movement);
        }
        break;

    case EPhase::Movement:
        if (Elapsed >= 0.45f)
        {
            const FVector Displacement = Player->GetActorLocation() - StartPosition;
            const float Distance = FVector::DotProduct(Displacement, MovementDirections[MovementIndex]);
            if (!Require(Distance > 100.f && Distance < 340.f && Player->GetActorLocation().Z > 85.f
                && (Displacement - MovementDirections[MovementIndex] * Distance).Size2D() < 2.f,
                TEXT("WASD key events move in their configured directions on the floor"))) return;
            SendKey(MovementKeys[MovementIndex], IE_Released, 0.f);
            Player->ConsumeMovementInputVector();
            Player->GetCharacterMovement()->StopMovementImmediately();
            StartPosition = Player->GetActorLocation();
            if (++MovementIndex < UE_ARRAY_COUNT(MovementKeys))
            {
                SendKey(MovementKeys[MovementIndex], IE_Pressed);
                Next(EPhase::Movement);
                break;
            }
            Player->SetActorRotation(FRotator(0.f, 90.f, 0.f));
            TapKey(EKeys::LeftShift);
            Capture(TEXT("01-Dodge"));
            Next(EPhase::Dodge);
        }
        break;

    case EPhase::Dodge:
        if (Player->IsDodging())
        {
            bSawDodgeImmunity = true;
            if (!Require(!Player->ReceiveChargeHit(Boss->GetActorLocation()) && Player->GetHealth() == 3, TEXT("Charge damage is rejected during dodge"))) return;
            Player->Dodge(); // Held/spammed dodge must not restart the timer.
        }
        if (Elapsed > 0.38f)
        {
            if (!Require(bSawDodgeImmunity && !Player->IsDodging(), TEXT("Dodge ends and immunity was exercised"))) return;
            if (!Require(FMath::IsNearlyEqual(FVector::Dist2D(StartPosition, Player->GetActorLocation()), 420.f, 4.f), TEXT("Dodge travels 420 cm across frame rates"))) return;
            Player->Dodge();
            if (!Require(!Player->IsDodging(), TEXT("Dodge cooldown rejects immediate reuse"))) return;
            StartPosition = Player->GetActorLocation();
            TapKey(EKeys::SpaceBar);
            Next(EPhase::InputJump);
        }
        break;

    case EPhase::InputJump:
        if (Elapsed >= 0.15f)
        {
            if (!Require(Player->GetCharacterMovement()->IsFalling() && Player->GetActorLocation().Z > StartPosition.Z + 30.f,
                TEXT("Space key starts a real CharacterMovement jump"))) return;
            Next(EPhase::InputLanding);
        }
        break;

    case EPhase::InputLanding:
        if (!Player->GetCharacterMovement()->IsFalling())
        {
            TapKey(EKeys::RightMouseButton);
            Next(EPhase::InputAltDodge);
        }
        break;

    case EPhase::InputAltDodge:
        if (!bInputSent)
        {
            if (!Require(Player->IsDodging(), TEXT("Right mouse button also starts dodge"))) return;
            bInputSent = true;
        }
        if (Elapsed >= 0.65f)
        {
            Mode->RetryEncounter();
            AimAtBoss();
            TapKey(EKeys::LeftMouseButton);
            Next(EPhase::Guard);
        }
        break;

    case EPhase::Guard:
        if (!bInputSent)
        {
            if (!Require(Player->IsAttacking() && Boss->GetHealth() == 3, TEXT("Left mouse button swings the sword, but normal boss armor rejects damage"))) return;
            bInputSent = true;
        }
        if (Elapsed > 0.6f)
        {
            if (!Require(Boss->GetHealth() == 3, TEXT("Normal armor survives the entire swing"))) return;
            if (!Require(Player->ReceiveChargeHit(Boss->GetActorLocation()), TEXT("A charge can damage a vulnerable player"))) return;
            if (!Require(!Player->ReceiveChargeHit(Boss->GetActorLocation()) && Player->GetHealth() == 2, TEXT("Post-hit immunity rejects repeated contact"))) return;
            Mode->RetryEncounter();
            StartPosition = Player->GetActorLocation();
            Next(EPhase::ImmediateRetry);
        }
        break;

    case EPhase::ImmediateRetry:
        // Let real CharacterMovement ticks run after Retry; a pending LaunchCharacter
        // would move the player despite the reset having zeroed current velocity.
        if (!Require(FVector::Dist2D(StartPosition, Player->GetActorLocation()) < 1.f
            && Player->GetVelocity().SizeSquared2D() < 1.f
            && Player->GetHealth() == 3 && !Player->IsInvulnerable(),
            TEXT("Retry immediately after a hit clears queued knockback and invulnerability"))) return;
        if (Elapsed >= 0.15f)
        {
            Boss->SetActorTickEnabled(true);
            Next(EPhase::Win);
        }
        break;

    case EPhase::Win:
    {
        const EIshibashiriState State = Boss->GetState();
        if (State == EIshibashiriState::Telegraph && LastBossState != static_cast<int32>(State) && Counters == 0)
            Capture(TEXT("02-Telegraph"));
        if (State == EIshibashiriState::Charge)
        {
            if (LastBossState != static_cast<int32>(State))
            {
                LockedDirection = Boss->GetChargeDirection();
                // Move out of the committed path; a homing bug would now rotate the boss.
                Player->SetActorLocation(FVector(0.f, 1000.f, 92.f), false, nullptr, ETeleportType::TeleportPhysics);
            }
            else
            {
                bSawLockedCharge = true;
                if (!Require(Boss->GetChargeDirection().Equals(LockedDirection, 0.001f), TEXT("Charge direction stays locked after target movement"))) return;
            }
        }
        if (State == EIshibashiriState::Recover)
        {
            if (LastBossState != static_cast<int32>(State))
            {
                const int32 Before = Boss->GetHealth();
                AimAtBoss();
                Player->Attack();
                if (!Require(Boss->GetHealth() == Before - 1, TEXT("Real sword sweep damages the recovering boss"))) return;
                ++Counters;
                LastHealth = Boss->GetHealth();
                bTestedExtraCounter = false;
                if (Counters == 1) Capture(TEXT("03-Counter"));
            }
            else if (!bTestedExtraCounter && Boss->GetStateTimeRemaining() < 0.8f)
            {
                // Attack cooldown has elapsed, but this recovery has already been consumed.
                Player->Attack();
                if (!Require(Boss->GetHealth() == LastHealth && !Boss->CanBeCountered(), TEXT("Only one counter per recovery even after attack cooldown"))) return;
                bTestedExtraCounter = true;
            }
        }
        LastBossState = static_cast<int32>(State);
        if (!Mode->IsEncounterActive())
        {
            if (!Require(Mode->GetResult() == EEncounterResult::Victory && Counters == 3 && Boss->GetHealth() == 0 && bSawLockedCharge, TEXT("Three counters reach victory after fixed-direction charges"))) return;
            if (!Require(Boss->GetPurifiedCount() == 0 && Boss->GetNushiState() == ENushiState::Calm
                && Mode->GetEncounterManager()->GetEncounterState() == ENushiEncounterState::Completed,
                TEXT("Counter victory completes shared lifecycle without fabricating purification"))) return;
            Player->Attack();
            Player->Dodge();
            if (!Require(!Player->IsDodging() && Boss->GetHealth() == 0, TEXT("Combat input stops after victory"))) return;
            ActorsBeforeRetry = CountActors(GetWorld());
            if (bCaptureScreenshots)
            {
                Capture(TEXT("04-Victory"));
                Next(EPhase::VictoryCapture);
            }
            else
            {
                TapKey(EKeys::R);
                Next(EPhase::VictoryRetry);
            }
        }
        break;
    }

    case EPhase::VictoryCapture:
        if (Elapsed >= 0.15f)
        {
            TapKey(EKeys::R);
            Next(EPhase::VictoryRetry);
        }
        break;

    case EPhase::VictoryRetry:
        if (!Require(Player->GetHealth() == 3 && Boss->GetHealth() == 3 && Mode->IsEncounterActive() && Player->GetDodgeCooldown() == 0.f, TEXT("R key after victory clears HP and cooldowns"))) return;
        if (!Require(CountActors(GetWorld()) == ActorsBeforeRetry, TEXT("Retry does not accumulate arena actors"))) return;
        LastHealth = 3;
        Next(EPhase::Lose);
        break;

    case EPhase::Lose:
    {
        const EIshibashiriState State = Boss->GetState();
        if (State == EIshibashiriState::Charge)
        {
            // Stay in contact even after knockback. A charge must still take at most one HP.
            Player->GetCharacterMovement()->StopMovementImmediately();
            const FVector Position = Boss->GetActorLocation() + Boss->GetChargeDirection() * 80.f;
            Player->SetActorLocation(FVector(Position.X, Position.Y, 92.f), false, nullptr, ETeleportType::TeleportPhysics);
        }
        if (State == EIshibashiriState::Recover && LastBossState != static_cast<int32>(State))
        {
            if (!Require(Player->GetHealth() == LastHealth - 1, TEXT("One full charge with sustained contact takes exactly one HP"))) return;
            LastHealth = Player->GetHealth();
        }
        LastBossState = static_cast<int32>(State);
        if (!Mode->IsEncounterActive())
        {
            if (!Require(Mode->GetResult() == EEncounterResult::Defeat && Player->GetHealth() == 0 && LastHealth == 1, TEXT("Exactly three charges reach defeat"))) return;
            if (bCaptureScreenshots)
            {
                Capture(TEXT("05-Defeat"));
                Next(EPhase::DefeatCapture);
            }
            else
            {
                TapKey(EKeys::R);
                Next(EPhase::DefeatRetry);
            }
        }
        break;
    }

    case EPhase::DefeatCapture:
        if (Elapsed >= 0.15f)
        {
            TapKey(EKeys::R);
            Next(EPhase::DefeatRetry);
        }
        break;

    case EPhase::DefeatRetry:
        if (!Require(Player->GetHealth() == 3 && Boss->GetHealth() == 3 && Mode->IsEncounterActive()
            && !Player->IsInvulnerable(), TEXT("R key after defeat restores a playable encounter"))) return;
        for (int32 Index = 0; Index < 10; ++Index) Mode->RetryEncounter();
        if (!Require(Player->GetHealth() == 3 && Boss->GetHealth() == 3 && Mode->IsEncounterActive()
            && !Player->IsInvulnerable() && CountActors(GetWorld()) == ActorsBeforeRetry, TEXT("Repeated retry after defeat leaves a clean playable encounter"))) return;
        UE_LOG(LogTemp, Display, TEXT("PROTOTYPE_INPUT_PASS %s: WASD, mouse XY, Shift/RMB, LMB, Space, R"), *RunId);
        Boss->SetActorTickEnabled(false);
        PlaceAtWall();
        Next(EPhase::CameraWall);
        break;

    case EPhase::CameraWall:
        if (Elapsed >= 0.1f && !bInputSent)
        {
            if (!CheckRaisedCamera()) return;
            if (WallIndex == 0) Capture(TEXT("06-WallCamera"));
            bInputSent = true;
        }
        if (Elapsed >= 0.3f)
        {
            if (++WallIndex < 4)
            {
                PlaceAtWall();
                Next(EPhase::CameraWall);
            }
            else
            {
                Player->SetActorLocation(FVector(0.f, 0.f, 92.f), false, nullptr, ETeleportType::TeleportPhysics);
                Player->GetController()->SetControlRotation(FRotator(-5.f, 0.f, 0.f));
                // Put the boss midway along the regular camera sightline. Its
                // bounds do not contain the camera, so this catches regressions
                // that only test the final camera position for obstruction.
                Boss->SetActorLocation(FVector(-500.f, 35.f, 352.f), false, nullptr, ETeleportType::TeleportPhysics);
                Next(EPhase::CameraBoss);
            }
        }
        break;

    case EPhase::CameraBoss:
        if (Elapsed >= 0.1f && !bInputSent)
        {
            if (!CheckRaisedCamera()) return;
            Capture(TEXT("07-BossCamera"));
            bInputSent = true;
        }
        if (Elapsed >= 0.3f)
        {
            LockedDirection = Player->GetAttackIndicatorDirection();
            if (!Require(LockedDirection.Equals(FVector::ForwardVector, 0.001f),
                TEXT("Raised camera framing does not redirect the player's aim toward the boss behind them"))) return;
            TapKey(EKeys::LeftMouseButton);
            Next(EPhase::CameraAim);
        }
        break;

    case EPhase::CameraAim:
        if (!bInputSent)
        {
            if (!Require(Player->IsAttacking() && Player->GetActorForwardVector().Equals(LockedDirection, 0.001f)
                && Boss->GetHealth() == 3, TEXT("Raised-view LMB swings along the displayed direction"))) return;
            SendKey(EKeys::MouseX, IE_Axis, 20.f);
            bInputSent = true;
        }
        if (Elapsed >= 0.1f && Elapsed <= 0.2f)
        {
            const FVector Aim = FRotator(0.f, Player->GetController()->GetControlRotation().Yaw, 0.f).Vector();
            if (!Require(Player->IsAttacking() && Player->GetAttackIndicatorDirection().Equals(LockedDirection, 0.001f)
                && !Aim.Equals(LockedDirection, 0.01f), TEXT("Mouse turn during a swing leaves its indicator and attack direction locked"))) return;
        }
        if (Elapsed >= 0.6f)
        {
            const FVector Aim = FRotator(0.f, Player->GetController()->GetControlRotation().Yaw, 0.f).Vector();
            if (!Require(!Player->IsAttacking() && Player->GetAttackIndicatorDirection().Equals(Aim, 0.001f),
                TEXT("After the swing the indicator follows the new mouse aim"))) return;
            OffsetBeforeCameraReturn = Cast<APlayerController>(Player->GetController())->PlayerCameraManager->GetCameraLocation()
                - Player->GetActorLocation();
            // Move clear without Retry: resetting the camera flag here would
            // conceal a bug that never exits the raised view during gameplay.
            Player->SetActorLocation(FVector(-1150.f, 0.f, 92.f), false, nullptr, ETeleportType::TeleportPhysics);
            Player->GetController()->SetControlRotation(FRotator(-18.f, 0.f, 0.f));
            Boss->SetActorLocation(FVector(2000.f, 0.f, 352.f), false, nullptr, ETeleportType::TeleportPhysics);
            Next(EPhase::CameraRestore);
        }
        break;

    case EPhase::CameraRestore:
    {
        if (!CheckPlayerFraming()) return;
        APlayerController* Controller = Cast<APlayerController>(Player->GetController());
        const FVector View = Controller->PlayerCameraManager->GetCameraLocation();
        const UCameraComponent* NormalCamera = Player->FindComponentByClass<UCameraComponent>();
        FCollisionQueryParams Params(SCENE_QUERY_STAT(TestCameraReturn), false, Player);
        Params.AddIgnoredActor(Boss);
        if (!Require(!GetWorld()->OverlapBlockingTestByChannel(View, FQuat::Identity, ECC_Camera,
            FCollisionShape::MakeSphere(10.f), Params)
            && !Boss->GetComponentsBoundingBox(true).ExpandBy(12.f).IsInside(View),
            TEXT("Every return frame remains outside walls and boss geometry"))) return;
        if (Elapsed >= 0.1f && Elapsed < Player->CameraClearDelay)
        {
            if (!Require(Player->IsUsingRaisedCamera() && (View - Player->GetActorLocation()).Equals(OffsetBeforeCameraReturn, 1.f),
                TEXT("Briefly clear sightlines do not immediately drop the raised camera"))) return;
        }
        if (Elapsed >= 0.28f && !bInputSent)
        {
            if (!Require(Player->IsUsingRaisedCamera() && NormalCamera
                && FVector::Dist(View, NormalCamera->GetComponentLocation()) > 20.f
                && FVector::Dist(View, Player->GetActorLocation() + OffsetBeforeCameraReturn) > 20.f,
                TEXT("Camera passes through an intermediate view instead of snapping back"))) return;
            bInputSent = true;
            if (!bTestedCameraReobstruction)
            {
                Boss->SetActorLocation(Player->GetActorLocation() + FVector(-500.f, 35.f, 260.f),
                    false, nullptr, ETeleportType::TeleportPhysics);
                Next(EPhase::CameraReobstruct);
                break;
            }
            Capture(TEXT("09-CameraReturn"));
        }
        if (Elapsed >= 0.75f)
        {
            if (!Require(!Player->IsUsingRaisedCamera() && NormalCamera
                && View.Equals(NormalCamera->GetComponentLocation(), 1.f),
                TEXT("Normal third-person view is fully restored after the blend"))) return;
            // Retry must discard both the blend and the previous mouse aim.
            PlaceAtWall();
            Next(EPhase::CameraReset);
        }
        break;
    }

    case EPhase::CameraReobstruct:
        if (Elapsed >= 0.1f)
        {
            if (!CheckRaisedCamera()) return;
            bTestedCameraReobstruction = true;
            OffsetBeforeCameraReturn = Cast<APlayerController>(Player->GetController())->PlayerCameraManager->GetCameraLocation()
                - Player->GetActorLocation();
            Boss->SetActorLocation(FVector(2000.f, 0.f, 352.f), false, nullptr, ETeleportType::TeleportPhysics);
            Next(EPhase::CameraRestore);
        }
        break;

    case EPhase::CameraReset:
        if (Elapsed >= 0.1f && !bInputSent)
        {
            if (!CheckRaisedCamera()) return;
            Mode->RetryEncounter();
            bInputSent = true;
        }
        else if (Elapsed >= 0.25f)
        {
            if (!Require(!Player->IsUsingRaisedCamera()
                && Player->GetAttackIndicatorDirection().Equals(FVector::ForwardVector, 0.001f),
                TEXT("Retry clears camera transition and resets the attack indicator"))) return;
            UE_LOG(LogTemp, Display, TEXT("PROTOTYPE_CAMERA_PASS %s: four walls, boar sightline, aim lock, smooth return, re-obstruction, retry"), *RunId);
            Finish(true);
        }
        break;
    default: break;
    }
}

void APrototypeSmokeTest::Finish(bool bSuccess)
{
    Phase = EPhase::Done;
    if (bSuccess) UE_LOG(LogTemp, Display, TEXT("PROTOTYPE_TEST_PASS %s"), *RunId);
    FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1);
}
