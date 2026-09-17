from pathlib import Path

ROOT = Path(__file__).parents[1]


def source(path):
    return (ROOT / path).read_text(encoding="utf-8")


def test_runtime_assets_remain_rigged_baseline():
    player = source("Source/IshibashiriPrototype/Private/PrototypePlayer.cpp")
    boss = source("Source/IshibashiriPrototype/Private/IshibashiriBoss.cpp")
    assert "/Game/Characters/Rigged/Shirotsura/SK_Shirotsura" in player
    assert "/Game/Characters/Rigged/Ishibashiri/SK_Ishibashiri" in boss
    assert "/Reference/" not in player


def test_sense_visual_roles_are_separate_and_read_only():
    player = source("Source/IshibashiriPrototype/Private/PrototypePlayer.cpp")
    assert "BoundaryBladeReaction" in player
    assert "GetBoundaryReading().Strength" in player
    assert "CorruptedArmReaction" in player
    assert "ECorruptionWarning::Danger" in player
    assert "DrawDebugDirectionalArrow" not in player


def test_kakon_debug_and_normal_presentations_are_split():
    boss = source("Source/IshibashiriPrototype/Private/IshibashiriBoss.cpp")
    assert "IsDebugGuidanceEnabled()" in boss
    assert "KakonState!=EKakonState::Purified" in boss
    assert "SenseStrength" in boss
    assert "EKakonState::Covered" in boss
    assert "PurifyPresentationRemaining[I]=.35f" in boss
    hud = source("Source/IshibashiriPrototype/Private/PrototypeHUD.cpp")
    assert "bDebugGuidance" in hud
    assert "STAMINA  %.0f / 100        KAKON  %d / 3" in hud


def test_basin_scale_and_calm_hooks_are_presentation_only():
    basin = source("Source/IshibashiriPrototype/Private/BasinPrototypeArena.cpp")
    mode = source("Source/IshibashiriPrototype/Private/PrototypeGameMode.cpp")
    assert "OldCedar%02d" in basin
    assert "SetCalmPresentation" in basin
    assert "SetCalmPresentation(true)" in mode
    assert "SetCalmPresentation(false)" in mode


def test_sense_and_kakon_presentation_continue_without_normal_play_primitives():
    player = source("Source/IshibashiriPrototype/Private/PrototypePlayer.cpp")
    boss = source("Source/IshibashiriPrototype/Private/IshibashiriBoss.cpp")
    assert "SetVisibility(Sense->IsBoundarySenseActive() && !bWeaponHidden)" in player
    assert "SetVisibility(Sense->IsCorruptionSenseActive())" in player
    rider_branch = boss.split("if (Target->IsGrabbing()", 1)[1].split("return;", 1)[0]
    assert "UpdateVisuals();" in rider_branch


def test_route_contract_still_has_eleven_nodes():
    boss = source("Source/IshibashiriPrototype/Private/IshibashiriBoss.cpp")
    route = boss.split("const FVector Route[] = {", 1)[1].split("};", 1)[0]
    assert route.count("{") == 11
