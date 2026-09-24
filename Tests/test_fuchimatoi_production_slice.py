from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def test_fuchimatoi_telegraph_uses_creature_motion_and_local_light():
    source = read("Source/IshibashiriPrototype/Private/FuchimatoiBoss.cpp")
    assert "FVector(-155.f,0.f,-55.f)" in source
    assert "GetAttackPresentation()" in source
    assert "HeadCueLight->SetIntensity" in source
    assert "IsDebugGuidanceEnabled() && (ActionState" in source


def test_fuchimatoi_has_recovery_wave_and_camera_framing():
    boss = read("Source/IshibashiriPrototype/Private/FuchimatoiBoss.cpp")
    player = read("Source/IshibashiriPrototype/Private/FuchimatoiPlayer.cpp")
    assert "BiteRecoveryDuration" in boss
    assert "VInterpTo(HeadProxyLocalLocation,FVector::ZeroVector" in boss
    assert "SerpentTime*1.35f-I*.58f" in boss
    assert "SmoothedCameraFocus" in player
    assert "GetPurificationPresentation" in player


def test_purification_envelope_is_shared_and_gameplay_neutral():
    base = read("Source/IshibashiriPrototype/Private/NushiBase.cpp")
    fuchimatoi = read("Source/IshibashiriPrototype/Private/FuchimatoiBoss.cpp")
    assert "NotifyPurificationPresentation" in base
    assert "NotifyPurificationPresentation();" in fuchimatoi
    assert "PurifyPresentationRemaining" in read("Source/IshibashiriPrototype/Private/IshibashiriBoss.cpp")
    assert "PurificationPresentation" not in read("Source/IshibashiriPrototype/Private/NushiProgressComponent.cpp")
