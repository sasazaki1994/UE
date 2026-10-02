"""Static contracts for Corruption Sense in Ishibashiri's legacy stamina path."""

import re
from pathlib import Path


ROOT = Path(__file__).parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def function_body(source: str, class_name: str, function_name: str) -> str:
    """Extract one C++ member body by balancing braces."""
    signature = re.search(
        rf"\b{re.escape(class_name)}::{re.escape(function_name)}\s*\([^)]*\)\s*(?:const\s*)?\{{",
        source,
    )
    assert signature, f"missing {class_name}::{function_name}"
    opening = source.find("{", signature.start())
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[opening + 1 : index]
    raise AssertionError(f"unclosed body for {class_name}::{function_name}")


def compact(source: str) -> str:
    return re.sub(r"\s+", "", source)


def test_tick_applies_player_sense_multiplier_only_to_natural_recovery():
    source = read("Source/IshibashiriPrototype/Private/ColossusClimbingComponent.cpp")
    tick = compact(function_body(source, "UColossusClimbingComponent", "TickComponent"))

    assert "Player->GetSense()->GetRecoveryMultiplier()" in tick
    assert "GroundRecoveryPerSecond*RecoveryMultiplier*Dt" in tick
    assert "-LedgeRecoveryPerSecond*RecoveryMultiplier" in tick
    assert "HangDrainPerSecond*RecoveryMultiplier" not in tick
    assert "BracedBuckDrainPerSecond*RecoveryMultiplier" not in tick
    assert "UnbracedBuckDrainPerSecond*RecoveryMultiplier" not in tick


def test_fixed_fall_recovery_does_not_use_sense_multiplier():
    source = read("Source/IshibashiriPrototype/Private/ColossusClimbingComponent.cpp")
    recovery = compact(function_body(source, "UColossusClimbingComponent", "RecoverFromFall"))

    assert "Stamina=FMath::Clamp(FallRecoveryStamina,0.f,100.f)" in recovery
    assert "RecoveryMultiplier" not in recovery
    assert "GetRecoveryMultiplier" not in recovery


def test_boundary_sense_has_no_separate_recovery_multiplier():
    header = read("Source/IshibashiriPrototype/Public/PlayerSenseComponent.h")
    getter = compact(header[header.index("float GetRecoveryMultiplier()") :])

    assert "IsRecoveryRiskActive()?RecoveryMultiplier:1.f" in getter
    assert "bBoundaryActive" not in getter.split(";", 1)[0]
    assert not re.search(r"Boundary\w*RecoveryMultiplier|RecoveryMultiplier\w*Boundary", header)


def test_retry_resets_sense_before_climbing_stamina():
    source = read("Source/IshibashiriPrototype/Private/PrototypePlayer.cpp")
    reset = compact(function_body(source, "APrototypePlayer", "ResetForEncounter"))

    assert "Sense->ResetSense()" in reset
    assert reset.index("Sense->ResetSense()") < reset.index("Climbing->Reset()")
