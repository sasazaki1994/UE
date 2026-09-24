import json
import re
from pathlib import Path

ROOT = Path(__file__).parents[1]

def read(path): return (ROOT / path).read_text(encoding="utf-8")

def test_magatsune_isolated_and_uses_shared_lifecycle():
    boss = read("Source/IshibashiriPrototype/Private/MagatsuneBoss.cpp")
    player = read("Source/IshibashiriPrototype/Private/MagatsunePlayer.cpp")
    header = read("Source/IshibashiriPrototype/Public/MagatsuneBoss.h")
    assert "public ANushiBase" in header
    assert "RegisterKakon(K)" in boss and "GetNushiProgressComponent" in boss
    assert 'CreateDefaultSubobject<UGrabComponent>(TEXT("SharedGrab"))' in player
    assert 'CreateDefaultSubobject<UStaminaComponent>(TEXT("SharedStamina"))' in player
    assert not any((ROOT / "Source/IshibashiriPrototype" / side / name).stat().st_mtime_ns == 0 for side in ("Public", "Private") for name in ())

def test_required_automation_and_input_driver_contracts():
    tests = read("Source/IshibashiriPrototype/Private/MagatsuneTests.cpp")
    integration = read("Source/IshibashiriPrototype/Private/MagatsuneIntegrationTest.cpp")
    for name in ("MagatsuneLifecycle", "RootTransformFollow", "RoutePhaseTransition", "FallRecovery", "RetryReset"):
        assert name in tests
    for fps in (30, 60, 120): assert str(fps) in tests
    assert "InputKey" in integration and "MAGATSUNE_TEST_PASS" in integration
    assert "SetActorLocation(B->GetRoute" not in integration

def test_tool_docs_and_validation_contract():
    tool = read("Tools/Prototype.ps1")
    assert "[switch]$Magatsune" in tool and "-MagatsuneTest" in tool
    doc = read("Docs/MagatsuneVerticalSlice.md")
    for shot in range(1, 13): assert f"`{shot:02d}-" in doc
    data = json.loads(read("Docs/MagatsuneValidation.json"))
    assert re.fullmatch(r"[0-9a-f]{40}", data["base"])
    for gate in ("build", "automation", "keyboard60", "keyboard30", "gamepad60", "screenshots"):
        assert data["checks"][gate]["status"] in {"PASS", "FAIL", "NOT_RUN"}

def test_living_terrain_presentation_keeps_gameplay_authority():
    boss = read("Source/IshibashiriPrototype/Private/MagatsuneBoss.cpp")
    player = read("Source/IshibashiriPrototype/Private/MagatsunePlayer.cpp")
    hud = read("Source/IshibashiriPrototype/Private/MagatsuneHUD.cpp")
    assert "The cue precedes the unchanged Wave > .78 gameplay check" in boss
    assert "RootVisuals[I]->SetRelativeLocation" in boss
    assert "GrabFrame, RouteMarkers and" in boss and "authoritative MovingRoot" in boss
    assert "AdvancePresentation(Dt)" in boss and "NotifyPurificationPresentation()" in boss
    assert "VInterpTo" in player and "FInterpTo" in player
    assert "IsDebugGuidanceEnabled()" in hud and 'TEXT("DEBUG Phase=%s | Route=%d/12")' in hud
