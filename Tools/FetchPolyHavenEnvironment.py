"""Download the CC0 Poly Haven sources used to replace Ishibashiri environment primitives.

Run: python Tools/FetchPolyHavenEnvironment.py
Downloads go to Saved/ModelDownloads/PolyHaven (ignored by git). The provenance record with
URLs, authors and SHA256 is written to Art/Environment/Ishibashiri/PolyHaven/sources.json.
"""
import hashlib
import json
import sys
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CACHE = ROOT / "Saved/ModelDownloads/PolyHaven"
RECORD = ROOT / "Art/Environment/Ishibashiri/PolyHaven/sources.json"
API = "https://api.polyhaven.com"
LICENSE_URL = "https://polyhaven.com/license"
MODELS = {
    "fir_tree_01": "SM_Ishibashiri_OldCedar_A, SM_Ishibashiri_OldCedar_B",
    "rock_moss_set_01": "SM_Ishibashiri_Rock_A, SM_Ishibashiri_Rock_B",
    "dead_tree_trunk": "SM_Ishibashiri_FallenCedar_A",
    "fern_02": "SM_Ishibashiri_Fern_A",
}
TEXTURES = {
    "forest_ground_04": ("M_Ishibashiri_Ground", "2k"),
    "mossy_rock": ("M_Ishibashiri_BoundaryStone", "1k"),
    "rough_wood": ("M_Ishibashiri_RitualWood", "1k"),
    "thatch_roof_angled": ("M_Ishibashiri_RopeStraw", "1k"),
    "rough_linen": ("M_Ishibashiri_RopePaper", "1k"),
}
TEXTURE_MAPS = {"Diffuse": "diff", "nor_gl": "nor_gl", "arm": "arm"}
# The 1k glTF omits foliage cutouts, the rock's packed ARM map and the third tree's trunk maps.
EXTRA_MODEL_MAPS = {
    "fir_tree_01": ("twig_alpha", "trunk_c_diff", "trunk_c_nor_gl", "trunk_c_arm"),
    "fern_02": ("Alpha",),
    "rock_moss_set_01": ("arm",),
}


def get_json(url):
    request = urllib.request.Request(url, headers={"User-Agent": "IshibashiriPrototype-AssetFetch"})
    with urllib.request.urlopen(request, timeout=60) as response:
        return json.load(response)


def download(url, path, expected_size=None):
    if path.is_file() and (expected_size is None or path.stat().st_size == expected_size):
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    request = urllib.request.Request(url, headers={"User-Agent": "IshibashiriPrototype-AssetFetch"})
    partial = path.with_suffix(path.suffix + ".part")
    with urllib.request.urlopen(request, timeout=120) as response, partial.open("wb") as output:
        while chunk := response.read(1 << 20):
            output.write(chunk)
    partial.replace(path)
    if expected_size is not None and path.stat().st_size != expected_size:
        raise RuntimeError("Size mismatch for %s" % url)


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while chunk := stream.read(1 << 20):
            digest.update(chunk)
    return digest.hexdigest()


def info(asset_id):
    data = get_json("%s/info/%s" % (API, asset_id))
    return {"name": data["name"], "authors": data["authors"], "page": "https://polyhaven.com/a/" + asset_id}


def fetch_model(asset_id, target):
    catalogue = get_json("%s/files/%s" % (API, asset_id))
    files = catalogue["gltf"]["1k"]["gltf"]
    directory = CACHE / asset_id
    entries = [(files["url"], directory / Path(files["url"]).name, files["size"])]
    entries += [(item["url"], directory / name, item["size"]) for name, item in files["include"].items()]
    for key in EXTRA_MODEL_MAPS.get(asset_id, ()):
        formats = catalogue[key]["1k"]
        item = formats.get("png") or formats["jpg"]
        entries.append((item["url"], directory / "textures" / Path(item["url"]).name, item["size"]))
    record = {"asset_id": asset_id, "type": "model", "target": target, "resolution": "1k", "format": "gltf", "files": []}
    for url, path, size in entries:
        print("download", path.relative_to(CACHE), flush=True)
        download(url, path, size)
        record["files"].append({"url": url, "path": path.relative_to(ROOT).as_posix(), "size": size, "sha256": sha256(path)})
    return record


def fetch_texture(asset_id, target, resolution):
    files = get_json("%s/files/%s" % (API, asset_id))
    record = {"asset_id": asset_id, "type": "texture", "target": target, "resolution": resolution, "files": []}
    for key, suffix in TEXTURE_MAPS.items():
        item = files[key][resolution]["jpg"]
        path = CACHE / asset_id / ("%s_%s_%s.jpg" % (asset_id, suffix, resolution))
        print("download", path.relative_to(CACHE), flush=True)
        download(item["url"], path, item["size"])
        record["files"].append({"map": suffix, "url": item["url"], "path": path.relative_to(ROOT).as_posix(),
                                "size": item["size"], "sha256": sha256(path)})
    return record


def main():
    sources = []
    for asset_id, target in MODELS.items():
        sources.append({**fetch_model(asset_id, target), **info(asset_id)})
    for asset_id, (target, resolution) in TEXTURES.items():
        sources.append({**fetch_texture(asset_id, target, resolution), **info(asset_id)})
    RECORD.parent.mkdir(parents=True, exist_ok=True)
    RECORD.write_text(json.dumps({"provider": "Poly Haven", "license": "CC0 1.0", "license_url": LICENSE_URL,
                                  "cache": CACHE.relative_to(ROOT).as_posix(), "sources": sources},
                                 indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print("POLYHAVEN_FETCH_PASS", len(sources), "sources ->", RECORD.relative_to(ROOT))


if __name__ == "__main__":
    sys.exit(main())
