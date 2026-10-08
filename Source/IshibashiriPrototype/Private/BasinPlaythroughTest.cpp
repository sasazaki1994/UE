#include "BasinPlaythroughTest.h"
#include "BasinPrototypeArena.h"
#include "ColossusClimbingComponent.h"
#include "IshibashiriBoss.h"
#include "PrototypeGameMode.h"
#include "PrototypePlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/LightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "InputKeyEventArgs.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

ABasinPlaythroughTest::ABasinPlaythroughTest()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void ABasinPlaythroughTest::BeginPlay()
{
    Super::BeginPlay();
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestRun="), RunId);
    int32 FPS = 60;
    FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestFPS="), FPS);
    bPreviousFixedTimeStep = FApp::UseFixedTimeStep();
    PreviousFixedDeltaTime = FApp::GetFixedDeltaTime();
    FApp::SetUseFixedTimeStep(true);
    FApp::SetFixedDeltaTime(1.0 / FMath::Clamp(FPS, 15, 240));
    bCapture = FParse::Param(FCommandLine::Get(), TEXT("PrototypeCapture"));
    Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (!Mode || !Mode->GetBasinArena() || !Mode->GetPlayer() || !Mode->GetBoss())
    {
        Fail(TEXT("basin encounter did not spawn"));
        return;
    }
    DodgeSide = EKeys::A;
    UInputComponent* Input = Mode->GetPlayer()->InputComponent;
    if (Input)
    {
        for (int32 Index = 0; Index < Input->GetNumActionBindings(); ++Index)
        {
            FInputActionBinding& Binding = Input->GetActionBinding(Index);
            if (Binding.GetActionName() != TEXT("Retry") || Binding.KeyEvent != IE_Pressed) continue;
            if (!Binding.ActionDelegate.IsBoundToObject(Mode->GetPlayer())) break;
            OriginalRetryDelegate = Binding.ActionDelegate;
            RetryBindingHandle = Binding.GetHandle();
            // Unified delegates share their underlying delegate. Replace the
            // wrapper before binding so the saved player handler stays bound.
            Binding.ActionDelegate = FInputActionUnifiedDelegate();
            Binding.ActionDelegate.BindDelegate(this, &ABasinPlaythroughTest::HandleRetryInput);
            break;
        }
    }
    if (RetryBindingHandle == INDEX_NONE)
    {
        Fail(TEXT("normal player Retry input binding did not initialize"));
        return;
    }
    SpawnLocation = Mode->GetBasinArena()->PlayerStart;
    ABasinPrototypeArena* Arena = Mode->GetBasinArena();
    PlayerStartTransform = FTransform(FRotator::ZeroRotator, Arena->PlayerStart);
    BossStartTransform = FTransform(FRotator(0.f, 180.f, 0.f), Arena->BossStart);
    TArray<UStaticMeshComponent*> Meshes; Arena->GetComponents(Meshes); InitialStaticMeshes = Meshes.Num();
    TArray<ULightComponent*> Lights; Arena->GetComponents(Lights); InitialLights = Lights.Num();
    TArray<UExponentialHeightFogComponent*> Fogs; Arena->GetComponents(Fogs); InitialFogs = Fogs.Num();
    InitialRocks = Arena->GetVisualRockCount(); InitialBoundaries = Arena->GetBoundaryCount(); InitialAccents = Arena->GetAccentCount();
    UE_LOG(LogTemp, Display, TEXT("BASIN_SCENARIO_BEGIN %s %s"), *RunId, *Diagnostic());
}

void ABasinPlaythroughTest::EndPlay(const EEndPlayReason::Type Reason)
{
    ReleaseAll();
    RestoreRetryInput();
    RestoreExecutionSettings();
    Super::EndPlay(Reason);
}

void ABasinPlaythroughTest::RestoreExecutionSettings()
{
    FApp::SetFixedDeltaTime(PreviousFixedDeltaTime);
    FApp::SetUseFixedTimeStep(bPreviousFixedTimeStep);
}

void ABasinPlaythroughTest::Finish(int32 ExitCode)
{
    ReleaseAll();
    RestoreRetryInput();
    RestoreExecutionSettings();
    bFinished = true;
    FPlatformMisc::RequestExitWithStatus(false, ExitCode);
}

