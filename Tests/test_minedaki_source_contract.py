"""Source-level guards for the Minedaki 3/3 validation harness.

These checks do not replace Unreal automation.  They keep the documented route
and capture contract reviewable on hosts where UE 5.6.1 is unavailable.
"""

from pathlib import Path
import re


ROOT = Path(__file__).parents[1]


def read(relative_path: str) -> str:
    return (ROOT / relative_path).read_text(encoding="utf-8-sig")


def compact(text: str) -> str:
    """Token-level view: the contract must survive clang-format whitespace changes."""
    return re.sub(r"\s+", "", text)


def test_route_has_fourteen_nodes_and_three_ordered_kakon_nodes() -> None:
    header = compact(read("Source/IshibashiriPrototype/Public/MinedakiBoss.h"))
    source = compact(read("Source/IshibashiriPrototype/Private/MinedakiBoss.cpp"))
    route = re.search(r"staticconstFVectorNodes\[\]=\{(.*?)\};", source, re.DOTALL)

    assert route is not None
    assert len(re.findall(r"\{[^{}]+\}", route.group(1))) == 14
    assert "RouteNodeCount=14" in header
    assert "Kakon1Node=6" in header
    assert "Kakon2Node=10" in header
    assert "Kakon3Node=13" in header


def test_driver_covers_two_recoveries_and_every_capture_stage() -> None:
    driver = read("Source/IshibashiriPrototype/Private/MinedakiIntegrationTest.cpp")
    prototype_tool = read("Tools/Prototype.ps1")
    capture_names = (
        "01-GroundGrab", "02-Phase1WallClimb", "03-FirstShake", "04-Kakon1",
        "05-Phase2BodyTransition", "06-ArmRoutePlatform", "07-Kakon2",
        "08-Phase3Transition", "09-FinalRoute", "10-FinalCling", "11-Kakon3",
        "12-Calm", "13-Victory", "14-Fall", "15-Recovery", "16-Retry",
    )

    assert "RecoveryRuns>=2" in compact(driver)
    for capture_name in capture_names:
        assert capture_name in driver
        assert capture_name in prototype_tool


def test_zero_duration_transitions_are_guarded() -> None:
    source = compact(read("Source/IshibashiriPrototype/Private/MinedakiBoss.cpp"))

    assert "RouteTransitionSeconds<=0" in source
    assert "CalmSeconds<=0" in source
