from pathlib import Path


ROOT = Path(__file__).parents[1]
BOSS_H = (ROOT / "Source/IshibashiriPrototype/Public/IshibashiriBoss.h").read_text(encoding="utf-8")
BOSS = (ROOT / "Source/IshibashiriPrototype/Private/IshibashiriBoss.cpp").read_text(encoding="utf-8")
CLIMB_H = (ROOT / "Source/IshibashiriPrototype/Public/ColossusClimbingComponent.h").read_text(encoding="utf-8")
CLIMB = (ROOT / "Source/IshibashiriPrototype/Private/ColossusClimbingComponent.cpp").read_text(encoding="utf-8")
PLAYER = (ROOT / "Source/IshibashiriPrototype/Private/PrototypePlayer.cpp").read_text(encoding="utf-8")
MODE = (ROOT / "Source/IshibashiriPrototype/Private/PrototypeGameMode.cpp").read_text(encoding="utf-8")


def _shake_state(rider_time: float) -> str:
    phase = rider_time % 12.0
    if rider_time >= 6.0 and phase >= 10.0:
        return "shake"
    if phase >= 8.0:
        return "warning"
    return "safe"


def _advance(rider_time: float, seconds: float, frame_rate: int, protected: bool = False) -> float:
    frames = round(seconds * frame_rate)
    dt = 1.0 / frame_rate
    for _ in range(frames):
        if not protected:
            rider_time += dt
    return rider_time


def test_first_shake_boundary_and_cadence_at_30_and_60_fps():
    assert "FirstBuckDelay = 6.f" in BOSS_H
    assert "RiderTime >= FirstBuckDelay" in BOSS
    for fps in (30, 60):
        rider_time = _advance(0.0, 6.0 - 1.0 / fps, fps)
        assert _shake_state(rider_time) != "shake"
        rider_time = _advance(rider_time, 2.0 + 2.0 / fps, fps)
        assert _shake_state(rider_time) == "warning"
        rider_time = _advance(rider_time, 2.0, fps)
        assert _shake_state(rider_time) == "shake"
        rider_time = _advance(rider_time, 2.0, fps)
        assert _shake_state(rider_time) == "safe"


def test_purification_commit_and_post_completion_delay_are_explicit():
    assert "PurificationDuration = .53f" in CLIMB_H
    assert "PostPurifyShakeDelay = 1.5f" in BOSS_H
    assert "GrantShakeImmunity(FMath::Max(0.f, CompletionDelay))" in BOSS
    assert "GrantShakeImmunity(PostPurifyShakeDelay)" in BOSS
    assert "Boss->CompletePurifyCore(Core)" in CLIMB
    begin = BOSS[BOSS.index("int32 AIshibashiriBoss::BeginPurifyCore") : BOSS.index("bool AIshibashiriBoss::CompletePurifyCore")]
    complete = BOSS[BOSS.index("bool AIshibashiriBoss::CompletePurifyCore") : BOSS.index("void AIshibashiriBoss::NotifyMounted")]
    assert "Kakon->Purify()" not in begin
    assert complete.count("Kakon->Purify()") == 1


def test_protection_pauses_clock_so_warning_and_shake_order_cannot_be_skipped():
    assert "bShakeClockPaused" in BOSS
    assert "bShakeClockPaused ? 0.f : DeltaSeconds" in BOSS
    for fps in (30, 60):
        # Commit one frame before the warning. The clock remains at that boundary
        # for the 0.53 s operation plus its 1.5 s result tail.
        before = _advance(0.0, 8.0 - 1.0 / fps, fps)
        delayed = _advance(before, 0.53 + 1.5, fps, protected=True)
        assert delayed == before
        assert _shake_state(delayed) == "safe"
        assert _shake_state(_advance(delayed, 2.0 / fps, fps)) == "warning"


def test_shake_and_button_mashing_do_not_accept_or_duplicate_purification():
    attempt = CLIMB[CLIMB.index("bool UColossusClimbingComponent::TryPurify") : CLIMB.index("void UColossusClimbingComponent::TickComponent")]
    assert "Boss->IsBucking()" in attempt
    assert "PendingPurificationCore != INDEX_NONE" in attempt
    assert "return false" in attempt
    assert PLAYER.index("if (!Climbing->TryPurify())") < PLAYER.index("AttackRemaining = AttackDuration", PLAYER.index("if (Climbing->IsClimbing())"))
    assert "CoreIndex != GetPurifiedCount()" in BOSS


def test_fall_recovery_and_manual_retry_contracts_remain_intact():
    assert "FallShakeImmunity = 3.f" in CLIMB_H
    assert "FallRecoveryStamina = 50.f" in CLIMB_H
    assert "BeginFallRecoveryWindow(FallRegrabWindow)" in CLIMB
    assert "PendingPurificationCore = INDEX_NONE; PurificationRemaining = 0.f" in CLIMB
    assert "Player->ResetForEncounter(PlayerSpawn())" in MODE
    assert "EncounterManager->ResetEncounter()" in MODE