void ABasinPlaythroughTest::Hold(const FKey& Key, bool bDown)
{
    if (!Mode || !Mode->GetPlayer() || Held.Contains(Key) == bDown) return;
    if (APlayerController* PC = Cast<APlayerController>(Mode->GetPlayer()->GetController()))
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, bDown ? IE_Pressed : IE_Released, bDown ? 1.f : 0.f));
    if (bDown) Held.Add(Key); else Held.Remove(Key);
}

void ABasinPlaythroughTest::Tap(const FKey& Key)
{
    Hold(Key, true);
    PendingRelease.AddUnique(Key);
}

float ABasinPlaythroughTest::AimAt(const FVector& Destination)
{
    APrototypePlayer* Player = Mode->GetPlayer();
    APlayerController* PC = Cast<APlayerController>(Player->GetController());
    if (!PC) return 180.f;
    const float DesiredYaw = (Destination - Player->GetActorLocation()).Rotation().Yaw;
    const float DeltaYaw = FMath::FindDeltaAngleDegrees(PC->GetControlRotation().Yaw, DesiredYaw);
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::MouseX, IE_Axis, FMath::Clamp(DeltaYaw * 5.f, -300.f, 300.f)));
    return FMath::Abs(DeltaYaw);
}

void ABasinPlaythroughTest::AimAndMove(const FVector& Destination, float StopDistance)
{
    const float Error = AimAt(Destination);
    Hold(EKeys::W, Error < 15.f && FVector::Dist2D(Mode->GetPlayer()->GetActorLocation(), Destination) > StopDistance);
}

bool ABasinPlaythroughTest::DriveApproach(float DeltaSeconds)
{
    APrototypePlayer* P = Mode->GetPlayer();
    AIshibashiriBoss* B = Mode->GetBoss();
    UColossusClimbingComponent* C = P->GetClimbing();
    AttackWait = FMath::Max(0.f, AttackWait - DeltaSeconds);
    if (P->GetHealth() != P->MaxHealth || !Mode->IsEncounterActive())
    {
        Fail(TEXT("ground dodge and counter route did not preserve health and encounter"));
        return false;
    }
    const EIshibashiriState State = B->GetState();
    if (LastBossState != static_cast<int32>(State))
    {
        CombatStateTime = 0.f;
        if (State == EIshibashiriState::Telegraph)
        {
            const FVector Right = FRotator(0.f, P->GetController()->GetControlRotation().Yaw, 0.f).RotateVector(FVector::RightVector);
            DodgeSide = FVector::DotProduct(Right, -P->GetActorLocation()) >= 0.f ? EKeys::D : EKeys::A;
        }
        if (State == EIshibashiriState::Charge) bDodgedThisCharge = false;
        LastBossState = static_cast<int32>(State);
    }
    CombatStateTime += DeltaSeconds;
    // Use the existing climbing test's observed dodge/counter route. The driver
    // never grants posture, stops AI, or moves either actor into a fixture.
    if (State == EIshibashiriState::Chase || (State == EIshibashiriState::Recover && !B->CanBeCountered()))
    {
        Hold(EKeys::A, false);
        Hold(EKeys::D, false);
        // Keep bait on the player's side when the boss crosses the basin centre.
        const FVector Bait = B->GetActorLocation() + (P->GetActorLocation() - B->GetActorLocation()).GetSafeNormal2D() * 850.f;
        AimAndMove(Bait, 70.f);
    }
    else if (State == EIshibashiriState::Telegraph)
    {
        Hold(EKeys::W, false);
        AimAt(B->GetActorLocation());
        Hold(DodgeSide, B->GetStateTimeRemaining() < .1f);
    }
    else if (State == EIshibashiriState::Charge)
    {
        if (!bChargeShot) { Shot(TEXT("06-Charge")); bChargeShot = true; }
        Hold(EKeys::W, false);
        if (CombatStateTime < .42f)
        {
            Hold(DodgeSide, true);
            if (CombatStateTime >= .1f && !bDodgedThisCharge)
            {
                Tap(EKeys::LeftShift);
                bDodgedThisCharge = true;
            }
        }
        else
        {
            Hold(DodgeSide, false);
            if (FVector::DotProduct(P->GetActorLocation() - B->GetActorLocation(), B->GetChargeDirection()) < -250.f)
                AimAndMove(B->GetActorLocation(), 300.f);
            else AimAt(B->GetActorLocation());
        }
        // Separate late-charge presentation evidence; keep the comparison shot
        // at its original launch frame and leave every input in its old order.
        if (bCapture && !bChargeEffectsShot && CombatStateTime >= 1.1f
            && FParse::Param(FCommandLine::Get(), TEXT("d3d12")) && FParse::Param(FCommandLine::Get(), TEXT("sm6")))
        {
            Shot(TEXT("08-ChargeEffects"));
            bChargeEffectsShot = true;
        }
    }
    else if (State == EIshibashiriState::Recover)
    {
        Hold(DodgeSide, false);
        const float Error = AimAt(B->GetActorLocation());
        const float Distance = FVector::Dist2D(P->GetActorLocation(), B->GetActorLocation());
        Hold(EKeys::W, Error < 15.f && Distance > 300.f);
        if (B->CanBeCountered() && Distance < 350.f && Error < 8.f && AttackWait <= 0.f)
        {
            Tap(EKeys::LeftMouseButton);
            AttackWait = .5f;
        }
    }
    else if (State == EIshibashiriState::Kneel)
    {
        Hold(EKeys::A, false);
        Hold(EKeys::D, false);
        const FVector Entry = B->GetClimbPosition(0);
        const FVector Outward = (Entry - B->GetActorLocation()).GetSafeNormal2D();
        const FVector Approach = Entry + Outward * 190.f;
        if (!bReachedForelegApproach)
        {
            if (FVector::Dist2D(P->GetActorLocation(), Approach) > 25.f) AimAndMove(Approach, 25.f);
            else { Hold(EKeys::W, false); bReachedForelegApproach = true; }
            return false;
        }
        const float Error = AimAt(Entry);
        const float Distance = FVector::Dist2D(P->GetActorLocation(), Entry);
        const float Angle = FMath::Abs(FMath::FindDeltaAngleDegrees(P->GetActorRotation().Yaw, (-Outward).Rotation().Yaw));
        // Walking inward turns the actor as well as aiming the controller.
        Hold(EKeys::W, Error < 8.f && (Distance > 150.f || (Angle >= C->MaximumWarpAngle && Distance > 80.f)));
        if (B->CanMount() && FVector::Dist(P->GetActorLocation(), Entry) <= C->GrabRange - 12.f
            && Distance <= 150.f && Error < 8.f && Angle < C->MaximumWarpAngle
            && !P->IsAttacking() && !P->IsDodging())
        {
            ReleaseAll();
            Tap(EKeys::E);
            return true;
        }
    }
    return false;
}

