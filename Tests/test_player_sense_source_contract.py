from pathlib import Path

ROOT = Path(__file__).parents[1]


def text(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def test_two_non_conflicting_hold_inputs_are_keyboard_and_gamepad_accessible():
    config = text("Config/DefaultInput.ini")
    for action, keyboard, gamepad in (
        ("BoundarySense", "Q", "Gamepad_LeftTrigger"),
        ("ArmSense", "F", "Gamepad_LeftShoulder"),
    ):
        assert f'ActionName="{action}",Key={keyboard}' in config
        assert f'ActionName="{action}",Key={gamepad}' in config


def test_sense_is_registered_not_world_scanned_and_does_not_own_progress():
    header = text("Source/IshibashiriPrototype/Public/PlayerSenseComponent.h")
    source = text("Source/IshibashiriPrototype/Private/PlayerSenseComponent.cpp")
    assert "RegisterBoundaryTarget" in header
    assert "GetAllActors" not in source
    assert "NushiProgress" not in header + source
    assert "Purify(" not in header + source


def test_risk_is_single_temporary_recovery_multiplier():
    header = text("Source/IshibashiriPrototype/Public/PlayerSenseComponent.h")
    assert "RecoveryMultiplier=.5f" in header
    assert "RiskTailSeconds=2.f" in header
    assert "MaxStamina" not in header
    for player in ("Fuchimatoi", "Minedaki", "Magatsune"):
        source = text(f"Source/IshibashiriPrototype/Private/{player}Player.cpp")
        assert "Sense->GetRecoveryMultiplier()" in source
        assert "Sense->ResetSense()" in source


def test_required_automation_cases_and_hud_labels_exist():
    tests = text("Source/IshibashiriPrototype/Private/PlayerSenseComponentTests.cpp")
    for name in ("BoundarySenseDirection", "BoundarySensePhaseFilter", "CorruptionSenseWarningRiskReset"):
        assert name in tests
    hud = "".join(text(f"Source/IshibashiriPrototype/Private/{name}HUD.cpp") for name in ("Prototype", "Fuchimatoi", "Minedaki", "Magatsune"))
    assert "BOUNDARY SENSE" in hud
    assert "CORRUPTION SENSE" in hud
