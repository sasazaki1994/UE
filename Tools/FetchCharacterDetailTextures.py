"""Download the CC0 Poly Haven microsurface sources layered over the rigged character atlases.

Run: python Tools/FetchCharacterDetailTextures.py
Downloads share the ignored Saved/ModelDownloads/PolyHaven cache. The provenance record with
URLs, authors and SHA256 is written to Art/Characters/Detail/sources.json.
"""
import json
import sys
from pathlib import Path

import FetchPolyHavenEnvironment as polyhaven

ROOT = Path(__file__).resolve().parents[1]
RECORD = ROOT / "Art/Characters/Detail/sources.json"
TEXTURES = {
    "rock_face": "Ishibashiri granite",
    "lichen_rock": "Ishibashiri dry lichen",
    "mossy_rock": "Ishibashiri moss cushion",
    "japanese_cedar_bark": "Ishibashiri blighted root",
    "thatch_roof_angled": "Ishibashiri weathered straw, Shirotsura rice straw",
    "rough_linen": "Ishibashiri paper offerings, Shirotsura unbleached linen",
    "denim_fabric": "Shirotsura indigo work cloth and repairs",
    "fine_grained_wood": "Shirotsura aged whitewood mask",
}


def main():
    sources = [{**polyhaven.fetch_texture(asset_id, target, "1k"), **polyhaven.info(asset_id)}
               for asset_id, target in TEXTURES.items()]
    RECORD.parent.mkdir(parents=True, exist_ok=True)
    RECORD.write_text(json.dumps({"provider": "Poly Haven", "license": "CC0 1.0", "license_url": polyhaven.LICENSE_URL,
                                  "cache": polyhaven.CACHE.relative_to(ROOT).as_posix(), "sources": sources},
                                 indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print("CHARACTER_DETAIL_FETCH_PASS", len(sources), "sources ->", RECORD.relative_to(ROOT))


if __name__ == "__main__":
    sys.exit(main())
