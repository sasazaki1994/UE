from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def test_audio_spec_keeps_assets_and_hooks_distinct():
    spec = (ROOT / "Docs/IshibashiriAudioSpec.md").read_text(encoding="utf-8")
    for contract in (
        "Gameplay Critical",
        "Shake / Cling timing contract",
        "Minimum SFX asset set",
        "ASSET_REQUIRED",
        "NOT_RUN",
        "No continuous BGM",
    ):
        assert contract in spec
    assert spec.count("| **Required") >= 10


def test_visual_spec_locks_palette_intensity_and_debug_separation():
    spec = (ROOT / "Docs/IshibashiriVisualLanguageSpec.md").read_text(encoding="utf-8")
    for contract in (
        "Corruption / 禍",
        "Purification",
        "Level 1",
        "Level 2",
        "Level 3",
        "Minimum VFX set (6 reusable systems)",
        "Debug, Production and Accessibility separation",
        "Multichannel critical contract",
        "ASSET_REQUIRED",
        "NOT_RUN",
    ):
        assert contract in spec


def test_acceptance_never_promotes_missing_presentation_assets_to_pass():
    feature = (ROOT / "Specs/Acceptance/IshibashiriProductionDesign.feature").read_text(encoding="utf-8")
    assert "Chargeを視覚と身体音の両方で予告する" in feature
    assert "ShakeとClingの瞬間を複数の手段で読む" in feature
    assert "DebugGuidanceなしで攻略情報を取得する" in feature
    assert "ASSET_REQUIRED" in feature
    assert "NOT_RUN" in feature