bool ABasinPlaythroughTest::IsOnBasinFloor(FString* Detail) const
{
    const APrototypePlayer* P = Mode->GetPlayer();
    const ABasinPrototypeArena* Arena = Mode->GetBasinArena();
    const UCharacterMovementComponent* Movement = P->GetCharacterMovement();
    const UPrimitiveComponent* Floor = Arena->GetFloorComponent();
    const float CapsuleBottom = P->GetActorLocation().Z - P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const float FloorTop = Arena->GetActorLocation().Z;
    const bool bValid = Movement->IsWalking() && Movement->CurrentFloor.IsWalkableFloor()
        && Movement->CurrentFloor.HitResult.GetComponent() == Floor
        && FMath::Abs(CapsuleBottom - FloorTop) <= 5.f;
    if (Detail) *Detail = FString::Printf(TEXT("walking=%d walkable=%d floor=%s expected=%s capsuleBottom=%.2f floorTop=%.2f"),
        Movement->IsWalking(), Movement->CurrentFloor.IsWalkableFloor(),
        *GetNameSafe(Movement->CurrentFloor.HitResult.GetComponent()), *GetNameSafe(Floor), CapsuleBottom, FloorTop);
    return bValid;
}

bool ABasinPlaythroughTest::ValidateRetryReset(FString& Detail) const
{
    const APrototypePlayer* P = Mode->GetPlayer(); const AIshibashiriBoss* B = Mode->GetBoss();
    const UColossusClimbingComponent* C = P->GetClimbing(); const ABasinPrototypeArena* Arena = Mode->GetBasinArena();
    TArray<UStaticMeshComponent*> Meshes; Arena->GetComponents(Meshes);
    TArray<ULightComponent*> Lights; Arena->GetComponents(Lights);
    TArray<UExponentialHeightFogComponent*> Fogs; Arena->GetComponents(Fogs);
    // Reset enters Walking, which calls AdjustFloorHeight synchronously. Keep
    // XY/rotation/scale exact while checking Z against UE's capsule-floor gap.
    FTransform GroundedPlayerStart = PlayerStartTransform;
    FVector GroundedLocation = GroundedPlayerStart.GetLocation();
    GroundedLocation.Z = P->GetActorLocation().Z;
    GroundedPlayerStart.SetLocation(GroundedLocation);
    const float FloorGap = P->GetActorLocation().Z - P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const bool bFloor = P->GetCharacterMovement()->CurrentFloor.IsWalkableFloor()
        && !P->GetCharacterMovement()->CurrentFloor.HitResult.bStartPenetrating
        && P->GetCharacterMovement()->CurrentFloor.HitResult.GetComponent() == Arena->GetFloorComponent()
        && FloorGap >= UCharacterMovementComponent::MIN_FLOOR_DIST - .1f
        && FloorGap <= UCharacterMovementComponent::MAX_FLOOR_DIST + .1f;
    const bool bTransforms = bFloor && P->GetActorTransform().Equals(GroundedPlayerStart, .1f)
        && B->GetActorTransform().Equals(BossStartTransform, .1f);
    const bool bPlayer = P->GetHealth() == P->MaxHealth && !P->IsDodging() && !P->IsAttacking()
        && !P->IsGrabbing() && P->GetCharacterMovement()->IsWalking();
    const bool bBoss = B->GetPosture() == B->MaxPosture && B->GetPurifiedCount() == 0
        && B->GetState() == EIshibashiriState::Chase;
    const bool bInput = !C->IsClimbing() && !C->IsGripping() && !C->HasMovementInput();
    const bool bArena = Meshes.Num() == InitialStaticMeshes && Lights.Num() == InitialLights && Fogs.Num() == InitialFogs
        && Arena->GetVisualRockCount() == InitialRocks && Arena->GetBoundaryCount() == InitialBoundaries
        && Arena->GetAccentCount() == InitialAccents;
    Detail = FString::Printf(TEXT("transforms=%d floorGap=%.2f player=%d boss=%d input=%d arena=%d hp=%d/%d bossHp=%d/%d purified=%d state=%s components(mesh/light/fog)=%d/%d/%d"),
        bTransforms,FloorGap,bPlayer,bBoss,bInput,bArena,P->GetHealth(),P->MaxHealth,B->GetPosture(),B->MaxPosture,
        B->GetPurifiedCount(),*B->GetStateLabel(),Meshes.Num(),Lights.Num(),Fogs.Num());
    return bTransforms && bPlayer && bBoss && bInput && bArena;
}

