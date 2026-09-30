"""Import the generated first batch into UE 5.6 with explicit shared materials.

UnrealEditor-Cmd <project> -run=pythonscript -script=<this file> -unattended -nullrhi
Run CreateIshibashiriEnvironmentKit.py in Blender first. No map is modified here.
"""
import json
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
SOURCE = ROOT / "Art/Environment/Ishibashiri/Generated"
DESTINATION = "/Game/Environment/Ishibashiri"
ASSETS = ("OldCedar_A", "Rock_A", "BoundaryStone_A")
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
LIB = unreal.MaterialEditingLibrary
EDITOR = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)


def expression(material, kind, tag, x, y):
    # Native mesh constructor references can root old expressions. Reuse them.
    for node in LIB.get_material_expressions(material):
        if isinstance(node, kind) and str(node.get_editor_property("desc")) == tag:
            return node
    node = LIB.create_material_expression(material, kind, x, y)
    node.set_editor_property("desc", tag)
    return node


def import_texture(surface, kind):
    name = "T_Ishibashiri_%s_%s" % (surface, kind)
    path = SOURCE / "Textures" / (name + ".png")
    if not path.is_file():
        raise RuntimeError("Generate the environment kit first: " + str(path))
    task = unreal.AssetImportTask()
    task.filename = str(path)
    task.destination_path = DESTINATION
    task.destination_name = name
    task.automated = task.replace_existing = task.save = True
    TOOLS.import_asset_tasks([task])
    texture = unreal.load_asset(DESTINATION + "/" + name)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError("Texture import failed: " + name)
    texture.set_editor_property("srgb", kind == "BaseColor")
    if kind == "Normal":
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        texture.set_editor_property("flip_green_channel", True)
    elif kind == "Roughness":
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    return texture


def material(surface, textures):
    name = "M_Ishibashiri_" + surface
    result = unreal.load_asset(DESTINATION + "/" + name)
    if result is None:
        result = TOOLS.create_asset(name, DESTINATION, unreal.Material, unreal.MaterialFactoryNew())
    result.set_editor_property("two_sided", surface == "Foliage")
    result.set_editor_property("used_with_instanced_static_meshes", True)
    coordinates = None
    if surface == "Ground":
        world = expression(result, unreal.MaterialExpressionWorldPosition, "ENV_WORLD", -1000, 600)
        mask = expression(result, unreal.MaterialExpressionComponentMask, "ENV_XY", -800, 600)
        mask.set_editor_property("r", True); mask.set_editor_property("g", True)
        mask.set_editor_property("b", False); mask.set_editor_property("a", False)
        LIB.connect_material_expressions(world, "", mask, "Input")
        coordinates = expression(result, unreal.MaterialExpressionMultiply, "ENV_WORLD_UV", -600, 600)
        coordinates.set_editor_property("const_b", 1.0 / 180.0)
        LIB.connect_material_expressions(mask, "", coordinates, "A")
    source_surface = "WetRock" if surface == "BoundaryStone" else surface
    for index, (kind, prop) in enumerate((("BaseColor", unreal.MaterialProperty.MP_BASE_COLOR),
                                        ("Normal", unreal.MaterialProperty.MP_NORMAL),
                                        ("Roughness", unreal.MaterialProperty.MP_ROUGHNESS))):
        sample = expression(result, unreal.MaterialExpressionTextureSample, "ENV_" + kind, -400, index * 220)
        sample.set_editor_property("texture", textures[source_surface][kind])
        sample.set_editor_property("sampler_type", {
            "BaseColor": unreal.MaterialSamplerType.SAMPLERTYPE_COLOR,
            "Normal": unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
            "Roughness": unreal.MaterialSamplerType.SAMPLERTYPE_MASKS,
        }[kind])
        if coordinates:
            LIB.connect_material_expressions(coordinates, "", sample, "Coordinates")
        output = sample
        if kind == "BaseColor":
            tint = expression(result, unreal.MaterialExpressionVectorParameter, "ENV_TINT", -400, -180)
            tint.set_editor_property("parameter_name", "Tint")
            tint.set_editor_property("default_value", unreal.LinearColor(1.18, 1.14, 1.05, 1) if surface == "BoundaryStone" else unreal.LinearColor(1, 1, 1, 1))
            output = expression(result, unreal.MaterialExpressionMultiply, "ENV_COLOR", -100, 0)
            LIB.connect_material_expressions(sample, "RGB", output, "A")
            LIB.connect_material_expressions(tint, "", output, "B")
        LIB.connect_material_property(output, "R" if kind == "Roughness" else ("RGB" if output == sample else ""), prop)
    LIB.recompile_material(result)
    unreal.EditorAssetLibrary.save_loaded_asset(result)
    return result


