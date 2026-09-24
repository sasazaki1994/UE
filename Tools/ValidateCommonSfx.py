"""Validate a complete Common SFX delivery using only the Python standard library."""
from __future__ import annotations

import json
from pathlib import Path
import struct
import sys
import wave

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "Audio" / "CommonSfx"
NAMES = ("BoundaryReading", "ArmWarning", "MountOpen", "ClingWarning",
         "KakonPurified", "EncounterCalmed")


def inspect_wav(path: Path) -> dict[str, float | int]:
    with wave.open(str(path), "rb") as stream:
        channels, width, rate, frames = (stream.getnchannels(), stream.getsampwidth(),
                                         stream.getframerate(), stream.getnframes())
        payload = stream.readframes(frames)
    if (channels, width, rate) != (1, 2, 48000):
        raise ValueError(f"{path.name}: expected 48 kHz/16-bit/mono")
    if not payload or frames == 0:
        raise ValueError(f"{path.name}: empty")
    samples = struct.unpack(f"<{len(payload) // 2}h", payload)
    peak = max(abs(sample) for sample in samples) / 32768
    rms = (sum(sample * sample for sample in samples) / len(samples)) ** 0.5 / 32768
    if rms < 0.001:
        raise ValueError(f"{path.name}: silent or nearly silent")
    if peak >= 1.0:
        raise ValueError(f"{path.name}: clipped")
    return {"duration_seconds": frames / rate, "peak": peak, "rms": rms}


def validate(base: Path = BASE) -> list[str]:
    manifest = json.loads((base / "manifest.json").read_text(encoding="utf-8"))
    events = manifest["events"]
    if [event["name"] for event in events] != list(NAMES):
        raise ValueError("manifest events are missing or out of specification order")
    if any(event["status"] != "adopted" for event in events):
        return ["BLOCKED: one or more roles are pending; no complete delivery claimed"]
    provenance = ("source_url", "author", "license", "license_url", "source_file", "edits")
    for event in events:
        if not all(event.get(field) for field in provenance) or event.get("redistribution_verified") is not True:
            raise ValueError(f"{event['name']}: incomplete provenance or redistribution check")
    if manifest.get("preview") != "CommonSfxPreview.wav":
        raise ValueError("manifest does not identify the required preview")
    reports = {name: inspect_wav(base / f"{name}.wav") for name in NAMES}
    reports["CommonSfxPreview"] = inspect_wav(base / "CommonSfxPreview.wav")
    levels = [reports[name]["rms"] for name in NAMES]
    if max(levels) / min(levels) > 8:
        raise ValueError("individual RMS levels differ by more than 18 dB")
    return [f"PASS: {name}: {reports[name]}" for name in (*NAMES, "CommonSfxPreview")]


if __name__ == "__main__":
    try:
        print(*validate(), sep="\n")
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