void ABasinPlaythroughTest::HandleRetryInput(FKey Key)
{
    // InputKey queues an event; this runs when the normal player binding is
    // actually dispatched. Invoke its original handler, then observe reset
    // synchronously before movement or boss Tick can advance the transforms.
    OriginalRetryDelegate.Execute(Key);
    if (!bAwaitingRetry) return;
    bAwaitingRetry = false;
    FString ResetDetail;
    if (!ValidateRetryReset(ResetDetail))
    {
        Fail(*FString::Printf(TEXT("retry reset invalid after player input callback: %s"), *ResetDetail));
        return;
    }
    bRetryObserved = true;
    UE_LOG(LogTemp, Display, TEXT("BASIN_RETRY_RESET %s %s"), *RunId, *ResetDetail);
}

void ABasinPlaythroughTest::RestoreRetryInput()
{
    UInputComponent* Input = Mode && Mode->GetPlayer() ? Mode->GetPlayer()->InputComponent.Get() : nullptr;
    if (Input && RetryBindingHandle != INDEX_NONE)
    {
        for (int32 Index = 0; Index < Input->GetNumActionBindings(); ++Index)
        {
            FInputActionBinding& Binding = Input->GetActionBinding(Index);
            if (Binding.GetHandle() == RetryBindingHandle) { Binding.ActionDelegate = OriginalRetryDelegate; break; }
        }
    }
    RetryBindingHandle = INDEX_NONE;
}

