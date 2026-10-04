"""Cut the downloaded CC0 Poly Haven sources down to game-ready Ishibashiri meshes.

Run after FetchPolyHavenEnvironment.py:
blender --background --factory-startup --python Tools/PreparePolyHavenEnvironment.py
Each output is one object with a ground-contact origin (XY centre, min Z = 0), exported as FBX
to Art/Environment/Ishibashiri/Generated/PolyHaven together with a JSON report and a preview.
"""
import json
import os
import random
from pathlib import Path

import bmesh
import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
CACHE = ROOT / "Saved/ModelDownloads/PolyHaven"
OUTPUT = ROOT / "Art/Environment/Ishibashiri/Generated/PolyHaven"
SEED = 830194
STEM_ATLAS_U = .17

# slots: source material -> exported slot. foliage: slot whose card islands are thinned.
SPECS = (
    {"asset": "SM_Ishibashiri_OldCedar_A", "source": "fir_tree_01", "object": "fir_tree_01_a_LOD0",
     # Dead-branch cards need their own cutout maps, which the 1k glTF does not ship; they are dropped.
     "slots": {"fir_tree_01_trunk_a": "Trunk", "fir_tree_01_bark": "Branch", "fir_tree_01_dead_branches": None,
               "fir_tree_01_twig": "Twig"},
     # Branch tubes are too thin to decimate; collapsing them leaves flat dark fins.
     "foliage": "Twig", "undecimated": ("Branch",), "card_budget": 400000, "card_scale": 1.9, "solid_ratio": .15,
     "scale": 1.0,
     # (card budget, extra card scale, solid ratio) applied to the finished LOD0.
     "lods": ((110000, 1.35, .35), (30000, 1.6, .3), (8000, 1.6, .25))},
    {"asset": "SM_Ishibashiri_OldCedar_B", "source": "fir_tree_01", "object": "fir_tree_01_b_LOD0",
     "slots": {"fir_tree_01_trunk_b": "Trunk", "fir_tree_01_bark": "Branch", "fir_tree_01_dead_branches": None,
               "fir_tree_01_twig": "Twig"},
     "foliage": "Twig", "undecimated": ("Branch",), "card_budget": 300000, "card_scale": 1.9, "solid_ratio": .15,
     "scale": 1.0, "lods": ((85000, 1.35, .35), (24000, 1.6, .3), (7000, 1.6, .25))},
    {"asset": "SM_Ishibashiri_Rock_A", "source": "rock_moss_set_01", "object": "rock_moss_set_01_rock01",
     "slots": {"rock_moss_set_01": "Rock"}, "solid_ratio": 1.0, "scale": 1.0},
    {"asset": "SM_Ishibashiri_Rock_B", "source": "rock_moss_set_01", "object": "rock_moss_set_01_rock04",
     "slots": {"rock_moss_set_01": "Rock"}, "solid_ratio": 1.0, "scale": 1.0},
    {"asset": "SM_Ishibashiri_FallenCedar_A", "source": "dead_tree_trunk", "object": "dead_tree_trunk",
     "slots": {"dead_tree_trunk": "Bark"}, "solid_ratio": .25, "scale": 4.0},
    {"asset": "SM_Ishibashiri_Fern_A", "source": "fern_02", "object": "fern_02_b",
     "slots": {"fern_02": "Fern"}, "solid_ratio": 1.0, "scale": 1.0},
)


def triangles(obj):
    return sum(len(polygon.vertices) - 2 for polygon in obj.data.polygons)


def slot_triangles(obj):
    counts = {}
    for polygon in obj.data.polygons:
        name = obj.data.materials[polygon.material_index].name
        counts[name] = counts.get(name, 0) + len(polygon.vertices) - 2
    return counts


