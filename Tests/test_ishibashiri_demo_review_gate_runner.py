from pathlib import Path

ROOT = Path(__file__).parents[1]
RUNNER = ROOT / "Tools/RunIshibashiriDemoReviewGate.ps1"


def script() -> str:
    return RUNNER.read_text(encoding="utf-8")


def test_dedicated_runner_and_all_independent_results_exist():
    text = script()
    assert RUNNER.exists()
    for gate in ("build", "demo60", "demo30", "demoGamepad", "demoCapture", "demoHighQuality",
                 "demoCompletion", "saveIsolation", "retry", "senseReset",
                 "normalCampaignRegression", "package"):
        assert f"{gate}='NOT_RUN'" in text


def test_build_is_first_and_failure_stops_gameplay():
    text = script()
    build = "$Results.build=Invoke-Gate 'build'"
    stop = "if($Results.build -ne 'PASS'){Save-Summary 'FAIL';exit 1}"
    assert text.index(build) < text.index(stop) < text.index("$Results.demo60=Invoke-Gate")


def test_demo_matrix_capture_hq_and_minimal_campaign_are_exact():
    text = script()
    expected = (
        "@('-Action','Test','-IshibashiriDemo','-SkipBuild','-TestFPS','60')",
        "@('-Action','Test','-IshibashiriDemo','-SkipBuild','-TestFPS','30')",
        "@('-Action','Test','-IshibashiriDemo','-Gamepad','-SkipBuild','-TestFPS','60')",
        "@('-Action','Test','-IshibashiriDemo','-Capture','-SkipBuild','-TestFPS','60')",
        "@('-Action','Test','-IshibashiriDemo','-Capture','-HighQuality','-SkipBuild','-TestFPS','60')",
        "@('-Action','Test','-Campaign','-SkipBuild','-TestFPS','60')",
    )
    for command in expected:
        assert command in text
    for boss in ("-Fuchimatoi", "-Minedaki", "-Magatsune"):
        assert boss not in text


def test_package_requires_every_demo_gate():
    text = script()
    major = next(line for line in text.splitlines() if line.strip().startswith("$Required=@("))
    for gate in ("build", "demo60", "demo30", "demoGamepad", "demoCapture", "demoHighQuality",
                 "demoCompletion", "saveIsolation", "retry", "senseReset", "normalCampaignRegression"):
        assert f"'{gate}'" in major
    assert "if(!($Required|Where-Object{$Results[$_] -ne 'PASS'}))" in text


def test_dry_run_never_starts_a_process_and_leaves_all_not_run():
    text = script()
    dry = text[text.index("if($DryRun){ foreach($Step"):text.index("if(!$Environment.isWindows)")]
    assert "Start-Process" not in dry
    assert "Save-Summary 'NOT_RUN'" in dry
    assert "all gates NOT_RUN" in dry


def test_runtime_contracts_require_markers_not_only_exit_zero():
    text = script()
    assert "ISHIBASHIRI_DEMO_E2E_PASS" in text
    assert "CAMPAIGN_SAVED|CAMPAIGN_CONTINUE|CAMPAIGN_SAVE_CLEAR_FAILED" in text
    assert "CLIMB_TEST_PASS" in text
    assert "PLAYER_SENSE reset" in text
    for renderer in ("D3D12", "SM6", "Lumen.*GI", "Lumen.*Reflection", "Virtual Shadow Map"):
        assert renderer in text


def test_evidence_has_sha_commands_streams_and_artifacts():
    text = script()
    for value in ("sourceSha=$SourceSha", "command=$Command", "startTime=", "exitCode=",
                  "stdout=", "stderr=", "expectedArtifact=", "actualArtifact=", "result="):
        assert value in text
    for folder in ("'logs'", "'screenshots'", "'performance'", "'demo'", "'campaign-regression'", "'package'"):
        assert folder in text


def test_control_rig_is_only_an_optional_warning():
    text = script()
    assert "BLOCKED / OPTIONAL QUALITY GATE" in text
    assert "does not block demo packaging" in text
    assert "ControlRig" not in text


def test_demo_capture_requires_all_fourteen_real_png_files():
    prototype = (ROOT / "Tools/Prototype.ps1").read_text(encoding="utf-8")
    for shot in ("01-Title", "02-Prologue", "03-Approach", "04-IshibashiriReveal", "05-Charge",
                 "06-GroundGrab", "07-Climbing", "08-Kakon1", "09-Kakon2", "10-Kakon3",
                 "11-Calm", "12-DemoEnding1", "13-DemoEnding2", "14-ReturnToTitle"):
        assert f"'{shot}'" in prototype
    assert 'Get-Item -LiteralPath (Join-Path $DemoCaptureDir "$Name.png") -ErrorAction Stop' in prototype
    assert "$Shot.Length -lt 100" in prototype
