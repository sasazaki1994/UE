import re
from pathlib import Path

ROOT = Path(__file__).parents[1]
def read(path: str) -> str: return (ROOT / path).read_text(encoding="utf-8")
def compact(text: str) -> str: return re.sub(r"\s+", "", text)

def test_approach_is_self_contained_with_primitive_fallbacks():
    header = read("Source/IshibashiriPrototype/Public/IshibashiriApproachArena.h")
    source = read("Source/IshibashiriPrototype/Private/IshibashiriApproachArena.cpp")
    assert "AIshibashiriApproachArena" in header
    for item in ("WetEarth", "BoundaryStone", "RitualPost", "GiantFootprint", "GougedRock", "ShatteredCedar", "CorruptionCrack"):
        assert item in source
    assert "/Engine/BasicShapes/" in source
    assert source.count("/Game/Environment/Ishibashiri/") == 3

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

def test_approach_sense_target_owns_the_basin_gate_transform():
    mode = read("Source/IshibashiriPrototype/Private/IshibashiriApproachGameMode.cpp")
    assert "SpawnActor<ATargetPoint>" in mode
    assert "SenseTarget->GetRootComponent()" in mode
    assert "SenseTarget->GetActorLocation().Equals(Arena->GetBasinGate(), 1.f)" in mode

def test_approach_prefers_validated_environment_assets_with_fallbacks():
    arena = read("Source/IshibashiriPrototype/Private/IshibashiriApproachArena.cpp")
    for asset in ("SM_Ishibashiri_OldCedar_A", "SM_Ishibashiri_Rock_A", "SM_Ishibashiri_BoundaryStone_A"):
        assert "/Game/Environment/Ishibashiri/" + asset in arena
    assert "CedarMesh ?" not in arena  # tree replacement has an explicit early-return fallback
    assert "RockMesh ? RockMesh.Get() : SphereMesh.Get()" in arena
    assert "BoundaryStoneMesh ? BoundaryStoneMesh.Get() : CubeMesh.Get()" in arena
    assert "if (bBlockoutMesh) C->SetMaterial" in arena

def test_approach_warning_subtitle_is_centered_and_viewport_safe():
    hud = read("Source/IshibashiriPrototype/Private/IshibashiriApproachHUD.cpp")
    assert "GetTextSize(S, TextWidth, TextHeight" in hud
    assert "AvailableWidth / FMath::Max(1.f, TextWidth)" in hud
    assert "(Canvas->ClipX - PanelWidth) * .5f" in hud
    assert "DrawRect(FLinearColor(.01f, .015f, .018f, .82f)" in hud
    assert "DrawLine(PanelX, PanelY, PanelX + PanelWidth" in hud
