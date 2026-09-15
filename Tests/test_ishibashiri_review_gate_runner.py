import json
from pathlib import Path

import pytest

ROOT = Path(__file__).parents[1]
RUNNER = ROOT / "Tools/RunIshibashiriReviewGate.ps1"


@pytest.fixture(scope="module")
def script() -> str:
    return RUNNER.read_text(encoding="utf-8")


def test_required_commands_and_order(script: str):
    commands = [
        "@('-Action','Build')",
        "@('-Action','Test','-Approach','-TestFPS','60')",
        "@('-Action','Test','-Approach','-TestFPS','30')",
        "@('-Action','Test','-Climbing','-Basin','-SkipBuild','-TestFPS','60')",
        "@('-Action','Test','-Climbing','-Basin','-SkipBuild','-TestFPS','30')",
        "@('-Action','Test','-ClimbingGamepad','-Basin','-SkipBuild','-TestFPS','60')",
        "@('-Action','Test','-Climbing','-Capture','-Basin','-HighQuality','-SkipBuild','-TestFPS','60')",
        "@('-Action','Test','-Campaign','-SkipBuild','-TestFPS','60')",
        "@('-Action','Test','-Campaign','-SkipBuild','-TestFPS','30')",
        "@('-Action','Test','-Campaign','-SkipBuild','-Gamepad','-TestFPS','60')",
        "@('-Action','Package')",
    ]
    positions = [script.index(command) for command in commands]
    assert positions[0] < min(positions[1:])


def test_build_failure_exits_before_gameplay(script: str):
    stop = "if ($Results.build -ne 'PASS') { Save-Summary 'FAIL'; Write-Host \"Evidence: $EvidenceRoot\"; exit 1 }"
    assert stop in script
    assert script.index(stop) < script.index("Invoke-Gate 'approach60'")


def test_package_is_guarded_by_all_major_gates(script: str):
    assert "$Major = @('approach60','approach30','ishibashiri60','ishibashiri30','gamepad','retry','senseReset','legacy','highQuality','campaign60','campaign30','campaignGamepad')" in script
    assert "if (!($Major | Where-Object { $Results[$_] -ne 'PASS' }))" in script


def test_evidence_and_status_contract(script: str):
    for directory in ("logs", "screenshots", "performance", "campaign", "approach", "climbing", "ik", "package"):
        assert f"'{directory}'" in script
    assert "summary.json" in script and "environment.json" in script
    assert "-UpdateReviewDocument" not in script  # PowerShell parameter is declared without a call-site flag.
    assert "if ($UpdateReviewDocument)" in script
    assert "-contains 'NOT_RUN'" in script
    assert "{'PARTIAL'}" in script


def test_control_rig_is_blocked_not_fabricated(script: str):
    assert "CR_Shirotsura_Climbing.uasset" in script
    assert "$Results.controlRig='BLOCKED'" in script
    assert "New-Item" not in script[script.index("if (Test-Path -LiteralPath $ControlRigFile)"):script.index("$Results.legacy")]


def test_existing_review_json_is_valid_and_unchanged_step_4b():
    review = json.loads((ROOT / "Docs/IshibashiriReviewGate.json").read_text(encoding="utf-8"))
    assert review["gate"].startswith("STEP 4B")
    assert review["verdict"] == "UNVERIFIED — WINDOWS UE RUNTIME NOT AVAILABLE"
