"""Blender 3.6 round-trip and preview validation for the Ishibashiri first batch."""
import json
import math
import os
import sys
from pathlib import Path

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "Art/Environment/Ishibashiri/Generated"
ASSETS = (
    "SM_Ishibashiri_OldCedar_A",
    "SM_Ishibashiri_Rock_A",
    "SM_Ishibashiri_BoundaryStone_A",
)
TOLERANCE = 0.03


def fail(message):
    raise RuntimeError(message)


def fresh_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def mesh_metrics(objects):
    vertices = triangles = 0
    points = []
    for obj in objects:
        if obj.type != "MESH" or not obj.data.vertices:
            continue
        vertices += len(obj.data.vertices)
        obj.data.calc_loop_triangles()
        triangles += len(obj.data.loop_triangles)
        for vertex in obj.data.vertices:
            world = obj.matrix_world @ vertex.co
            if not all(math.isfinite(value) for value in world):
                fail(f"{obj.name}: NaN or Inf vertex")
            points.append(world)
        if not all(math.isfinite(value) and 1e-6 < abs(value) < 1e6 for value in obj.scale):
            fail(f"{obj.name}: abnormal scale {tuple(obj.scale)}")
    if vertices <= 0 or triangles <= 0 or not points:
        fail("empty geometry")
    dimensions = [max(p[i] for p in points) - min(p[i] for p in points) for i in range(3)]
    return vertices, triangles, [round(value * 100, 3) for value in dimensions]


def import_and_validate(asset, suffix, report):
    fresh_scene()
    path = OUTPUT / asset.removeprefix("SM_Ishibashiri_") / (asset + suffix)
    if suffix == ".fbx":
        bpy.ops.import_scene.fbx(filepath=str(path))
    else:
        bpy.ops.import_scene.gltf(filepath=str(path))
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    collision_name = "UCX_" + asset + "_00"
    collisions = [obj for obj in meshes if obj.name == collision_name]
    if len(collisions) != 1:
        fail(f"{asset} {suffix}: expected collision {collision_name}, got {[o.name for o in meshes]}")
    visual = [obj for obj in meshes if not obj.name.startswith("UCX_")]
    vertices, triangles, dimensions = mesh_metrics(visual)
    collision_vertices, collision_triangles, _ = mesh_metrics(collisions)
    expected = list(report["dimensions_cm"].values())
    deviations = [abs(actual-wanted)/max(wanted, 1e-9) for actual, wanted in zip(dimensions, expected)]
    if any(value > TOLERANCE for value in deviations):
        fail(f"{asset} {suffix}: dimensions {dimensions} differ from {expected}; deviations={deviations}")
    return {
        "status": "PASS", "format": suffix[1:].upper(), "file": path.name,
        "mesh_count": len(visual), "vertices": vertices, "triangles": triangles,
        "dimensions_cm": dict(zip(("x", "y", "z"), dimensions)),
        "dimension_tolerance_fraction": TOLERANCE,
        "collision": {"status": "PASS", "name": collision_name,
                      "vertices": collision_vertices, "triangles": collision_triangles},
        "finite_geometry": True, "scale_check": "PASS", "empty_geometry": False,
    }


def validate_png(path, minimum=(640, 480)):
    if not path.is_file() or path.stat().st_size <= 0:
        fail(f"missing or empty PNG: {path}")
    image = bpy.data.images.load(str(path), check_existing=False)
    width, height = image.size
    if width < minimum[0] or height < minimum[1]:
        fail(f"PNG too small: {path} ({width}x{height})")
    pixels = list(image.pixels)
    if not pixels or max(pixels) - min(pixels) < 0.01:
        fail(f"PNG appears single-colour: {path}")
    bpy.data.images.remove(image)
    return {"status": "PASS", "width": width, "height": height, "bytes": path.stat().st_size,
            "not_single_colour": True}


def main():
    results = {}
    for asset in ASSETS:
        directory = OUTPUT / asset.removeprefix("SM_Ishibashiri_")
        report = json.loads((directory / (asset + "_report.json")).read_text(encoding="utf-8"))
        formats = {name: import_and_validate(asset, "." + name, report) for name in ("fbx", "glb")}
        preview_result = validate_png(directory / (asset + "_preview.png"))
        roundtrip = {"asset_id": asset, "blender_version": bpy.app.version_string,
                     "status": "PASS", "formats": formats, "preview": preview_result}
        (directory / (asset + "_roundtrip.json")).write_text(
            json.dumps(roundtrip, indent=2) + "\n", encoding="utf-8")
        results[asset] = "PASS"
    contact = validate_png(OUTPUT / "IshibashiriEnvironment_FirstBatch_ContactSheet.png", (1200, 600))
    summary = {
        "source_commit": os.environ.get("SOURCE_COMMIT", "UNKNOWN"),
        "blender_version": bpy.app.version_string,
        "generation": "PASS", "verification": "PASS", "roundtrip": "PASS",
        "assets": results, "preview": "PASS", "contact_sheet": contact["status"],
        "ue_import": "NOT_RUN", "ue_material": "NOT_RUN", "ue_collision": "NOT_RUN",
        "ue_runtime": "NOT_RUN", "visual_approval": "NOT_RUN",
        "candidate_status": "BLENDER PRODUCTION CANDIDATE",
    }
    (OUTPUT / "validation-summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print("FAIL:", exc, file=sys.stderr)
        raise
