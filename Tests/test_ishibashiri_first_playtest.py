import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).parents[1]
sys.path.insert(0, str(ROOT / "Tools"))
import ValidateIshibashiriFirstPlaytest as V


def repository_record() -> dict:
    return json.loads(V.DEFAULT_RECORD.read_text(encoding="utf-8"))


def completed_record(status: str = "PASS") -> dict:
    record = repository_record()
    record.update(
        status=status,
        sourceSha="a" * 40,
        package="Packaged/IshibashiriPrototype.exe",
        packageSha256="b" * 64,
        viewport="1920x1080",
        qualityMode="Legacy",
        startedAt="2026-09-27T10:00:00+00:00",
        endedAt="2026-09-27T10:15:00+00:00",
        evidenceLocation="private-evidence/session-01",
    )
    record["participant"]["wasInvolvedInDevelopment"] = False
    record["input"].update(kind="keyboard_mouse")
    record["consent"].update(notes=True, screen=False, audio=False)
    record["results"].update(
        completedWithoutCoaching=True,
        returnedToTitleAfterTwoEndingCards=True,
        understoodCalmRatherThanDeath=True,
    )
    for beat in record["beats"]:
        beat.update(
            observed=True,
            firstUnderstoodAt="2026-09-27T10:05:00+00:00",
            observation="Observed without intervention.",
        )
    return record


def validate_record(record: dict) -> list[str]:
    with tempfile.TemporaryDirectory() as temporary:
        path = Path(temporary) / "record.json"
        path.write_text(json.dumps(record), encoding="utf-8")
        return V.validate(path)


class IshibashiriFirstPlaytestValidation(unittest.TestCase):
    def test_repository_record_truthfully_remains_not_run(self):
        self.assertEqual(
            V.validate(),
            [
                "human first playtest: NOT_RUN",
                "automated E2E and Capture do not satisfy this check",
            ],
        )

    def test_complete_uncoached_record_can_pass(self):
        report = validate_record(completed_record())
        self.assertIn("human first playtest: PASS", report)
        self.assertIn(f"package SHA-256: {'b' * 64}", report)
        self.assertIn("input: keyboard_mouse", report)

    def test_pass_rejects_coaching(self):
        record = completed_record()
        record["beats"][3]["coaching"] = True
        with self.assertRaisesRegex(ValueError, "PASS cannot include coaching"):
            validate_record(record)

    def test_physical_gamepad_requires_its_model(self):
        record = completed_record()
        record["input"]["kind"] = "physical_gamepad"
        with self.assertRaisesRegex(ValueError, "input.deviceModel is required"):
            validate_record(record)

    def test_fail_requires_participant_reported_blocker(self):
        record = completed_record("FAIL")
        with self.assertRaisesRegex(ValueError, "participant-reported blocker"):
            validate_record(record)

    def test_not_run_rejects_claimed_session_evidence(self):
        record = copy.deepcopy(repository_record())
        record["sourceSha"] = "a" * 40
        with self.assertRaisesRegex(ValueError, "must not contain session evidence"):
            validate_record(record)

    def test_completed_record_requires_package_digest(self):
        record = completed_record()
        record["packageSha256"] = "not-a-digest"
        with self.assertRaisesRegex(ValueError, "64-character SHA-256"):
            validate_record(record)

    def test_understanding_time_must_be_inside_session(self):
        record = completed_record()
        record["beats"][0]["firstUnderstoodAt"] = "2026-09-27T09:59:59+00:00"
        with self.assertRaisesRegex(ValueError, "must be within the session"):
            validate_record(record)

    def test_unobserved_beat_cannot_claim_understanding_time(self):
        record = completed_record("FAIL")
        record["participantReportedBlockers"] = ["Could not identify the counter window."]
        record["beats"][3]["observed"] = False
        with self.assertRaisesRegex(ValueError, "cannot have firstUnderstoodAt"):
            validate_record(record)


if __name__ == "__main__":
    unittest.main()
