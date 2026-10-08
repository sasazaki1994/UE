from pathlib import Path


ROOT = Path(__file__).parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def test_basin_is_explicit_and_does_not_create_legacy_arena() -> None:
    source = read("Source/IshibashiriPrototype/Private/PrototypeGameMode.cpp")
    assert 'FParse::Param(FCommandLine::Get(), TEXT("BasinPrototype"))' in source
    assert "if (bBasinPrototype)" in source
    assert "else CreateArena();" in source


def test_basin_separates_visual_rocks_from_blocking_boundary() -> None:
    source = read("Source/IshibashiriPrototype/Private/BasinPrototypeArena.cpp")
    assert "Rock->SetCollisionEnabled(ECollisionEnabled::NoCollision)" in source
    assert 'Boundary->SetCollisionProfileName(TEXT("BlockAll"))' in source
    assert "for (int32 I=0; I<8; ++I)" in source
    assert "for (int32 J=-2; J<=2; ++J)" in source


def test_basin_switch_is_forwarded_to_play_and_tests() -> None:
    script = read("Tools/Prototype.ps1")
    assert "[switch]$Basin" in script
    assert script.count("$TestArguments += '-BasinPrototype'") == 1
    assert script.count("$PlayArguments += '-BasinPrototype'") == 1


def test_basin_visual_language_is_readable_and_non_blocking() -> None:
    source = read("Source/IshibashiriPrototype/Private/BasinPrototypeArena.cpp")
    assert 'TEXT("SacredPath%02d")' in source
    assert 'TEXT("RitualStake%d")' in source
    assert "Accent->SetCollisionEnabled(ECollisionEnabled::NoCollision)" in source
    assert 'TEXT("BasinGroundHaze")' in source
    assert "Fog->SetFogMaxOpacity(.22f)" in source


def test_basin_scenario_is_separate_and_requires_basin() -> None:
    script = read("Tools/Prototype.ps1")
    mode = read("Source/IshibashiriPrototype/Private/PrototypeGameMode.cpp")
    assert "[switch]$BasinScenario" in script
    assert "if ($BasinScenario -and !$Basin)" in script
    assert "'-BasinPlaythroughTest'" in script
    assert 'TEXT("BasinPlaythroughTest")' in mode


def test_basin_scenario_does_not_arrange_or_stop_gameplay() -> None:
    source = read("Source/IshibashiriPrototype/Private/BasinPlaythroughTest.cpp")
    assert "SetActorLocation" not in source
    assert "SetActorTickEnabled" not in source
    assert "GrabRange - 12.f" in source
    assert "BASIN_SCENARIO_FAIL" in source
    assert "ReleaseAll();" in source
    assert 'Tap(EKeys::R)' in source


def test_basin_scenario_validates_floor_rest_retry_and_cleanup() -> None:
    source = read("Source/IshibashiriPrototype/Private/BasinPlaythroughTest.cpp")
    assert "CurrentFloor.HitResult.GetComponent() == Floor" in source
    assert "GetScaledCapsuleHalfHeight" in source
    assert "StaminaAtLedge + 10.f" in source
    assert "ValidateRetryReset" in source
    assert "GetPurifiedCount() == 0" in source
    assert "HasMovementInput()" in source
    assert "RestoreExecutionSettings" in source
    assert source.count("DriveApproach(DeltaSeconds)") == 2  # normal approach and approach after Retry


def function_body(source: str, signature: str) -> str:
    opening = source.index("{", source.index(signature))
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[opening + 1:index]
    raise AssertionError(f"unterminated function: {signature}")


def test_basin_approach_drives_real_counter_before_guarded_grab() -> None:
    source = read("Source/IshibashiriPrototype/Private/BasinPlaythroughTest.cpp")
    driver = function_body(source, "bool ABasinPlaythroughTest::DriveApproach(")
    charge = driver.index("else if (State == EIshibashiriState::Charge)")
    recover = driver.index("else if (State == EIshibashiriState::Recover)")
    kneel = driver.index("else if (State == EIshibashiriState::Kneel)")
    assert charge < driver.index("Tap(EKeys::LeftShift)", charge) < recover
    assert recover < driver.index("B->CanBeCountered()", recover) < driver.index("Tap(EKeys::LeftMouseButton)", recover) < kneel
    mount = driver[kneel:]
    assert mount.index("B->CanMount()") < mount.index("Tap(EKeys::E)")
    assert mount.index("Angle < C->MaximumWarpAngle") < mount.index("Tap(EKeys::E)")
    assert "!P->IsAttacking() && !P->IsDodging()" in mount
    for forbidden in ("SetActorLocation", "SetActorRotation", "SetActorTickEnabled", "TryReceiveCounter", "RetryEncounter"):
        assert forbidden not in driver
    assert "CreateSimulated(EKeys::MouseX, IE_Axis" in function_body(source, "float ABasinPlaythroughTest::AimAt(")


