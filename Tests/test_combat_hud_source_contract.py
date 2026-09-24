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
    expected_line_counts = {
        "PrototypeHUD.cpp": "2 + (bCampaign ? 1 : 0) + SenseLines + (bDebugGuidance ? 2 : 0) + (Player && !Player->GetFeedback().IsEmpty() ? 1 : 0)",
        "FuchimatoiHUD.cpp": "4+SenseLines+(bDebugGuidance?2:0)+(bRecoveryBehindCamera?1:0)",
        "MinedakiHUD.cpp": "4 + SenseLines + (bDebugGuidance ? 2 : 0)",
        "MagatsuneHUD.cpp": "4 + SenseLines + (bDebugGuidance ? 1 : 0)",
    }
    for name, line_count in expected_line_counts.items():
        text = source(name)
        assert f"LineCount = {line_count}" in text or f"LineCount={line_count}" in text
        assert "SenseLines" in text


def test_fuchimatoi_counts_offscreen_recovery_guidance():
    text = source("FuchimatoiHUD.cpp")
    assert "bRecoveryBehindCamera=Boss->IsRecoveryUnlocked()" in text
    assert "Project(Boss->GetRecoveryAnchor()->GetActorLocation()).Z<=0" in text
    assert "(bRecoveryBehindCamera?1:0)" in text
    assert "Gold beacon is behind the camera - turn to find it" in text


def test_required_player_guidance_remains_available():
    combined = "\n".join(source(name) for name in (
        "PrototypeHUD.cpp", "FuchimatoiHUD.cpp", "MinedakiHUD.cpp", "MagatsuneHUD.cpp"
    ))
    for label in ("STAMINA", "KAKON", "HOLD E / RB", "RECOVERY", "Retry"):
        assert label in combined
