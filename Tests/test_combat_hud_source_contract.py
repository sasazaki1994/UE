from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HUD_DIR = ROOT / "Source" / "IshibashiriPrototype" / "Private"


def source(name: str) -> str:
    return (HUD_DIR / name).read_text(encoding="utf-8")


def test_all_encounter_huds_gate_internal_guidance():
    for name in ("PrototypeHUD.cpp", "FuchimatoiHUD.cpp", "MinedakiHUD.cpp", "MagatsuneHUD.cpp"):
        text = source(name)
        assert "IsDebugGuidanceEnabled()" in text
        assert "bDebugGuidance" in text


def test_normal_hud_does_not_use_development_labels():
    fuchimatoi = source("FuchimatoiHUD.cpp")
    minedaki = source("MinedakiHUD.cpp")
    assert "primitive encounter" not in fuchimatoi
    assert "NODE %d/10" not in fuchimatoi
    assert "SAFE NODE" not in minedaki
    assert "KAKON 1 at NODE" not in minedaki
    assert "NEXT ROUTE <= NODE" not in minedaki


def test_variable_hud_rows_size_the_background():
    for name in ("PrototypeHUD.cpp", "FuchimatoiHUD.cpp", "MinedakiHUD.cpp", "MagatsuneHUD.cpp"):
        text = source(name)
        assert "LineCount" in text
        assert "SenseLines" in text


def test_required_player_guidance_remains_available():
    combined = "\n".join(source(name) for name in (
        "PrototypeHUD.cpp", "FuchimatoiHUD.cpp", "MinedakiHUD.cpp", "MagatsuneHUD.cpp"
    ))
    for label in ("STAMINA", "KAKON", "HOLD E / RB", "RECOVERY", "Retry"):
        assert label in combined