void ABasinPlaythroughTest::Next(EPhase NewPhase, float Timeout)
{
    Hold(EKeys::W, false);
    Hold(EKeys::A, false);
    Hold(EKeys::D, false);
    Phase = NewPhase;
    PhaseTime = 0.f;
    PhaseTimeout = Timeout;
    bDodgedThisCharge = false;
    LastBossState = INDEX_NONE;
    CombatStateTime = AttackWait = 0.f;
    bReachedForelegApproach = false;
}

void ABasinPlaythroughTest::ReleaseAll()
{
    if (!Mode || !Mode->GetPlayer()) { Held.Empty(); PendingRelease.Empty(); return; }
    for (const FKey& Key : Held.Array()) Hold(Key, false);
    PendingRelease.Empty();
    if (APlayerController* PC = Cast<APlayerController>(Mode->GetPlayer()->GetController()))
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::MouseX, IE_Axis, 0.f));
}

FString ABasinPlaythroughTest::Diagnostic() const
{
    if (!Mode || !Mode->GetPlayer() || !Mode->GetBoss()) return TEXT("encounter=invalid");
    const APrototypePlayer* P = Mode->GetPlayer();
    const AIshibashiriBoss* B = Mode->GetBoss();
    const UColossusClimbingComponent* C = P->GetClimbing();
    return FString::Printf(TEXT("phase=%d player=%s boss=%s entryDistance=%.1f bossState=%s climb=%d node=%d moving=%d movement=%d"),
        int32(Phase), *P->GetActorLocation().ToCompactString(), *B->GetActorLocation().ToCompactString(),
        FVector::Dist(P->GetActorLocation(), B->GetClimbPosition(0)), *B->GetStateLabel(), C->IsClimbing(),
        C->GetNode(), C->IsMoving(), int32(P->GetCharacterMovement()->MovementMode));
}

void ABasinPlaythroughTest::Fail(const TCHAR* Reason)
{
    if (bFinished) return;
    UE_LOG(LogTemp, Error, TEXT("BASIN_SCENARIO_FAIL %s reason=%s %s"), *RunId, Reason, *Diagnostic());
    Finish(1);
}

void ABasinPlaythroughTest::Shot(const TCHAR* Name)
{
    if (!bCapture) return;
    const FString Dir = FPaths::ProjectSavedDir() / TEXT("Screenshots/Basin") / RunId;
    IFileManager::Get().MakeDirectory(*Dir, true);
    if (auto* PC = Cast<APlayerController>(Mode->GetPlayer()->GetController()); PC && PC->PlayerCameraManager)
    {
        const FMinimalViewInfo& View = PC->PlayerCameraManager->GetCameraCacheView();
        int32 Width = 0, Height = 0;
        PC->GetViewportSize(Width, Height);
        UE_LOG(LogTemp, Display, TEXT("ENVIRONMENT_CAPTURE_VIEW %s %s arranged=false camera=%s rotation=%s fov=%.2f resolution=%dx%d"),
            *RunId, Name, *View.Location.ToString(), *View.Rotation.ToString(), View.FOV, Width, Height);
    }
    UE_LOG(LogTemp, Display, TEXT("BASIN_CAPTURE_STATE %s %s %s"), *RunId, Name, *Diagnostic());
    FScreenshotRequest::RequestScreenshot(Dir / (FString(Name) + TEXT(".png")), false, false);
}

