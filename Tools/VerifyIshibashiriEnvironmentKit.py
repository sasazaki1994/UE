"""Validate generated Ishibashiri first-batch artifacts against manifest authority."""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST_PATH = ROOT / "Art/Environment/Ishibashiri/manifest.json"
GENERATED = MANIFEST_PATH.parent / "Generated"
ASSETS = (
    "SM_Ishibashiri_OldCedar_A", "SM_Ishibashiri_Rock_A", "SM_Ishibashiri_BoundaryStone_A",
)


def dimension_ranges(spec):
    dims = spec["dimensions"]
    if "size_cm" in dims:
        return dims["size_cm"]
    # Cedar X/Y are governed by crown width; Z by height. Trunk diameter is separately authored.
    return {"x": dims["crown_width_cm"], "y": dims["crown_width_cm"], "z": dims["height_cm"]}


def validate(root=GENERATED):
    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    specs = {item["asset_id"]: item for item in manifest["assets"]}
    errors, missing = [], []
    for asset in ASSETS:
        directory = root / asset.removeprefix("SM_Ishibashiri_")
        expected = [directory/(asset+suffix) for suffix in (".glb", ".fbx", "_report.json", "_preview.png")]
        absent = [str(p.relative_to(root.parent) if p.is_relative_to(root.parent) else p)
                  for p in expected if not p.is_file()]
        if absent:
            missing.extend(absent)
            continue
        try:
            report = json.loads(expected[2].read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            errors.append(f"{asset}: invalid report JSON: {exc}")
            continue
        spec = specs[asset]
        if report.get("asset_id") != asset: errors.append(f"{asset}: asset_id mismatch")
        for axis, limits in dimension_ranges(spec).items():
            value = report.get("dimensions_cm", {}).get(axis)
            if value is None or not limits[0] <= value <= limits[1]:
                errors.append(f"{asset}: {axis} dimension {value} outside {limits}")
        lod0 = spec["lods"][0]
        tris = report.get("triangle_count")
        if not isinstance(tris, int) or not lod0["triangle_target_min"] <= tris <= lod0["triangle_target_max"]:
            if not report.get("triangle_budget_exception_reason"):
                errors.append(f"{asset}: triangle count {tris} outside LOD0 budget without reason")
        if not report.get("pivot"): errors.append(f"{asset}: pivot policy missing")
        if not str(report.get("collision_status", "")).startswith("GENERATED"):
            errors.append(f"{asset}: collision not generated")
        max_materials = 2 if asset.endswith("OldCedar_A") else 1
        if report.get("material_count", 999) > max_materials: errors.append(f"{asset}: material slot limit exceeded")
        if report.get("ue_import_status") != "NOT_RUN" or report.get("ue_visual_review_status") != "NOT_RUN":
            errors.append(f"{asset}: UE review must remain NOT_RUN")
    if missing:
        return "NOT_GENERATED", ["Missing generated artifact: " + p for p in missing]
    return ("FAIL", errors) if errors else ("PASS", [])


def main():
    status, messages = validate()
    print(status)
    for message in messages: print("-", message)
    return 0 if status == "PASS" else (2 if status == "NOT_GENERATED" else 1)


if __name__ == "__main__":
    sys.exit(main())
