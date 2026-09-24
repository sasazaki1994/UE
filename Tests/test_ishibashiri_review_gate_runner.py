import json
from pathlib import Path
import re

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
        "@('-Action','Test','-Fuchimatoi','-Recovery','-Capture','-SkipBuild','-TestFPS','60')",
        "@('-Action','Test','-Fuchimatoi','-Recovery','-SkipBuild','-TestFPS','30')",
        "@('-Action','Test','-Fuchimatoi','-Recovery','-Gamepad','-SkipBuild','-TestFPS','60')",
        "@('-Action','Test','-Minedaki','-Capture','-SkipBuild','-TestFPS','60')",
        "@('-Action','Test','-Minedaki','-SkipBuild','-TestFPS','30')",
        "@('-Action','Test','-Minedaki','-Gamepad','-SkipBuild','-TestFPS','60')",
        "@('-Action','Test','-Magatsune','-Capture','-SkipBuild','-TestFPS','60')",
        "@('-Action','Test','-Magatsune','-SkipBuild','-TestFPS','30')",
        "@('-Action','Test','-Magatsune','-Gamepad','-SkipBuild','-TestFPS','60')",
        "@('-Action','Package')",
    ]
    positions = [script.index(command) for command in commands]
    assert positions[0] < min(positions[1:])


def test_build_failure_exits_before_gameplay(script: str):
    stop = "if ($Results.build -ne 'PASS') { Save-Summary 'FAIL'; Write-Host \"Evidence: $EvidenceRoot\"; exit 1 }"
    assert stop in script
    assert script.index(stop) < script.index("$Results.approach60 = Invoke-Gate 'approach60'")


def test_package_is_guarded_by_all_major_gates(script: str):
    major_line = next(line for line in script.splitlines() if line.strip().startswith("$Major = @("))
    for gate in ("campaignGamepad", "fuchimatoi60", "fuchimatoi30", "fuchimatoiGamepad",
                 "minedaki60", "minedaki30", "minedakiGamepad", "magatsune60",
                 "magatsune30", "magatsuneGamepad"):
        assert f"'{gate}'" in major_line
    assert "if (!($Major | Where-Object { $Results[$_] -ne 'PASS' }))" in script


def test_evidence_and_status_contract(script: str):
    for directory in ("logs", "screenshots", "performance", "campaign", "approach", "climbing", "ik", "fuchimatoi", "minedaki", "magatsune", "package"):
        assert f"'{directory}'" in script
    assert "summary.json" in script and "environment.json" in script
    assert "-UpdateReviewDocument" not in script  # PowerShell parameter is declared without a call-site flag.
    assert "if ($UpdateReviewDocument)" in script
    assert "-contains 'NOT_RUN'" in script
    assert "{'PARTIAL'}" in script


def test_dry_run_reports_everything_as_not_run_without_launching_processes(script: str):
    assert "[switch]$DryRun" in script
    start = script.index("    if ($DryRun) {", script.index("$Environment.isWindows"))
    dry_run = script[start:script.index("if (!$Environment.isWindows)")]
    assert "Start-Process" not in dry_run
    assert "Save-Summary 'NOT_RUN'" in dry_run
    assert "Unreal validation was NOT_RUN" in dry_run
    planned = re.findall(r"Invoke-Gate '([^']+)'", dry_run)
    assert planned == [
        "build", "approach60", "approach30", "ishibashiri60", "ishibashiri30",
        "gamepad", "climbingIK", "legacy", "highQuality", "campaign60", "campaign30",
        "campaignGamepad", "fuchimatoi60", "fuchimatoi30", "fuchimatoiGamepad",
        "minedaki60", "minedaki30", "minedakiGamepad", "magatsune60", "magatsune30",
        "magatsuneGamepad", "package",
    ]
    assert len(planned) == len(set(planned))
    assert "requiredInputs=$RequiredInputs" in script
    assert "expectedArtifacts=$ExpectedArtifacts" in script
    assert "exitCode=$null" in script


def test_partial_or_not_run_gate_is_not_a_successful_process_result(script: str):
    assert "if ($Verdict -eq 'PASS') { exit 0 } else { exit 1 }" in script


def test_control_rig_is_blocked_not_fabricated(script: str):
    assert "CR_Shirotsura_Climbing.uasset" in script
    assert "$Results.controlRig='BLOCKED'" in script
    assert "New-Item" not in script[script.index("if (Test-Path -LiteralPath $ControlRigFile)"):script.index("$Results.legacy")]


def test_existing_review_json_is_valid_and_unchanged_step_4b():
    review = json.loads((ROOT / "Docs/IshibashiriReviewGate.json").read_text(encoding="utf-8"))
    assert review["gate"].startswith("STEP 4B")
    assert review["verdict"] == "UNVERIFIED — WINDOWS UE RUNTIME NOT AVAILABLE"
