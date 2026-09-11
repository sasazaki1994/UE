import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path):
    return (ROOT / path).read_text()


def test_motion_warping_plugin_and_runtime_module_are_enabled():
    project = json.loads(read("IshibashiriPrototype.uproject"))
    enabled = {plugin["Name"] for plugin in project["Plugins"] if plugin.get("Enabled")}
    assert {"PythonScriptPlugin", "ControlRig", "FullBodyIK", "MotionWarping"} <= enabled
    assert '"MotionWarping"' in read("Source/IshibashiriPrototype/IshibashiriPrototype.Build.cs")


def test_grab_target_reuses_route_zero_and_tracks_the_boss():
    climbing = read("Source/IshibashiriPrototype/Private/ColossusClimbingComponent.cpp")
    assert "Candidate->GetClimbPosition(0)" in climbing
    assert "AddOrUpdateWarpTargetFromTransform(GrabWarpTargetName, Target)" in climbing
    assert "MaximumWarpDistance" in climbing and "MaximumWarpAngle" in climbing
    assert "SetActorLocation(Boss->GetClimbPosition(Node))" in climbing  # documented legacy fallback only


def test_warp_has_one_handoff_and_shared_cancel_cleanup():
    climbing = read("Source/IshibashiriPrototype/Private/ColossusClimbingComponent.cpp")
    assert "Boss = Candidate;" in climbing
    assert "RemoveWarpTarget(GrabWarpTargetName)" in climbing
    assert 'CancelGrabWarp(TEXT("Reset"))' in climbing
    assert 'CancelGrabWarp(TEXT("UnsafeState"))' in climbing
    assert "UpdateIK(Dt);\n    UpdateGrabWarp(Dt);" in climbing


def test_asset_contract_does_not_claim_binary_assets_exist():
    docs = read("Docs/GrabMotionWarpValidation.md")
    assert "AM_Shirotsura_Grab_Ishibashiri" in docs
    assert "UNVERIFIED" in docs
    assert not any(ROOT.glob("Content/**/*Grab_Ishibashiri*.uasset"))
