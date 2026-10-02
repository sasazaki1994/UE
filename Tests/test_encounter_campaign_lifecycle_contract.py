"""Static contracts for the shared Campaign boundary of all four encounters."""

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PRIVATE = ROOT / "Source/IshibashiriPrototype/Private"

ENCOUNTERS = {
    "Prototype": ("APrototypeGameMode", "Ishibashiri", "HandleEncounterCompleted"),
    "Fuchimatoi": ("AFuchimatoiGameMode", "Fuchimatoi", "HandleCompleted"),
    "Minedaki": ("AMinedakiGameMode", "Minedaki", "HandleCompleted"),
    "Magatsune": ("AMagatsuneGameMode", "Magatsune", "HandleCompleted"),
}


def function_body(source: str, class_name: str, function_name: str) -> str:
    """Return one C++ member body, balancing braces rather than crossing functions."""
    signature = re.search(
        rf"\b{re.escape(class_name)}::{re.escape(function_name)}\s*\([^)]*\)\s*(?:const\s*)?\{{",
        source,
    )
    assert signature, f"missing {class_name}::{function_name}"
    opening = source.find("{", signature.start())
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[opening + 1 : index]
    raise AssertionError(f"unclosed body for {class_name}::{function_name}")


def compact(source: str) -> str:
    return re.sub(r"\s+", "", source)


def inline_function_body(source: str, function_name: str) -> str:
    signature = re.search(rf"\b{re.escape(function_name)}\s*\([^)]*\)\s*const\s*\{{", source)
    assert signature, f"missing inline {function_name}"
    opening = source.find("{", signature.start())
    closing = source.find("}", opening)
    assert closing >= 0, f"unclosed inline {function_name}"
    return source[opening + 1 : closing]


def sources():
    for mode, metadata in ENCOUNTERS.items():
        yield mode, metadata, (PRIVATE / f"{mode}GameMode.cpp").read_text(encoding="utf-8")


def test_all_encounters_bind_their_completion_handler():
    for mode, (class_name, _, handler_name), source in sources():
        start_play = compact(function_body(source, class_name, "StartPlay"))
        expected = compact(f"OnEncounterCompleted.AddUniqueDynamic(this, &{class_name}::{handler_name})")
        assert expected in start_play, mode


def test_retry_cancels_pending_travel_without_advancing_campaign():
    for mode, (class_name, encounter, _), source in sources():
        retry = compact(function_body(source, class_name, "RetryEncounter"))
        assert "ClearTimer(CampaignAdvanceTimer)" in retry, mode
        assert f"NotifyEncounterRetry(ECampaignState::{encounter})" in retry, mode
        assert "ResetEncounter()" in retry and "StartEncounter()" in retry, mode
        assert "CompleteEncounter(" not in retry and "TravelToCurrentChapter(" not in retry, mode


def test_completion_only_schedules_the_matching_campaign_after_victory_delay():
    for mode, (class_name, encounter, handler_name), source in sources():
        completed = compact(function_body(source, class_name, handler_name))
        assert f"IsCurrentEncounter(ECampaignState::{encounter})" in completed, mode
        assert "SetTimer(CampaignAdvanceTimer" in completed, mode
        assert ",2.f,false)" in completed, mode
        assert "CompleteEncounter(" not in completed, mode
        assert "TravelToCurrentChapter(" not in completed, mode


def test_campaign_travel_is_guarded_by_successful_matching_completion():
    for mode, (class_name, encounter, _), source in sources():
        advance = compact(function_body(source, class_name, "AdvanceCampaign"))
        completion = f"CompleteEncounter(ECampaignState::{encounter})"
        assert completion in advance, mode
        assert "TravelToCurrentChapter(this)" in advance, mode
        assert re.search(
            rf"if\(.*&&\w+->{re.escape(completion)}\)(?:\{{)?"
            r"\w+->TravelToCurrentChapter\(this\);",
            advance,
        ), f"{mode}: travel must remain inside the successful CompleteEncounter branch"


def test_standalone_encounters_cannot_satisfy_the_completion_guard():
    header = (ROOT / "Source/IshibashiriPrototype/Public/CampaignGameInstance.h").read_text(encoding="utf-8")
    current_encounter = compact(inline_function_body(header, "IsCurrentEncounter"))
    assert "returnbCampaignActive&&State==Encounter;" in current_encounter