def isolate(spec):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(CACHE / spec["source"] / (spec["source"] + "_1k.gltf")))
    obj = bpy.data.objects[spec["object"]]
    for other in list(bpy.context.scene.objects):
        if other != obj:
            bpy.data.objects.remove(other, do_unlink=True)
    obj.parent = None
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    obj.location = (0, 0, 0)
    obj.scale = [s * spec["scale"] for s in obj.scale]
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    return obj


def rename_slots(obj, slots):
    """Collapse source materials into the exported slot names, in first-seen order; None drops faces."""
    dropped = {index for index, material in enumerate(obj.data.materials) if slots[material.name] is None}
    if dropped:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.delete(bm, geom=[face for face in bm.faces if face.material_index in dropped], context="FACES")
        bm.to_mesh(obj.data)
        bm.free()
        for index in sorted(dropped, reverse=True):
            obj.data.materials.pop(index=index)
    names = []
    for material in obj.data.materials:
        slot = slots[material.name]
        if slot not in names:
            names.append(slot)
    remap = [names.index(slots[material.name]) for material in obj.data.materials]
    indices = [remap[polygon.material_index] for polygon in obj.data.polygons]
    # clear() resets every polygon to slot 0, so the remapped indices are written afterwards.
    obj.data.materials.clear()
    for name in names:
        obj.data.materials.append(bpy.data.materials.get(name) or bpy.data.materials.new(name))
    obj.data.polygons.foreach_set("material_index", indices)
    obj.data.update()
    return names