void ABasinPlaythroughTest::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bFinished || !Mode) return;
    // BeginPlay precedes the first camera update; capture from the real view.
    if (!bStartShot) { Shot(TEXT("01-Start")); bStartShot = true; }
    for (const FKey& Key : PendingRelease) Hold(Key, false);
    PendingRelease.Empty();
    PhaseTime += DeltaSeconds;
    TotalTime += DeltaSeconds;
    if (Mode->GetResult() == EEncounterResult::Defeat) { Fail(TEXT("player defeated during traversal")); return; }
    if (PhaseTime > PhaseTimeout) { Fail(*FString::Printf(TEXT("phase %d timeout"), int32(Phase))); return; }

    APrototypePlayer* P = Mode->GetPlayer();
    AIshibashiriBoss* B = Mode->GetBoss();
    UColossusClimbingComponent* C = P->GetClimbing();
    if ((Phase == EPhase::Approach || Phase == EPhase::Mount || Phase == EPhase::RetryApproach)
        && P->GetCharacterMovement()->IsFalling())
    { Fail(TEXT("unexpected fall while approaching climb hold")); return; }
    switch (Phase)
    {
    case EPhase::Approach:
        if (DriveApproach(DeltaSeconds))
        {
            Shot(TEXT("02-BeforeMount"));
            Next(EPhase::Mount, 2.f);
        }
        break;
    case EPhase::Mount:
        if (C->IsClimbing()) { Hold(EKeys::W, true); Next(EPhase::Climb, 12.f); Hold(EKeys::W, true); }
        break;
    case EPhase::Climb:
        if (!C->IsClimbing()) { Fail(TEXT("unexpected fall before rest ledge")); return; }
        Hold(EKeys::E, B->IsBuckWarning() || B->IsBucking());
        Hold(EKeys::W, true);
        if (C->GetNode() == 3 && !C->IsMoving())
        {
            Hold(EKeys::W, false);
            StaminaAtLedge = C->GetStamina();
            Shot(TEXT("03-FirstLedge"));
            Next(EPhase::Rest, 3.f);
        }
        break;
    case EPhase::Rest:
        if (!C->IsClimbing()) { Fail(TEXT("detached before ledge recovery")); return; }
        Hold(EKeys::E, B->IsBuckWarning() || B->IsBucking());
        if (C->GetNode() != 3 || C->IsMoving() || FVector::Dist(P->GetActorLocation(), B->GetClimbPosition(3)) > 3.f)
        { Fail(TEXT("player did not remain stopped on rest ledge")); return; }
        if (PhaseTime >= 1.f)
        {
            const float Required = FMath::Min(100.f, StaminaAtLedge + 10.f);
            if (C->GetStamina() + 1.f < Required) { Fail(TEXT("stamina did not recover or remain full at rest ledge")); return; }
            Tap(EKeys::SpaceBar);
            Next(EPhase::Detach, 2.f);
        }
        break;
    case EPhase::Detach:
        if (!C->IsClimbing() && P->GetCharacterMovement()->IsFalling()) { bSawFalling = true; Next(EPhase::Land, 8.f); }
        break;
    case EPhase::Land:
        if (bSawFalling && P->GetCharacterMovement()->IsWalking())
        {
            FString FloorDetail;
            if (!IsOnBasinFloor(&FloorDetail)) { Fail(*FString::Printf(TEXT("invalid landing: %s"), *FloorDetail)); return; }
            Shot(TEXT("04-Landed"));
            GroundMoveStart = P->GetActorLocation();
            AimAndMove(B->GetActorLocation());
            Next(EPhase::GroundMove, 3.f);
            Hold(EKeys::W, true);
        }
        break;
    case EPhase::GroundMove:
        AimAndMove(B->GetActorLocation());
        if (!IsOnBasinFloor()) { Fail(TEXT("left basin floor during post-landing movement")); return; }
        if (FVector::Dist2D(GroundMoveStart, P->GetActorLocation()) > 120.f)
        {
            bSawGroundMovement = true; ReleaseAll(); Tap(EKeys::LeftShift); Next(EPhase::GroundDodge, 1.f);
        }
        break;
    case EPhase::GroundDodge:
        if (!bSawGroundMovement) { Fail(TEXT("dodge attempted before post-landing movement succeeded")); return; }
        if (P->IsDodging())
        {
            ReleaseAll();
            bRetryObserved = false;
            bAwaitingRetry = true;
            Tap(EKeys::R);
            Next(EPhase::Retry, 3.f);
        }
        break;
    case EPhase::Retry:
        if (bRetryObserved && FVector::Dist(P->GetActorLocation(), SpawnLocation) < 3.f && !C->IsClimbing()
            && FMath::IsNearlyEqual(C->GetStamina(), 100.f) && P->GetCharacterMovement()->IsWalking()
            && !C->IsGripping() && !C->HasMovementInput())
        {
            Shot(TEXT("05-Retry"));
            Next(EPhase::RetryApproach, 60.f);
        }
        break;
    case EPhase::RetryApproach:
        DriveApproach(DeltaSeconds);
        if (C->IsClimbing())
        {
            ReleaseAll();
            UE_LOG(LogTemp, Display, TEXT("BASIN_SCENARIO_PASS %s %.2fs %s"), *RunId, TotalTime, *Diagnostic());
            Finish(0);
        }
        break;
    case EPhase::Done: break;
    }
}
