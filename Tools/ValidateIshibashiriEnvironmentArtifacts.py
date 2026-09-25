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
PREVIEW_PREFIX = "PREVIEW_"


def fail(message):
    raise RuntimeError(message)


def fresh_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def aabb(points):
    lower = [min(point[i] for point in points) for i in range(3)]
    upper = [max(point[i] for point in points) for i in range(3)]
    return {"min": lower, "max": upper,
            "dimensions_cm": [round((upper[i] - lower[i]) * 100, 3) for i in range(3)]}


def object_diagnostic(obj):
    obj.data.calc_loop_triangles()
    local = [vertex.co.copy() for vertex in obj.data.vertices]
    world = [obj.matrix_world @ point for point in local]
    return {"name": obj.name, "location": list(obj.location),
            "rotation_euler": list(obj.rotation_euler), "scale": list(obj.scale),
            "vertex_count": len(local), "triangle_count": len(obj.data.loop_triangles),
            "local_aabb": aabb(local), "world_aabb": aabb(world)}


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
    bounds = aabb(points)
    return vertices, triangles, bounds["dimensions_cm"], bounds


def import_and_validate(asset, suffix, report):
    fresh_scene()
    path = OUTPUT / asset.removeprefix("SM_Ishibashiri_") / (asset + suffix)
    if suffix == ".fbx":
        bpy.ops.import_scene.fbx(filepath=str(path))
    else:
        bpy.ops.import_scene.gltf(filepath=str(path))
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    preview_objects = [obj.name for obj in bpy.context.scene.objects if obj.name.startswith(PREVIEW_PREFIX)]
    if preview_objects:
        fail(f"{asset} {suffix}: preview objects leaked into export: {preview_objects}")
    collision_name = "UCX_" + asset + "_00"
    collisions = [obj for obj in meshes if obj.name == collision_name]
    if len(collisions) != 1:
        fail(f"{asset} {suffix}: expected collision {collision_name}, got {[o.name for o in meshes]}")
    visual = [obj for obj in meshes if not obj.name.startswith("UCX_")]
    expected_names = set(report["visual_object_names"])
    actual_names = {obj.name for obj in visual}
    if actual_names != expected_names:
        fail(f"{asset} {suffix}: visual object mismatch expected={sorted(expected_names)} actual={sorted(actual_names)}")
    vertices, triangles, dimensions, world_bounds = mesh_metrics(visual)
    collision_vertices, collision_triangles, collision_dimensions, collision_bounds = mesh_metrics(collisions)
    # Importers may split vertices at normals/UV seams; triangle loss is not valid.
    if triangles != report["triangle_count"]:
        fail(f"{asset} {suffix}: triangle count mismatch {triangles}/{report['triangle_count']}")
    expected_collision = report["collision"]
    if collision_triangles != expected_collision["triangle_count"]:
        fail(f"{asset} {suffix}: collision geometry count mismatch")
    expected = list(report["dimensions_cm"].values())
    deviations = [abs(actual-wanted)/max(wanted, 1e-9) for actual, wanted in zip(dimensions, expected)]
    if any(value > TOLERANCE for value in deviations):
        fail(f"{asset} {suffix}: dimensions {dimensions} differ from {expected}; deviations={deviations}")
    collision_deviations = [abs(actual-wanted)/max(wanted, 1e-9)
                            for actual, wanted in zip(collision_dimensions, expected_collision["dimensions_cm"])]
    if any(value > TOLERANCE for value in collision_deviations):
        fail(f"{asset} {suffix}: collision dimensions changed; deviations={collision_deviations}")
    return {
        "status": "PASS", "format": suffix[1:].upper(), "file": path.name,
        "mesh_count": len(visual), "visual_object_names": sorted(actual_names),
        "vertex_count": vertices, "triangle_count": triangles,
        "dimensions_cm": dict(zip(("x", "y", "z"), dimensions)),
        "world_aabb": world_bounds, "objects": [object_diagnostic(obj) for obj in visual],
        "dimension_tolerance_fraction": TOLERANCE,
        "collision": {"status": "PASS", "name": collision_name,
                      "vertex_count": collision_vertices, "triangle_count": collision_triangles,
                      "dimensions_cm": collision_dimensions, "world_aabb": collision_bounds,
                      "object": object_diagnostic(collisions[0])},
        "finite_geometry": True, "scale_check": "PASS", "empty_geometry": False,
        "preview_objects": [],
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
    contact = validate_png(OUTPUT / "IshibashiriEnvironment_FirstBatch_ContactSheet.png", (1800, 720))
    if (contact["width"], contact["height"]) != (1800, 720):
        fail(f"contact sheet must be 1800x720, got {contact['width']}x{contact['height']}")
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
