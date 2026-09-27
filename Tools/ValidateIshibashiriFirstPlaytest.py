"""Validate Ishibashiri first-play evidence without manufacturing human results."""
from __future__ import annotations

from datetime import datetime
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_RECORD = ROOT / "Docs" / "IshibashiriFirstPlaytestValidation.json"
BEATS = (
    "Title / Prologue",
    "Approach / boundary",
    "Charge / dodge",
    "Recover / counter",
    "Kneel / Grab",
    "Route / Cling / rest",
    "Kakon 1 / 2 / 3",
    "Calm / ending / Title",
)


def _required_text(value: object, field: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise ValueError(f"{field} is required")
    return value


def _timestamp(value: object, field: str) -> datetime:
    text = _required_text(value, field)
    try:
        parsed = datetime.fromisoformat(text.replace("Z", "+00:00"))
    except ValueError as error:
        raise ValueError(f"{field} must be an ISO-8601 timestamp") from error
    if parsed.tzinfo is None:
        raise ValueError(f"{field} must include a timezone")
    return parsed


def validate(path: Path = DEFAULT_RECORD) -> list[str]:
    record = json.loads(path.read_text(encoding="utf-8"))
    if record.get("schemaVersion") != 2:
        raise ValueError("schemaVersion must be 2")
    status = record.get("status")
    if status not in {"NOT_RUN", "PASS", "FAIL"}:
        raise ValueError("status must be NOT_RUN, PASS, or FAIL")
    beats = record.get("beats")
    if not isinstance(beats, list) or [beat.get("name") for beat in beats] != list(BEATS):
        raise ValueError("beats must contain every specified beat in order")
    if status == "NOT_RUN":
        session_fields = ("sourceSha", "package", "packageSha256", "startedAt", "endedAt")
        if any(record.get(field) is not None for field in session_fields):
            raise ValueError("NOT_RUN must not contain session evidence")
        if any(beat.get("observed") for beat in record.get("beats", [])):
            raise ValueError("NOT_RUN must not claim observed beats")
        return ["human first playtest: NOT_RUN", "automated E2E and Capture do not satisfy this check"]

    source_sha = _required_text(record.get("sourceSha"), "sourceSha")
    if not re.fullmatch(r"[0-9a-fA-F]{40}", source_sha):
        raise ValueError("sourceSha must be a full 40-character Git SHA")
    _required_text(record.get("package"), "package")
    package_sha = _required_text(record.get("packageSha256"), "packageSha256")
    if not re.fullmatch(r"[0-9a-fA-F]{64}", package_sha):
        raise ValueError("packageSha256 must be a 64-character SHA-256 digest")
    _required_text(record.get("viewport"), "viewport")
    _required_text(record.get("qualityMode"), "qualityMode")
    if record.get("participant", {}).get("wasInvolvedInDevelopment") is not False:
        raise ValueError("participant must not have been involved in development")
    input_record = record.get("input", {})
    if input_record.get("kind") not in {"keyboard_mouse", "physical_gamepad"}:
        raise ValueError("input.kind must be keyboard_mouse or physical_gamepad")
    if input_record.get("kind") == "physical_gamepad":
        _required_text(input_record.get("deviceModel"), "input.deviceModel")
    consent = record.get("consent", {})
    if consent.get("notes") is not True:
        raise ValueError("consent.notes must be true")
    if not all(isinstance(consent.get(field), bool) for field in ("screen", "audio")):
        raise ValueError("consent.screen and consent.audio must be boolean")
    started = _timestamp(record.get("startedAt"), "startedAt")
    ended = _timestamp(record.get("endedAt"), "endedAt")
    if ended <= started:
        raise ValueError("endedAt must be later than startedAt")

    for beat in beats:
        if not isinstance(beat.get("observed"), bool):
            raise ValueError(f"{beat['name']}: observed must be boolean")
        if not isinstance(beat.get("failures"), int) or isinstance(beat.get("failures"), bool) or beat["failures"] < 0:
            raise ValueError(f"{beat['name']}: failures must be a non-negative integer")
        if not isinstance(beat.get("coaching"), bool):
            raise ValueError(f"{beat['name']}: coaching must be boolean")
        _required_text(beat.get("observation"), f"{beat['name']}.observation")
        understood_at = beat.get("firstUnderstoodAt")
        if beat["observed"]:
            understood = _timestamp(understood_at, f"{beat['name']}.firstUnderstoodAt")
            if not started <= understood <= ended:
                raise ValueError(f"{beat['name']}: firstUnderstoodAt must be within the session")
        elif understood_at is not None:
            raise ValueError(f"{beat['name']}: an unobserved beat cannot have firstUnderstoodAt")
    _required_text(record.get("evidenceLocation"), "evidenceLocation")

    if status == "PASS":
        results = record.get("results", {})
        required_results = (
            "completedWithoutCoaching",
            "returnedToTitleAfterTwoEndingCards",
            "understoodCalmRatherThanDeath",
        )
        if any(results.get(field) is not True for field in required_results):
            raise ValueError("PASS requires completion, Title return, and calm understanding")
        if any(not beat["observed"] for beat in beats):
            raise ValueError("PASS requires every beat to be observed")
        if any(beat["coaching"] for beat in beats):
            raise ValueError("PASS cannot include coaching")
    elif not record.get("participantReportedBlockers"):
        raise ValueError("FAIL requires at least one participant-reported blocker")
    return [
        f"human first playtest: {status}",
        f"source SHA: {source_sha}",
        f"package SHA-256: {package_sha.lower()}",
        f"input: {input_record['kind']}",
    ]


if __name__ == "__main__":
    try:
        target = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_RECORD
        print(*validate(target), sep="\n")
    except (OSError, ValueError, KeyError, TypeError, json.JSONDecodeError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
