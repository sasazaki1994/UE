from pathlib import Path

ROOT = Path(__file__).parents[1]
HEADER = (ROOT / "Source/IshibashiriPrototype/Public/IshibashiriBoss.h").read_text()
BOSS = (ROOT / "Source/IshibashiriPrototype/Private/IshibashiriBoss.cpp").read_text()
PLAYER = (ROOT / "Source/IshibashiriPrototype/Private/PrototypePlayer.cpp").read_text()
CLIMB = (ROOT / "Source/IshibashiriPrototype/Private/ColossusClimbingComponent.cpp").read_text()
HUD = (ROOT / "Source/IshibashiriPrototype/Private/PrototypeHUD.cpp").read_text()


def test_posture_is_local_and_body_health_victory_is_removed():
    assert "MaxPosture = 3" in HEADER
    assert "Posture = MaxPosture" in BOSS
    assert "Health = FMath::Max(0, Health - 1)" not in BOSS
    counter = BOSS[BOSS.index("bool AIshibashiriBoss::TryReceiveCounter") : BOSS.index("void AIshibashiriBoss::UpdateVisuals")]
    assert "CalmNushi" not in counter
    assert "EnterState(EIshibashiriState::Kneel)" in counter
    assert "Boss HP" not in PLAYER + HUD


def test_only_recover_counter_and_kneel_mount_window():
    assert "GetState() == EIshibashiriState::Recover" in HEADER
    assert "!bChargeHitPlayer && !bCounterUsed && Posture > 0" in HEADER
    assert "MountWindowDuration = 5.f" in HEADER
    assert "case EIshibashiriState::Kneel" in BOSS
    assert "Posture = MaxPosture;" in BOSS
    assert "GrabMarker->SetVisibility(DisplayState == EIshibashiriState::Kneel)" in BOSS
    assert "CorruptionBulges" in HEADER + BOSS


def test_new_grab_is_gated_but_success_uses_existing_route():
    assert "if (!Candidate->CanMount()) return;" in CLIMB
    assert "!Candidate->CanMount()" in CLIMB
    assert CLIMB.count("Boss->NotifyMounted();") == 2
    assert "Node = 0; Destination = INDEX_NONE" in CLIMB
    assert "if (Target->IsGrabbing() && !Target->GetClimbing()->IsGrabWarping())" in BOSS


def test_sense_and_feedback_describe_posture_not_damage():
    assert "State == EIshibashiriState::Kneel" in PLAYER
    assert "SenseBoss->IsBucking()" in PLAYER
    assert "DEFLECTED - evade the charge first" in PLAYER
    assert "POSTURE BROKEN - GRAB NOW" in PLAYER
    assert "POSTURE BREAK %d/%d" in PLAYER


def test_shared_three_kakon_completion_remains_the_only_gameplay_completion():
    assert "Kakon->Purify()" in BOSS
    assert "CalmNushi" not in BOSS[BOSS.index("bool AIshibashiriBoss::TryPurifyCore") :]
    assert "OnEncounterCompleted.AddUniqueDynamic" in (ROOT / "Source/IshibashiriPrototype/Private/PrototypeGameMode.cpp").read_text()
    spec = (ROOT / "Specs/Acceptance/ColossusClimbing.feature").read_text()
    assert "地上斬撃と部分浄化では勝利しない" in spec
    assert "Mount Windowを逃す" in spec