def thin_cards(obj, slot_index, budget, scale):
    """Keep seeded foliage cards up to a triangle budget and enlarge them to hold the crown silhouette."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.faces.ensure_lookup_table()
    uv = bm.loops.layers.uv.active
    seen, islands = set(), []
    for face in bm.faces:
        if face.material_index != slot_index or face.index in seen:
            continue
        island, stack = [], [face]
        seen.add(face.index)
        while stack:
            current = stack.pop()
            island.append(current)
            for edge in current.edges:
                for linked in edge.link_faces:
                    if linked.index not in seen and linked.material_index == slot_index:
                        seen.add(linked.index)
                        stack.append(linked)
        islands.append(island)
    random.Random(SEED).shuffle(islands)
    kept, remove = 0, []
    for island in islands:
        count = sum(len(item.verts) - 2 for item in island)
        if kept + count > budget:
            remove.extend(island)
            continue
        kept += count
        # The opaque left strip of the twig atlas is stem bark; enlarged stems read as dark spikes.
        loops = [loop for item in island for loop in item.loops]
        if sum(loop[uv].uv.x for loop in loops) / len(loops) < STEM_ATLAS_U:
            continue
        verts = {vert for item in island for vert in item.verts}
        centre = sum((vert.co for vert in verts), Vector()) / len(verts)
        for vert in verts:
            vert.co = centre + (vert.co - centre) * scale
    bmesh.ops.delete(bm, geom=remove, context="FACES")
    bm.to_mesh(obj.data)
    bm.free()


def decimate_solids(obj, kept_slots, ratio):
    """Collapse each slot on its own except kept_slots; decimating alpha cards would destroy their shapes."""
    if ratio >= 1.0:
        return obj
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.mesh.separate(type="MATERIAL")
    parts = list(bpy.context.selected_objects)
    for part in parts:
        used = {polygon.material_index for polygon in part.data.polygons}
        if any(part.data.materials[index].name in kept_slots for index in used):
            continue
        modifier = part.modifiers.new("Decimate", "DECIMATE")
        modifier.ratio = ratio
        modifier.use_collapse_triangulate = True
        bpy.context.view_layer.objects.active = part
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    bpy.ops.object.select_all(action="DESELECT")
    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    return bpy.context.object


def ground_origin(obj):
    points = [vertex.co for vertex in obj.data.vertices]
    lower = [min(point[i] for point in points) for i in range(3)]
    upper = [max(point[i] for point in points) for i in range(3)]
    offset = Vector(((lower[0] + upper[0]) * .5, (lower[1] + upper[1]) * .5, lower[2]))
    for vertex in obj.data.vertices:
        vertex.co -= offset
    return [round((upper[i] - lower[i]) * 100, 2) for i in range(3)]


def export(obj, path):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={"MESH"}, axis_forward="-Y",
                             axis_up="Z", apply_unit_scale=True, mesh_smooth_type="FACE", add_leaf_bones=False,
                             bake_anim=False, path_mode="STRIP")


def preview(obj, path, dimensions):
    span = max(dimensions) / 100
    target = Vector((0, 0, dimensions[2] / 200))
    bpy.ops.mesh.primitive_plane_add(size=span * 3, location=(0, 0, -.01))
    bpy.ops.object.camera_add(location=target + Vector((.8, -1.0, .35)).normalized() * span * 2.1)
    camera = bpy.context.object
    camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()
    bpy.context.scene.camera = camera
    bpy.ops.object.light_add(type="SUN", rotation=(.7, -.3, -.6))
    bpy.context.object.data.energy = 3.5
    scene = bpy.context.scene
    scene.world = scene.world or bpy.data.worlds.new("World")
    scene.world.color = (.35, .4, .45)
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = scene.render.resolution_y = 720
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)


def prepare(spec):
    obj = isolate(spec)
    source_triangles = triangles(obj)
    names = rename_slots(obj, spec["slots"])
    foliage = names.index(spec["foliage"]) if "foliage" in spec else -1
    print("POLYHAVEN_STEP renamed", slot_triangles(obj))
    if foliage >= 0:
        thin_cards(obj, foliage, spec["card_budget"], spec["card_scale"])
        print("POLYHAVEN_STEP thinned", slot_triangles(obj))
    kept_slots = {spec.get("foliage"), *spec.get("undecimated", ())}
    obj = decimate_solids(obj, kept_slots, spec["solid_ratio"])
    print("POLYHAVEN_STEP decimated", slot_triangles(obj))
    obj.name = obj.data.name = spec["asset"]
    dimensions = ground_origin(obj)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    export(obj, OUTPUT / (spec["asset"] + ".fbx"))
    lods = []
    for index, (budget, scale, ratio) in enumerate(spec.get("lods", ()), start=1):
        # Engine LOD reduction collapses alpha cards into opaque spikes, so foliage LODs are authored here.
        lod = obj.copy()
        lod.data = obj.data.copy()
        bpy.context.collection.objects.link(lod)
        thin_cards(lod, list(lod.data.materials).index(bpy.data.materials[spec["foliage"]]), budget, scale)
        lod = decimate_solids(lod, kept_slots, ratio)
        lod.name = "%s_LOD%d" % (spec["asset"], index)
        export(lod, OUTPUT / (lod.name + ".fbx"))
        lods.append({"file": lod.name + ".fbx", "triangles": triangles(lod), "slot_triangles": slot_triangles(lod)})
        bpy.data.objects.remove(lod, do_unlink=True)
    report = {"asset": spec["asset"], "source": spec["source"], "source_object": spec["object"],
              "license": "CC0 1.0 (Poly Haven)", "source_triangles": source_triangles,
              "triangles": triangles(obj), "slot_triangles": slot_triangles(obj), "dimensions_cm": dimensions,
              "material_slots": names, "authored_lods": lods,
              "scale": spec["scale"], "blender_version": bpy.app.version_string}
    (OUTPUT / (spec["asset"] + "_report.json")).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    preview(obj, OUTPUT / (spec["asset"] + "_preview.png"), dimensions)
    print("POLYHAVEN_PREPARED", json.dumps(report))


def main():
    only = set(filter(None, os.environ.get("POLYHAVEN_ASSETS", "").split(",")))
    for spec in SPECS:
        if not only or spec["asset"] in only:
            prepare(spec)
    print("POLYHAVEN_PREPARE_PASS", len(SPECS))


if __name__ == "__main__":
    main()
