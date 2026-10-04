from pathlib import Path

ROOT = Path(__file__).parents[1]
RUNNER = (ROOT / "Tools/Prototype.ps1").read_text(encoding="utf-8")


def section(start: str, end: str) -> str:
    begin = RUNNER.index(start)
    return RUNNER[begin : RUNNER.index(end, begin)]


def test_automated_runs_disable_physical_gamepad_input():
    test = section("'Test' {", "'Package' {")
    launch = test.index("$TestArguments = @(")
    assert "'-DisablePlugins=XInputDevice'" in test[launch : test.index("\n", launch)]
    run = test.index("Invoke-TimedTest -Program $EditorCmd -Arguments $TestArguments")
    guard = test.index("'Mounting Engine plugin XInputDevice'")
    assert run < guard < test.index("$PassMarker =")


def test_human_play_and_editor_keep_physical_gamepads():
    assert "XInputDevice" not in section("'Play' {", "'Editor' {")
    assert "XInputDevice" not in section("'Editor' {", "'Test' {")


def test_simulated_gamepad_drivers_do_not_depend_on_the_device_plugin():
    for driver in ("ClimbingIntegrationTest.cpp", "PrototypeGamepadTest.cpp"):
        source = (ROOT / "Source/IshibashiriPrototype/Private" / driver).read_text(encoding="utf-8")
        assert "FInputKeyEventArgs::CreateSimulated" in source
        assert "XInput" not in source