def import_mesh(short_name, materials):
    name = "SM_Ishibashiri_" + short_name
    directory = SOURCE / short_name
    report = json.loads((directory / (name + "_report.json")).read_text(encoding="utf-8"))
    task = unreal.AssetImportTask()
    task.filename = str(directory / (name + ".fbx"))
    task.destination_path = DESTINATION
    task.destination_name = name
    task.automated = task.replace_existing = task.replace_existing_settings = task.save = True
    task.factory = unreal.FbxFactory()
    options = unreal.FbxImportUI()
    options.automated_import_should_detect_type = False
    options.import_mesh = True
    options.import_as_skeletal = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.import_materials = options.import_textures = False
    data = options.static_mesh_import_data
    data.combine_meshes = True
    data.auto_generate_collision = False
    data.one_convex_hull_per_ucx = True
    data.generate_lightmap_u_vs = False
    data.convert_scene = True
    data.convert_scene_unit = True
    data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    task.options = options
    TOOLS.import_asset_tasks([task])
    mesh = unreal.load_asset(DESTINATION + "/" + name)
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError("Static mesh import failed: " + name)
    slots = mesh.get_editor_property("static_materials")
    expected = ("Bark", "Foliage") if short_name == "OldCedar_A" else (("WetRock",) if short_name == "Rock_A" else ("BoundaryStone",))
    if len(slots) != len(expected):
        raise RuntimeError("Material slot contract failed for %s: %s" % (name, slots))
    for index, slot in enumerate(slots):
        label = str(slot.material_slot_name)
        if label not in expected:
            raise RuntimeError("Unexpected material slot %s on %s" % (label, name))
        mesh.set_material(index, materials[label])
    bounds = mesh.get_bounding_box()
    dimensions = bounds.max - bounds.min
    values = [dimensions.x, dimensions.y, dimensions.z]
    # FBX axes can swap X/Y; the sorted horizontal extents are the scale contract.
    authored = report["dimensions_cm"]
    for measured, target in zip(sorted(values[:2]) + [values[2]], sorted([authored["x"], authored["y"]]) + [authored["z"]]):
        if abs(measured - target) > max(2.0, target * .01):
            raise RuntimeError("Unexpected UE scale for %s: %s vs %s" % (name, values, authored))
    if abs(bounds.min.z) > 2.0:
        raise RuntimeError("Ground pivot failed for %s: min Z=%s" % (name, bounds.min.z))
    collision_count = EDITOR.get_simple_collision_count(mesh)
    if collision_count != 1:
        raise RuntimeError("Expected one authored UCX hull for %s, got %s" % (name, collision_count))
    body = mesh.get_editor_property("body_setup")
    body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX)
    reductions = unreal.StaticMeshReductionOptions()
    reductions.auto_compute_lod_screen_size = False
    reductions.reduction_settings = [unreal.StaticMeshReductionSettings(percent_triangles=amount, screen_size=size)
                                     for amount, size in ((1.0, 1.0), (.5, .45), (.20, .16))]
    if EDITOR.set_lods(mesh, reductions) < 0 or EDITOR.get_lod_count(mesh) != 3:
        raise RuntimeError("Three-LOD generation failed for " + name)
    lod_triangles = [mesh.get_num_triangles(index) for index in range(3)]
    if not (lod_triangles[0] > lod_triangles[1] > lod_triangles[2] > 0):
        raise RuntimeError("LOD triangle reduction failed: " + str(lod_triangles))
    if EDITOR.get_num_uv_channels(mesh, 0) < 1:
        raise RuntimeError("Missing surface UVs: " + name)
    if not unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False):
        raise RuntimeError("Could not save " + name)
    return {"asset": mesh.get_path_name(), "dimensions_cm": values, "ground_min_z_cm": bounds.min.z,
            "material_slots": [str(slot.material_slot_name) for slot in slots],
            "simple_collision_hulls": collision_count, "lod_triangles": lod_triangles,
            "lod_screen_sizes": list(EDITOR.get_lod_screen_sizes(mesh)), "nanite": False}


def main():
    unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.Enable 0")
    textures = {surface: {kind: import_texture(surface, kind) for kind in ("BaseColor", "Normal", "Roughness")}
                for surface in ("Bark", "Foliage", "WetRock", "Ground")}
    materials = {surface: material(surface, textures) for surface in ("Bark", "Foliage", "WetRock", "BoundaryStone", "Ground")}
    meshes = [import_mesh(name, materials) for name in ASSETS]
    unreal.EditorAssetLibrary.save_directory(DESTINATION, only_if_is_dirty=False, recursive=True)
    result = {"status": "PASS", "engine_version": unreal.SystemLibrary.get_engine_version(),
              "destination": DESTINATION, "static_meshes": meshes, "texture_resolution": 1024,
              "textures": [texture.get_path_name() for maps in textures.values() for texture in maps.values()],
              "materials": [value.get_path_name() for value in materials.values()],
              "ground_mapping": "world XY / 180cm", "lod_visual_review": "RUNTIME_REVIEW_REQUIRED",
              "runtime_navigation": "PARENT_REVIEW_REQUIRED", "human_first_play": "NOT_RUN"}
    (SOURCE.parent / "ue-import-report.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    unreal.log("ISHIBASHIRI_ENVIRONMENT_IMPORT_PASS")


if __name__ == "__main__":
    main()
