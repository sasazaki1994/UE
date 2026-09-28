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


def test_ishibashiri_production_hud_is_contextual_and_non_numeric():
    text = source("PrototypeHUD.cpp")
    production = text[:text.index("if (bDebugGuidance)")]
    assert "MAGAHARAI / ISHIBASHIRI" not in production
    assert 'TEXT("STAMINA' not in production
    assert 'TEXT("KAKON' not in production
    assert "bShowStamina = Player->IsGrabbing()" in production
    assert "Stamina / 100.f" in production
    assert "PurificationNoticeRemaining = 1.5f" in production
    assert 'TEXT("禍根 %d/%d")' in production


def test_ishibashiri_first_use_prompts_are_queued_by_context():
    text = source("PrototypeHUD.cpp")
    prompt_block = text[text.index("FString Prompt;"):text.index("if (!Prompt.IsEmpty())")]
    # One else-if queue means no two context cards can be selected in a frame.
    assert prompt_block.count("else if") == 4
    assert "!bSawDodge" in prompt_block and "EIshibashiriState::Telegraph" in prompt_block
    assert "!bSawCounter" in prompt_block and "CanBeCountered()" in prompt_block
    assert "!bSawGrab" in prompt_block and "CanMount()" in prompt_block
    assert "!bSawCling" in prompt_block and "IsBuckWarning" not in prompt_block
    assert "bBuckWarning" in prompt_block
    assert "!bSawPurify" in prompt_block and "bPurifiableCoreNear" in prompt_block


def test_ishibashiri_result_and_debug_information_are_separated():
    text = source("PrototypeHUD.cpp")
    assert 'TEXT("VICTORY' not in text
    assert 'TEXT("石走りは鎮まった / Encounter Completed")' in text
    assert 'TEXT("R / Y - Retry encounter")' in text
    debug = text[text.index("if (bDebugGuidance)"):]
    for label in ("DEBUG GUIDANCE", "HP %d/%d", "POSTURE", "STATE", "STAMINA %.1f/100", "ROUTE", "SHAKE"):
        assert label in debug
