from pathlib import Path


ROOT = Path(__file__).parents[1]
PRIVATE = ROOT / "Source/IshibashiriPrototype/Private"
PUBLIC = ROOT / "Source/IshibashiriPrototype/Public"


def test_story_wrap_keeps_layout_width_separate_from_measurement():
    hud = (PRIVATE / "CampaignHUD.cpp").read_text(encoding="utf-8")
    layout = (PRIVATE / "CampaignTextLayout.h").read_text(encoding="utf-8")
    assert "StrLen(Font, TEXT(\"白面\"), SampleWidth, LineHeight)" in hud
    assert "const float DisplayWidth" in hud
    assert "DisplayWidth) / FMath::Max(Scale" in layout
    assert "ContinueTop - BodyTop" in hud


def test_all_encounter_players_use_shared_visual_and_reset_it():
    for player in ("Prototype", "Fuchimatoi", "Minedaki", "Magatsune"):
        source = (PRIVATE / f"{player}Player.cpp").read_text(encoding="utf-8")
        header = (PUBLIC / f"{player}Player.h").read_text(encoding="utf-8")
        assert "UShirotsuraVisualComponent" in header
        assert "ShirotsuraVisual->Configure(GetMesh()" in source
        assert "ShirotsuraVisual->ResetPresentation()" in source


def test_shared_visual_has_mutually_exclusive_fallback_and_state_guard():
    source = (PRIVATE / "ShirotsuraVisualComponent.cpp").read_text(encoding="utf-8")
    assert "Mesh->SetVisibility(bUsingRig" in source
    assert "Fallback->SetVisibility(!bUsingRig" in source
    assert "Next == CurrentState" in source
    assert "ProductionVisuals::ApplyAtBeginPlay" in source
