import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import wave

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "Tools"))
import ValidateCommonSfx as V


class CommonSfxValidation(unittest.TestCase):
    def test_repository_truthfully_reports_incomplete_delivery(self):
        self.assertEqual(V.validate(), ["BLOCKED: one or more roles are pending; no complete delivery claimed"])

    def test_complete_delivery_is_decoded_and_checked(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            manifest = json.loads((V.BASE / "manifest.json").read_text(encoding="utf-8"))
            for event in manifest["events"]:
                event["status"] = "adopted"
                event.update(source_url="https://example.test/sound", author="fixture",
                             license="CC0-1.0", license_url="https://creativecommons.org/publicdomain/zero/1.0/",
                             source_file="fixture.wav", edits="unit-test fixture",
                             redistribution_verified=True)
            manifest["preview"] = "CommonSfxPreview.wav"
            (base / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            for name in (*V.NAMES, "CommonSfxPreview"):
                with wave.open(str(base / f"{name}.wav"), "wb") as output:
                    output.setparams((1, 2, 48000, 4800, "NONE", "not compressed"))
                    output.writeframes(b"".join(struct.pack("<h", 4000 if index % 2 else -4000)
                                                for index in range(4800)))
            self.assertEqual(len(V.validate(base)), 7)

    def test_clipping_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "clipped.wav"
            with wave.open(str(path), "wb") as output:
                output.setparams((1, 2, 48000, 2, "NONE", "not compressed"))
                output.writeframes(struct.pack("<hh", -32768, 32767))
            with self.assertRaisesRegex(ValueError, "clipped"):
                V.inspect_wav(path)


if __name__ == "__main__":
    unittest.main()
