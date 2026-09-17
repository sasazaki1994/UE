import re
from pathlib import Path

ROOT = Path(__file__).parents[1]
def read(path: str) -> str: return (ROOT / path).read_text(encoding="utf-8")
def compact(text: str) -> str: return re.sub(r"\s+", "", text)

def test_approach_is_self_contained_and_primitive_only():
    header = read("Source/IshibashiriPrototype/Public/IshibashiriApproachArena.h")
    source = read("Source/IshibashiriPrototype/Private/IshibashiriApproachArena.cpp")
    assert "AIshibashiriApproachArena" in header
    for item in ("WetEarth", "BoundaryStone", "RitualPost", "GiantFootprint", "GougedRock", "ShatteredCedar", "CorruptionCrack"):
        assert item in source
    assert "/Engine/BasicShapes/" in source
    assert "/Game/" not in source

def test_reveal_is_once_non_combat_and_three_seconds():
    source = read("Source/IshibashiriPrototype/Private/IshibashiriApproachArena.cpp")
    assert "Stage<3" in compact(source)
    assert "RevealCount++" in source
    assert "RevealElapsed>=3.f" in compact(source)
    assert "combat=false" in source
    assert "AIshibashiriBoss" not in source

def test_gate_owns_only_thin_campaign_transition():
    mode = read("Source/IshibashiriPrototype/Private/IshibashiriApproachGameMode.cpp")
    campaign = read("Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp")
    assert "CompleteApproach" in mode and "TravelToCurrentChapter" in mode
    assert "IshibashiriApproachGameMode" in campaign
    assert "ECampaignState::IshibashiriApproach" in campaign

def test_approach_sense_and_audio_hooks_are_explicit():
    mode = read("Source/IshibashiriPrototype/Private/IshibashiriApproachGameMode.cpp")
    arena = read("Source/IshibashiriPrototype/Private/IshibashiriApproachArena.cpp")
    assert "RegisterBoundaryTarget" in mode
    assert "ECorruptionWarning::Transition" in mode and "ECorruptionWarning::Danger" in mode
    assert "NOT_IMPLEMENTED — AUDIO ASSET REQUIRED" in arena