def test_basin_retry_restarts_the_input_driver_and_preserves_capture_history() -> None:
    source = read("Source/IshibashiriPrototype/Private/BasinPlaythroughTest.cpp")
    next_phase = function_body(source, "void ABasinPlaythroughTest::Next(")
    for reset in ("LastBossState = INDEX_NONE", "CombatStateTime = AttackWait = 0.f", "bReachedForelegApproach = false"):
        assert reset in next_phase
    assert "bChargeShot" not in next_phase
    tick = function_body(source, "void ABasinPlaythroughTest::Tick(")
    assert tick.count("DriveApproach(DeltaSeconds)") == 2
    assert 'Next(EPhase::RetryApproach, 60.f)' in tick
    script = read("Tools/Prototype.ps1")
    assert "'-BasinPlaythroughTest' { 154 }" in script
    assert "@('01-Start','02-BeforeMount','03-FirstLedge','04-Landed','05-Retry','06-Charge')" in script


def test_basin_charge_capture_observes_charge_and_actual_view() -> None:
    source = read("Source/IshibashiriPrototype/Private/BasinPlaythroughTest.cpp")
    driver = function_body(source, "bool ABasinPlaythroughTest::DriveApproach(")
    charge = driver.index("else if (State == EIshibashiriState::Charge)")
    recover = driver.index("else if (State == EIshibashiriState::Recover)")
    assert 'Shot(TEXT("06-Charge"))' in driver[charge:recover]
    shot = function_body(source, "void ABasinPlaythroughTest::Shot(")
    assert shot.index("GetCameraCacheView()") < shot.index("RequestScreenshot")
    assert "GetViewportSize(Width, Height)" in shot
    assert "camera=%s rotation=%s fov=%.2f resolution=%dx%d" in shot
    assert "BASIN_CAPTURE_STATE" in shot


def test_environment_and_climbing_capture_log_actual_view_and_arrangement() -> None:
    climbing = read("Source/IshibashiriPrototype/Private/ClimbingIntegrationTest.cpp")
    for signature in ("void AClimbingIntegrationTest::Shot(", "void AClimbingIntegrationTest::DemoShot("):
        shot = function_body(climbing, signature)
        assert shot.index("GetCameraCacheView()") < shot.index("RequestScreenshot")
        assert "GetViewportSize(Width, Height)" in shot
        assert 'bCampaignE2E ? TEXT("false") : TEXT("true")' in shot
        assert "CLIMB_CAPTURE_STATE" in shot
    review = function_body(read("Source/IshibashiriPrototype/Private/EnvironmentReviewCapture.cpp"),
                           "void AEnvironmentReviewCapture::Tick(")
    assert review.index("GetCameraCacheView()") < review.index("RequestScreenshot")
    assert "arranged=true camera=%s rotation=%s fov=%.2f resolution=%dx%d" in review


def test_basin_and_climbing_bait_remains_on_the_player_side_of_the_boss() -> None:
    # The boss crosses the world origin from the normal basin start. An origin
    # reference flips the bait behind it, causing a hit before the timed dodge.
    drivers = (
        ("Source/IshibashiriPrototype/Private/BasinPlaythroughTest.cpp", "bool ABasinPlaythroughTest::DriveApproach("),
        ("Source/IshibashiriPrototype/Private/ClimbingIntegrationTest.cpp", "bool AClimbingIntegrationTest::DriveMountWindow("),
    )
    for filename, signature in drivers:
        driver = function_body(read(filename), signature)
        compact = "".join(driver.split())
        assert "B->GetActorLocation()+(P->GetActorLocation()-B->GetActorLocation()).GetSafeNormal2D()*850.f" in compact
        assert "(-B->GetActorLocation()).GetSafeNormal2D()*850.f" not in compact
        assert driver.index("GetHealth()") < driver.index("const EIshibashiriState State")
        assert "Tap(EKeys::LeftShift)" in driver and "Tap(EKeys::LeftMouseButton)" in driver
        for authority in ("TryReceiveCounter", "SetHealth", "ResetForEncounter"):
            assert authority not in driver


