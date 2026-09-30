"""Reproduce the six common cues from the credited, externally downloaded recordings.

Requires numpy, scipy, soundfile. Source recordings remain outside the repository.
Usage: python Tools/PrepareCommonSfx.py --source-root <download/extract directory>
The manifest records exact inputs, cuts, processing and SHA-256 values.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil

import numpy as np
import soundfile as sf
from scipy.signal import butter, resample_poly, sosfilt

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Audio/CommonSfx"
RATE = 48000
CC0 = "https://creativecommons.org/publicdomain/zero/1.0/"
SOURCES = {
    "bell": ("impact/Audio/impactBell_heavy_000.ogg", "Kenney", "https://kenney.nl/assets/impact-sounds"),
    "wood": ("impact/Audio/impactWood_heavy_000.ogg", "Kenney", "https://kenney.nl/assets/impact-sounds"),
    "stone": ("impact/Audio/impactMining_000.ogg", "Kenney", "https://kenney.nl/assets/impact-sounds"),
    "creak": ("rpg/Audio/creak1.ogg", "Kenney Vleugels (Kenney.nl)", "https://kenney.nl/assets/rpg-audio"),
    "water": ("splash-hq.mp3", "qubodup (edit); blaukreuz (original recording)", "https://freesound.org/people/qubodup/sounds/212143/"),
    "wind": ("wind-hq.mp3", "florianreichelt", "https://freesound.org/people/florianreichelt/sounds/459977/"),
}
# Every layer is an edit of a licensed recording, never a synthesized tone/noise.
# cut in/out seconds refer to the downloaded source before speed adjustment.
RECIPES = [
    ("BoundaryReading", "A short, clear bell response; pulse interval conveys proximity.", .07,
     [dict(source="bell", cut=[.00, .56], speed=1.25, gain_db=-2, highpass_hz=220, lowpass_hz=6200, fade_in=.006, fade_out=.12, at=0)]),
    ("ArmWarning", "Low wooden warning knock with a subdued ringing tail.", .085,
     [dict(source="wood", cut=[.00, .313], speed=.72, gain_db=0, highpass_hz=90, lowpass_hz=2200, fade_in=.004, fade_out=.10, at=0),
      dict(source="bell", cut=[.00, .48], speed=.60, gain_db=-11, highpass_hz=100, lowpass_hz=1800, fade_in=.006, fade_out=.18, at=.035)]),
    ("MountOpen", "Rock strike and a low wood impact mark the short mounting opportunity.", .10,
     [dict(source="stone", cut=[.00, .72], speed=.85, gain_db=0, highpass_hz=100, lowpass_hz=4500, fade_in=.003, fade_out=.17, at=0),
      dict(source="wood", cut=[.00, .313], speed=.68, gain_db=-5, highpass_hz=70, lowpass_hz=2500, fade_in=.003, fade_out=.10, at=0)]),
    ("ClingWarning", "A creak and two restrained bell taps precede shaking.", .09,
     [dict(source="creak", cut=[.00, .66], speed=.8, gain_db=0, highpass_hz=160, lowpass_hz=4000, fade_in=.02, fade_out=.16, at=0),
      dict(source="bell", cut=[.00, .35], speed=1.15, gain_db=-12, highpass_hz=300, lowpass_hz=5000, fade_in=.006, fade_out=.09, at=.05),
      dict(source="bell", cut=[.00, .35], speed=1.15, gain_db=-14, highpass_hz=300, lowpass_hz=5000, fade_in=.006, fade_out=.09, at=.39)]),
    ("KakonPurified", "A stone crack opens into a brief water release.", .10,
     [dict(source="stone", cut=[.00, .45], speed=1.1, gain_db=-4, highpass_hz=120, lowpass_hz=5500, fade_in=.003, fade_out=.10, at=0),
      dict(source="water", cut=[.00, 1.48], speed=1.0, gain_db=0, highpass_hz=170, lowpass_hz=6200, fade_in=.02, fade_out=.38, at=.06)]),
    ("EncounterCalmed", "Quiet wind and receding water make a short calming tail before the next card.", .065,
     [dict(source="wind", cut=[8.0, 9.85], speed=1.0, gain_db=0, highpass_hz=170, lowpass_hz=3800, fade_in=.28, fade_out=.55, at=0),
      dict(source="water", cut=[.75, 2.0], speed=1.0, gain_db=-9, highpass_hz=220, lowpass_hz=4200, fade_in=.16, fade_out=.48, at=.12)]),
]


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def layer_audio(base: Path, layer: dict) -> tuple[np.ndarray, dict]:
    filename, author, url = SOURCES[layer["source"]]
    path = base / filename
    audio, rate = sf.read(path, dtype="float64", always_2d=True)
    first, last = [int(t * rate) for t in layer["cut"]]
    audio = audio[first:min(last, len(audio))].mean(axis=1)
    if len(audio) < 10:
        raise ValueError(f"Empty source cut: {filename}")
    audio = resample_poly(audio, RATE, int(round(rate * layer["speed"])))
    for key, mode in (("highpass_hz", "highpass"), ("lowpass_hz", "lowpass")):
        audio = sosfilt(butter(2, layer[key], btype=mode, fs=RATE, output="sos"), audio)
    for fade_key, reverse in (("fade_in", False), ("fade_out", True)):
        n = min(len(audio), int(layer[fade_key] * RATE))
        envelope = np.linspace(0, 1, n)
        if reverse:
            audio[-n:] *= envelope[::-1]
        else:
            audio[:n] *= envelope
    audio *= 10 ** (layer["gain_db"] / 20)
    info = dict(layer, source_file=filename, author=author, source_url=url,
                source_sha256=digest(path), license="CC0-1.0", license_url=CC0,
                redistribution_verified=True)
    if layer["source"] == "water":
        info.update(download_url="https://cdn.freesound.org/previews/212/212143_71257-hq.mp3",
                    source_format_note="Public HQ MP3 preview, not the original FLAC.",
                    original_source_url="https://freesound.org/people/blaukreuz/sounds/195877/",
                    original_license="CC0-1.0")
    if layer["source"] == "wind":
        info.update(download_url="https://cdn.freesound.org/previews/459/459977_6253486-hq.mp3",
                    source_format_note="Public HQ MP3 preview of the original MP3.")
    return audio, info


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", type=Path, required=True)
    args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    events = []
    preview = []
    cursor = 0
    preview_timeline = []
    for name, intent, target_rms, recipe in RECIPES:
        layers = [layer_audio(args.source_root, item) for item in recipe]
        length = max(int(item["at"] * RATE) + len(audio) for (audio, _), item in zip(layers, recipe))
        output = np.zeros(length)
        for (audio, _), item in zip(layers, recipe):
            start = int(item["at"] * RATE)
            output[start:start + len(audio)] += audio
        rms = np.sqrt(np.mean(output ** 2))
        gain = min(target_rms / max(rms, 1e-9), .63 / max(np.max(np.abs(output)), 1e-9))
        output *= gain
        path = OUT / f"{name}.wav"
        sf.write(path, output, RATE, subtype="PCM_16")
        sources = [info for _, info in layers]
        events.append(dict(name=name, status="adopted", intent=intent,
                           source_url=sources[0]["source_url"], author="; ".join(dict.fromkeys(s["author"] for s in sources)),
                           license="CC0-1.0", license_url=CC0, redistribution_verified=True,
                           source_file="; ".join(dict.fromkeys(s["source_file"] for s in sources)),
                           edits=dict(layers=sources, downmix="arithmetic stereo mean", resampling="scipy resample_poly",
                                      filter="2nd-order Butterworth high/low-pass", final_gain_db=round(float(20 * np.log10(gain)), 5),
                                      peak_limit_linear=.63, output_format="48000 Hz, PCM16, mono"),
                           sha256=digest(path), duration_seconds=len(output) / RATE))
        preview.append(np.zeros(int(.65 * RATE)))
        cursor += int(.65 * RATE)
        preview_timeline.append(dict(name=name, start_seconds=cursor / RATE, duration_seconds=len(output) / RATE))
        # Build preview from the actual quantized deliverable, not a float draft.
        quantized, _ = sf.read(path)
        preview.append(quantized)
        cursor += len(quantized)
    preview.append(np.zeros(int(.65 * RATE)))
    sf.write(OUT / "CommonSfxPreview.wav", np.concatenate(preview), RATE, subtype="PCM_16")
    for pack in ("impact", "rpg"):
        shutil.copyfile(args.source_root / pack / "License.txt", OUT / f"Kenney-{pack}-License.txt")
    manifest = dict(schema_version=2, delivery_status="implemented_pending_listening", verified_date="2026-09-29",
                    listening_review="not_performed", ue_import_review="not_performed", game_playback_review="not_performed",
                    review_note="Source pages/licenses and PCM data verified. No human headphone/speaker listening has occurred; automated checks do not establish subjective audio quality.",
                    format=dict(sample_rate_hz=RATE, sample_width_bits=16, channels=1),
                    preview="CommonSfxPreview.wav", preview_timeline=preview_timeline,
                    preview_sha256=digest(OUT / "CommonSfxPreview.wav"), events=events)
    (OUT / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Prepared {len(events)} licensed cues + preview in {OUT}")


if __name__ == "__main__":
    main()
