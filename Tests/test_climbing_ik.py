import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def test_only_required_engine_plugins_are_enabled():
    project = json.loads((ROOT / "IshibashiriPrototype.uproject").read_text())
    enabled = {p["Name"] for p in project["Plugins"] if p.get("Enabled")}
    assert {"ControlRig", "FullBodyIK"} <= enabled
    assert "MotionWarping" in enabled
    assert not ({"Niagara", "PCG"} & enabled)


def test_real_authored_bone_names_are_used():
    rig = (ROOT / "Tools/RigCharacterModels.py").read_text()
    runtime = (ROOT / "Source/IshibashiriPrototype/Private/PrototypePlayer.cpp").read_text()
    for bone in ("root", "pelvis", "spine", "chest"):
        assert f"'{bone}'" in rig
    assert "bone('hand_'+s" in rig
    assert "bone('foot_'+s" in rig
    for effector in ("hand_L", "hand_R", "foot_L", "foot_R"):
        assert effector in runtime


def test_vertical_slice_preserves_route_and_has_local_targets():
    boss = (ROOT / "Source/IshibashiriPrototype/Private/IshibashiriBoss.cpp").read_text()
    climbing = (ROOT / "Source/IshibashiriPrototype/Private/ColossusClimbingComponent.cpp").read_text()
    assert "Node >= 0 && Node <= 3" in climbing
    assert "Frame.TransformPosition(Anchor + Offset)" in climbing
    assert boss.count("{-270,-215,65}") == 1
    assert "IKBlendInSeconds" in (ROOT / "Source/IshibashiriPrototype/Public/ColossusClimbingComponent.h").read_text()
    assert "IKBlendOutSeconds" in (ROOT / "Source/IshibashiriPrototype/Public/ColossusClimbingComponent.h").read_text()