def test_basin_retry_snapshot_runs_after_original_input_handler_before_tick() -> None:
    source = read("Source/IshibashiriPrototype/Private/BasinPlaythroughTest.cpp")
    begin = function_body(source, "void ABasinPlaythroughTest::BeginPlay(")
    assert begin.index("OriginalRetryDelegate = Binding.ActionDelegate") < begin.index("Binding.ActionDelegate = FInputActionUnifiedDelegate()")
    assert begin.index("Binding.ActionDelegate = FInputActionUnifiedDelegate()") < begin.index("Binding.ActionDelegate.BindDelegate")
    assert "Binding.ActionDelegate.IsBoundToObject(Mode->GetPlayer())" in begin
    dispatch = function_body(source, "void ABasinPlaythroughTest::HandleRetryInput(")
    assert dispatch.index("OriginalRetryDelegate.Execute(Key)") < dispatch.index("ValidateRetryReset(ResetDetail)")
    assert dispatch.index("ValidateRetryReset(ResetDetail)") < dispatch.index("bRetryObserved = true")
    assert "RetryEncounter" not in dispatch and "RequestScreenshot" not in dispatch
    tick = function_body(source, "void ABasinPlaythroughTest::Tick(")
    retry_start = tick.index("case EPhase::GroundDodge:")
    retry_wait = tick.index("case EPhase::Retry:")
    assert tick.index("bAwaitingRetry = true", retry_start) < tick.index("Tap(EKeys::R)", retry_start)
    assert "ValidateRetryReset" not in tick
    assert 'Shot(TEXT("05-Retry"))' not in tick[retry_start:retry_wait]
    assert 'bRetryObserved &&' in tick[retry_wait:]
    assert tick.index('Shot(TEXT("05-Retry"))', retry_wait) < tick.index("Next(EPhase::RetryApproach", retry_wait)
    for signature in ("void ABasinPlaythroughTest::Finish(", "void ABasinPlaythroughTest::EndPlay("):
        assert "RestoreRetryInput()" in function_body(source, signature)
    restore = function_body(source, "void ABasinPlaythroughTest::RestoreRetryInput(")
    assert "Binding.GetHandle() == RetryBindingHandle" in restore
    assert "Binding.ActionDelegate = OriginalRetryDelegate" in restore


def test_basin_retry_keeps_xy_rotation_scale_exact_and_validates_grounded_z() -> None:
    source = read("Source/IshibashiriPrototype/Private/BasinPlaythroughTest.cpp")
    check = function_body(source, "bool ABasinPlaythroughTest::ValidateRetryReset(")
    assert "GroundedLocation.Z = P->GetActorLocation().Z" in check
    assert "P->GetActorTransform().Equals(GroundedPlayerStart, .1f)" in check
    assert "B->GetActorTransform().Equals(BossStartTransform, .1f)" in check
    assert "CurrentFloor.IsWalkableFloor()" in check
    assert "!P->GetCharacterMovement()->CurrentFloor.HitResult.bStartPenetrating" in check
    assert "const bool bTransforms = bFloor &&" in check
    assert "CurrentFloor.HitResult.GetComponent() == Arena->GetFloorComponent()" in check
    assert "FloorGap >= UCharacterMovementComponent::MIN_FLOOR_DIST - .1f" in check
    assert "FloorGap <= UCharacterMovementComponent::MAX_FLOOR_DIST + .1f" in check
    for forbidden in ("SetActorLocation", "SetActorTransform", "ResetForEncounter"):
        assert forbidden not in check


def test_basin_late_charge_effects_capture_is_separate_once_and_high_quality_only() -> None:
    source = read("Source/IshibashiriPrototype/Private/BasinPlaythroughTest.cpp")
    driver = function_body(source, "bool ABasinPlaythroughTest::DriveApproach(")
    start = driver.index("else if (State == EIshibashiriState::Charge)")
    end = driver.index("else if (State == EIshibashiriState::Recover)")
    charge = driver[start:end]
    effect = charge.index('Shot(TEXT("08-ChargeEffects"))')
    assert charge.index("bCapture && !bChargeEffectsShot && CombatStateTime >= 1.1f") < effect
    assert charge.index('FParse::Param(FCommandLine::Get(), TEXT("d3d12"))') < effect
    assert charge.index('FParse::Param(FCommandLine::Get(), TEXT("sm6"))') < effect
    assert effect < charge.index("bChargeEffectsShot = true")
    assert charge.index('Shot(TEXT("06-Charge"))') < charge.index("Hold(EKeys::W, false)")
    assert charge.index("Tap(EKeys::LeftShift)") < effect
    assert source.count('Shot(TEXT("08-ChargeEffects"))') == 1
    assert "bChargeEffectsShot" not in function_body(source, "void ABasinPlaythroughTest::Next(")
    assert "bool bChargeEffectsShot = false;" in read("Source/IshibashiriPrototype/Public/BasinPlaythroughTest.h")
