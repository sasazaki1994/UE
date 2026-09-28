from pathlib import Path


ROOT = Path(__file__).parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def test_final_design_documents_and_acceptance_exist():
    gameplay = read("Docs/IshibashiriFinalGameplaySpec.md")
    hud = read("Docs/IshibashiriHUDInformationSpec.md")
    camera = read("Docs/IshibashiriCameraSpec.md")
    acceptance = read("Specs/Acceptance/IshibashiriProductionDesign.feature")

    assert "a2d697b9d6593456b84da8905301b2cf257903e3" in gameplay
    assert "remote freshness is therefore **UNVERIFIED**" in gameplay
    assert "1,311 m" in gameplay and "3–4 min median" in gameplay
    assert "15 m/s" in gameplay and "1.65 s" in gameplay and "5.0 s Kneel" in gameplay
    assert "fixed 11-node" in gameplay and "Fall/recovery target" in gameplay
    assert "Victory" in hud and "**完全削除**" in hud
    assert "Normal play" in hud and "Sense held" in hud and "`-DebugGuidance`" in hud
    assert "740 cm" in camera and "88–103°" in camera
    assert "0.35°" in camera and "emergency high shoulder" in camera
    assert "初見合格はRuntime理解Evidenceを必要とする" in acceptance


def test_locked_values_are_traceable_to_current_source():
    boss = read("Source/IshibashiriPrototype/Public/IshibashiriBoss.h")
    player = read("Source/IshibashiriPrototype/Public/PrototypePlayer.h")
    climbing = read("Source/IshibashiriPrototype/Public/ColossusClimbingComponent.h")
    player_cpp = read("Source/IshibashiriPrototype/Private/PrototypePlayer.cpp")

    for value in ("TelegraphDuration = 1.f", "ChargeSpeed = 1500.f", "MaxChargeDuration = 1.35f", "RecoveryDuration = 1.65f", "MountWindowDuration = 5.f"):
        assert value in boss
    for value in ("BuckPeriod = 12.f", "BuckWarningDuration = 2.f", "BuckDuration = 2.f"):
        assert value in boss
    for value in ("HangDrainPerSecond = 5.f", "LedgeRecoveryPerSecond = 22.f", "BracedBuckDrainPerSecond = 18.f", "UnbracedBuckTolerance = .70f"):
        assert value in climbing
    assert "TargetArmLength = 740.f" in player_cpp
    assert "Camera->FieldOfView = 95.f" in player_cpp
    assert "GrabCameraFOV = 88.f" in player
    assert "ClimbingCameraFOV = 100.f" in player


def test_production_target_deltas_are_not_claimed_as_implemented():
    gameplay = read("Docs/IshibashiriFinalGameplaySpec.md")
    assert "only **one** start is implemented" in gameplay
    assert "scoped recovery implementation places the player" in gameplay
    assert "required HUD/presentation delta" in gameplay
    assert "Minimal source changes required before Production gate" in gameplay
