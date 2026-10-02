from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PRIVATE = ROOT / "Source" / "IshibashiriPrototype" / "Private"
HUDS = ("PrototypeHUD.cpp", "FuchimatoiHUD.cpp", "MinedakiHUD.cpp", "MagatsuneHUD.cpp")
LATER_HUDS = HUDS[1:]


def source(name: str) -> str:
    return (PRIVATE / name).read_text(encoding="utf-8")


def block_after(text: str, marker: str) -> str:
    start = text.index(marker)
    brace = text.index("{", start)
    depth = 0
    for index in range(brace, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start:index + 1]
    raise AssertionError(f"unterminated block after {marker}")


def without_debug(text: str) -> str:
    debug = block_after(text, "if (bDebugGuidance)")
    return text.replace(debug, "")


def test_internal_values_and_control_legends_are_debug_only():
    debug_tokens = {
        "PrototypeHUD.cpp": ("HP %d/%d", "STATE %s", "STAMINA %.1f/100", "ROUTE %d -> %d"),
        "FuchimatoiHUD.cpp": ("State=%s", "HP=%d/3", "RouteNode=%d/10", "RecoveryMultiplier=%.1f"),
        "MinedakiHUD.cpp": ("State=%s", "RouteNode=%d/%d", "OpenThrough=%d", "Telemetry Height=", "RecoveryMultiplier=%.1f"),
        "MagatsuneHUD.cpp": ("Phase=%s", "RouteNode=%d/12", "Telemetry Pulses=", "RecoveryMultiplier=%.1f"),
    }
    for name, tokens in debug_tokens.items():
        text = source(name)
        debug = block_after(text, "if (bDebugGuidance)")
        normal = without_debug(text)
        for token in tokens:
            assert token in debug
            assert token not in normal
    for name in LATER_HUDS:
        normal = without_debug(source(name))
        assert "Move WASD/LS" not in normal
        assert "Camera Mouse/RS" not in normal
        assert "Retry R/Y" not in normal


def test_normal_progress_is_transient_and_stamina_is_non_numeric():
    for name in HUDS:
        normal = without_debug(source(name))
        assert 'TEXT("禍根 %d/%d")' in normal
        assert "PurificationNoticeRemaining" in normal
        assert "bShowStamina" in normal
        assert "DrawRect" in normal
        assert 'TEXT("STAMINA' not in normal
        assert 'TEXT("KAKON' not in normal


def test_sense_labels_are_inside_their_active_condition_blocks():
    for name in HUDS:
        text = without_debug(source(name))
        boundary = block_after(text, "if (Player->GetSense()->IsBoundarySenseActive())")
        corruption = block_after(text, "if (Player->GetSense()->IsCorruptionSenseActive())")
        assert "BOUNDARY SENSE" in boundary
        assert "GetBoundaryStrengthLabel" in boundary
        assert "GetCorruptionWarningLabel" in corruption


def test_normal_prompt_is_selected_once_by_an_else_if_queue():
    for name in LATER_HUDS:
        text = without_debug(source(name))
        selection = text[text.index("FString Prompt;"):text.index("if (!Prompt.IsEmpty())")]
        assert selection.count("FString Prompt;") == 1
        assert "else if" in selection
        assert selection.count("Prompt =") >= 3


def test_huds_are_read_only_presentation():
    forbidden = (
        "Purify(",
        "CompleteEncounter(",
        "SetCampaignState",
        "ConsumeStamina(",
        "RestoreStamina(",
        "RefillStamina(",
        "ResetStamina(",
        "SetMaxStamina(",
        "AdvanceRoute(",
        "TryPurify",
        "RetryEncounter(",
    )
    for name in HUDS:
        text = source(name)
        for call in forbidden:
            assert call not in text, f"{name} must not own gameplay authority: {call}"
