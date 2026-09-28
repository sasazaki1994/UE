from pathlib import Path

ROOT = Path(__file__).parents[1]
CLIMB_H = (ROOT / "Source/IshibashiriPrototype/Public/ColossusClimbingComponent.h").read_text(encoding="utf-8")
CLIMB = (ROOT / "Source/IshibashiriPrototype/Private/ColossusClimbingComponent.cpp").read_text(encoding="utf-8")
BOSS = (ROOT / "Source/IshibashiriPrototype/Private/IshibashiriBoss.cpp").read_text(encoding="utf-8")
MODE = (ROOT / "Source/IshibashiriPrototype/Private/PrototypeGameMode.cpp").read_text(encoding="utf-8")


def test_involuntary_falls_prefer_reached_safe_ledge_and_preserve_progress():
    assert "if (Boss->IsRestNode(Node)) LastSafeNode = Node;" in CLIMB
    recovery = CLIMB[CLIMB.index("void UColossusClimbingComponent::RecoverFromFall") : CLIMB.index("bool UColossusClimbingComponent::TryPurify")]
    assert "GetClimbPosition(LastSafeNode)" in recovery
    assert "Node = LastSafeNode" in recovery
    assert "ResetNushi" not in recovery
    assert "RetryEncounter" not in recovery


def test_fall_recovery_values_and_foreleg_regrab_window_are_locked():
    assert "FallRecoveryStamina = 50.f" in CLIMB_H
    assert "FallShakeImmunity = 3.f" in CLIMB_H
    assert "FallRegrabWindow = 6.f" in CLIMB_H
    assert "GetClimbPosition(0)" in CLIMB
    assert "BeginFallRecoveryWindow(FallRegrabWindow)" in CLIMB
    assert "ShakeImmunityRemaining > 0.f" in BOSS


def test_invalid_anchor_has_stable_spawn_fallback_and_manual_retry_still_resets():
    assert "GetPlayerRecoverySpawn" in CLIMB
    assert "IsUsableRecoveryLocation" in CLIMB
    assert "Player->ResetForEncounter(PlayerSpawn())" in MODE
    assert "EncounterManager->ResetEncounter()" in MODE


def test_shake_and_stamina_use_local_recovery_not_defeat():
    fall_branch = CLIMB[CLIMB.index("if (Stamina <= 0.f || UnsafeBuckTime") :]
    assert 'RecoverFromFall(Stamina <= 0.f ? TEXT("Stamina") : TEXT("Shake"))' in fall_branch
    assert "FinishEncounter(false)" not in fall_branch
