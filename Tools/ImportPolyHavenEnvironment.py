"""Import the prepared CC0 Poly Haven meshes and surfaces over the Ishibashiri environment slots.

UnrealEditor-Cmd <project> -ExecutePythonScript=<this file> -unattended -nullrhi -nosound
Order: FetchPolyHavenEnvironment.py -> (Blender) PreparePolyHavenEnvironment.py and
CreateIshibashiriPropKit.py -> this file.
The procedural BoundaryStone mesh from ImportIshibashiriEnvironmentKit.py keeps its shape and
receives the CC0 mossy rock surface. Procedural props and decals receive CC0 surfaces too.
No map is modified here.
"""
import json
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
CACHE = ROOT / "Saved/ModelDownloads/PolyHaven"
PREPARED = ROOT / "Art/Environment/Ishibashiri/Generated/PolyHaven"
PROPS = ROOT / "Art/Environment/Ishibashiri/Generated/Props"
REPORT = ROOT / "Art/Environment/Ishibashiri/PolyHaven/ue-import-report.json"
DESTINATION = "/Game/Environment/Ishibashiri"
TEXTURES = DESTINATION + "/PolyHaven"
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
LIB = unreal.MaterialEditingLibrary
ASSETS = unreal.EditorAssetLibrary
EDITOR = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
MP = unreal.MaterialProperty


def maps(source, prefix, alpha=None):
    directory = CACHE / source / "textures"
    found = {}
    for kind in ("diff", "nor_gl", "arm"):
        candidates = sorted(directory.glob("%s_%s_1k.*" % (prefix, kind)))
        if not candidates:
            raise RuntimeError("Missing %s %s map; run FetchPolyHavenEnvironment.py" % (prefix, kind))
        found[kind] = candidates[0]
    if alpha:
        found["alpha"] = next(iter(sorted(directory.glob("%s_1k.*" % alpha))))
    return found


def surface_maps(source, resolution):
    directory = CACHE / source
    return {kind: directory / ("%s_%s_%s.jpg" % (source, kind, resolution)) for kind in ("diff", "nor_gl", "arm")}


# (exported slot -> material name, maps, options)
MESHES = {
    "SM_Ishibashiri_OldCedar_A": {
        "Trunk": ("M_Ishibashiri_OldCedar_Trunk", maps("fir_tree_01", "fir_tree_01_trunk_a"), {}),
        "Branch": ("M_Ishibashiri_OldCedar_Branch", maps("fir_tree_01", "fir_tree_01_bark"), {}),
        "Twig": ("M_Ishibashiri_OldCedar_Twig", maps("fir_tree_01", "fir_tree_01_twig", "fir_tree_01_twig_alpha"),
                 {"foliage": True}),
    },
    "SM_Ishibashiri_OldCedar_B": {
        "Trunk": ("M_Ishibashiri_OldCedar_TrunkB", maps("fir_tree_01", "fir_tree_01_trunk_b"), {}),
        "Branch": ("M_Ishibashiri_OldCedar_Branch", maps("fir_tree_01", "fir_tree_01_bark"), {}),
        "Twig": ("M_Ishibashiri_OldCedar_Twig", maps("fir_tree_01", "fir_tree_01_twig", "fir_tree_01_twig_alpha"),
                 {"foliage": True}),
    },
    "SM_Ishibashiri_Rock_A": {
        "Rock": ("M_Ishibashiri_MossRock", maps("rock_moss_set_01", "rock_moss_set_01"), {}),
    },
    "SM_Ishibashiri_Rock_B": {
        "Rock": ("M_Ishibashiri_MossRock", maps("rock_moss_set_01", "rock_moss_set_01"), {}),
    },
    "SM_Ishibashiri_FallenCedar_A": {
        "Bark": ("M_Ishibashiri_FallenCedar", maps("dead_tree_trunk", "dead_tree_trunk"), {}),
    },
    "SM_Ishibashiri_Fern_A": {
        "Fern": ("M_Ishibashiri_Fern", maps("fern_02", "fern_02", "fern_02_alpha"), {"foliage": True}),
    },
}
PROP_MESHES = {
    "SM_Ishibashiri_RitualPost_A": {
        # Faded vermilion paint over grey weathered wood.
        "Wood": ("M_Ishibashiri_RitualWood", surface_maps("rough_wood", "1k"), {"tint": (.95, .34, .22)}),
    },
    "SM_Ishibashiri_OldRope_A": {
        "Rope": ("M_Ishibashiri_RopeStraw", surface_maps("thatch_roof_angled", "1k"), {"tint": (.85, .78, .6)}),
        "Paper": ("M_Ishibashiri_RopePaper", surface_maps("rough_linen", "1k"), {"two_sided": True, "tint": (1.15, 1.1, .98)}),
    },
    "SM_Ishibashiri_SteppingStone_A": {
        "Stone": ("M_Ishibashiri_SteppingStone", surface_maps("mossy_rock", "1k"), {"tint": (.9, .86, .78)}),
    },
}
DECALS = ("D_Ishibashiri_Footprint_A", "D_Ishibashiri_CorruptionCrack_A")
SURFACES = {
    "M_Ishibashiri_Ground": (surface_maps("forest_ground_04", "2k"), {"world_uv_cm": 300.0}),
    "M_Ishibashiri_BoundaryStone": (surface_maps("mossy_rock", "1k"), {}),
}
# (triangle fraction, screen size). Foliage meshes keep 1.0 and receive the Blender-authored LOD files.
LOD_LEVELS = {"SM_Ishibashiri_OldCedar_A": ((1.0, 1.0), (1.0, .5), (1.0, .2), (1.0, .08)),
              "SM_Ishibashiri_OldCedar_B": ((1.0, 1.0), (1.0, .5), (1.0, .2), (1.0, .08)),
              "SM_Ishibashiri_Rock_A": ((1.0, 1.0), (.5, .3), (.2, .1)),
              "SM_Ishibashiri_Rock_B": ((1.0, 1.0), (.5, .3), (.2, .1)),
              "SM_Ishibashiri_FallenCedar_A": ((1.0, 1.0), (.4, .3), (.15, .1))}


def connect(source, output, target, pin):
    # A wrong pin name silently returns False and the material falls back to defaults.
    if not LIB.connect_material_expressions(source, output, target, pin):
        raise RuntimeError("Material connection failed: %s.%s -> %s.%s (inputs: %s)" % (
            source.get_name(), output, target.get_name(), pin, list(LIB.get_material_expression_input_names(target))))


def connect_property(source, output, prop):
    if not LIB.connect_material_property(source, output, prop):
        raise RuntimeError("Material property connection failed: %s.%s -> %s" % (source.get_name(), output, prop))


def import_texture(path, kind, folder=TEXTURES, prefix="T_PH_"):
    name = prefix + path.stem
    task = unreal.AssetImportTask()
    task.filename = str(path)
    task.destination_path = folder
    task.destination_name = name
    task.automated = task.replace_existing = task.save = True
    TOOLS.import_asset_tasks([task])
    texture = unreal.load_asset(folder + "/" + name)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError("Texture import failed: " + str(path))
    texture.set_editor_property("srgb", kind == "diff")
    if kind == "nor_gl":
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        texture.set_editor_property("flip_green_channel", True)
    elif kind == "diff":
        # UE's normal-map detection flags the pale rough_linen diffuse, which breaks its Color sampler.
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
    else:
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    ASSETS.save_loaded_asset(texture)
    return texture


def build_material(name, files, options):
    path = DESTINATION + "/" + name
    material = unreal.load_asset(path)
    if material is None:
        material = TOOLS.create_asset(name, DESTINATION, unreal.Material, unreal.MaterialFactoryNew())
    # UE 5.6 Python cannot enumerate expressions, so re-imports rebuild the graph from scratch.
    LIB.delete_all_material_expressions(material)
    foliage = options.get("foliage", False)
    material.set_editor_property("two_sided", foliage or options.get("two_sided", False))
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED if foliage else unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("used_with_instanced_static_meshes", True)
    coordinates = None
    if "world_uv_cm" in options:
        world = LIB.create_material_expression(material, unreal.MaterialExpressionWorldPosition, -1100, 300)
        mask = LIB.create_material_expression(material, unreal.MaterialExpressionComponentMask, -900, 300)
        for channel, enabled in (("r", True), ("g", True), ("b", False), ("a", False)):
            mask.set_editor_property(channel, enabled)
        connect(world, "", mask, "")
        coordinates = LIB.create_material_expression(material, unreal.MaterialExpressionMultiply, -700, 300)
        coordinates.set_editor_property("const_b", 1.0 / options["world_uv_cm"])
        connect(mask, "", coordinates, "A")
    samplers = {"diff": unreal.MaterialSamplerType.SAMPLERTYPE_COLOR,
                "nor_gl": unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
                "arm": unreal.MaterialSamplerType.SAMPLERTYPE_MASKS,
                "alpha": unreal.MaterialSamplerType.SAMPLERTYPE_MASKS}
    nodes = {}
    for index, (kind, file) in enumerate(files.items()):
        node = LIB.create_material_expression(material, unreal.MaterialExpressionTextureSample, -450, index * 260)
        node.set_editor_property("texture", import_texture(file, kind))
        node.set_editor_property("sampler_type", samplers[kind])
        if coordinates:
            connect(coordinates, "", node, "UVs")
        nodes[kind] = node
    tint = LIB.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -450, -260)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(*options.get("tint", (1, 1, 1)), 1))
    color = LIB.create_material_expression(material, unreal.MaterialExpressionMultiply, -150, -100)
    connect(nodes["diff"], "RGB", color, "A")
    connect(tint, "", color, "B")
    connect_property(color, "", MP.MP_BASE_COLOR)
    connect_property(nodes["nor_gl"], "RGB", MP.MP_NORMAL)
    connect_property(nodes["arm"], "R", MP.MP_AMBIENT_OCCLUSION)
    connect_property(nodes["arm"], "G", MP.MP_ROUGHNESS)
    if "alpha" in nodes:
        connect_property(nodes["alpha"], "R", MP.MP_OPACITY_MASK)
    LIB.recompile_material(material)
    ASSETS.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def build_decal(name):
    """Deferred decal from the procedural BaseColor (alpha = coverage) and Normal maps."""
    folder = DESTINATION + "/Decals"
    color = import_texture(PROPS / (name + "_BaseColor.png"), "diff", folder, "T_")
    normal = import_texture(PROPS / (name + "_Normal.png"), "nor_gl", folder, "T_")
    path = DESTINATION + "/" + name
    material = unreal.load_asset(path)
    if material is None:
        material = TOOLS.create_asset(name, DESTINATION, unreal.Material, unreal.MaterialFactoryNew())
    LIB.delete_all_material_expressions(material)
    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    color_node = LIB.create_material_expression(material, unreal.MaterialExpressionTextureSample, -450, 0)
    color_node.set_editor_property("texture", color)
    normal_node = LIB.create_material_expression(material, unreal.MaterialExpressionTextureSample, -450, 280)
    normal_node.set_editor_property("texture", normal)
    normal_node.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    roughness = LIB.create_material_expression(material, unreal.MaterialExpressionConstant, -450, 520)
    roughness.set_editor_property("r", .42)
    connect_property(color_node, "RGB", MP.MP_BASE_COLOR)
    connect_property(color_node, "A", MP.MP_OPACITY)
    connect_property(normal_node, "RGB", MP.MP_NORMAL)
    connect_property(roughness, "", MP.MP_ROUGHNESS)
    LIB.recompile_material(material)
    ASSETS.save_loaded_asset(material, only_if_is_dirty=False)
    return material.get_path_name()


def import_mesh(name, slots, directory=PREPARED):
    report = json.loads((directory / (name + "_report.json")).read_text(encoding="utf-8"))
    path = DESTINATION + "/" + name
    # Replacing in place would keep the procedural slot layout; a clean import uses the new slots.
    if ASSETS.does_asset_exist(path):
        ASSETS.delete_asset(path)
    task = unreal.AssetImportTask()
    task.filename = str(directory / (name + ".fbx"))
    task.destination_path = DESTINATION
    task.destination_name = name
    task.automated = task.replace_existing = task.save = True
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
    data.generate_lightmap_u_vs = False
    data.convert_scene = True
    data.convert_scene_unit = True
    data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    task.options = options
    TOOLS.import_asset_tasks([task])
    mesh = unreal.load_asset(path)
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError("Static mesh import failed: " + name)
    labels = [str(slot.material_slot_name) for slot in mesh.get_editor_property("static_materials")]
    if sorted(labels) != sorted(slots):
        raise RuntimeError("Material slot contract failed for %s: %s" % (name, labels))
    for index, label in enumerate(labels):
        material_name, files, options = slots[label]
        mesh.set_material(index, build_material(material_name, files, options))
    bounds = mesh.get_bounding_box()
    size = bounds.max - bounds.min
    measured = [size.x, size.y, size.z]
    authored = report["dimensions_cm"]
    for value, target in zip(sorted(measured[:2]) + [measured[2]], sorted(authored[:2]) + [authored[2]]):
        if abs(value - target) > max(2.0, target * .01):
            raise RuntimeError("Unexpected UE scale for %s: %s vs %s" % (name, measured, authored))
    # Ground props sit on Z=0; the rope pivots at its first attachment point and hangs below it.
    expected_min_z = report.get("bounds_min_cm", (0, 0, 0))[2]
    if abs(bounds.min.z - expected_min_z) > 2.0:
        raise RuntimeError("Pivot failed for %s: min Z=%s, expected %s" % (name, bounds.min.z, expected_min_z))
    if name in LOD_LEVELS:
        reductions = unreal.StaticMeshReductionOptions()
        reductions.auto_compute_lod_screen_size = False
        reductions.reduction_settings = [unreal.StaticMeshReductionSettings(percent_triangles=amount, screen_size=screen)
                                         for amount, screen in LOD_LEVELS[name]]
        if EDITOR.set_lods(mesh, reductions) != len(LOD_LEVELS[name]):
            raise RuntimeError("LOD generation failed for " + name)
        for index, lod in enumerate(report.get("authored_lods", ()), start=1):
            if EDITOR.import_lod(mesh, index, str(PREPARED / lod["file"])) != index:
                raise RuntimeError("Authored LOD import failed: " + lod["file"])
    if not ASSETS.save_loaded_asset(mesh, only_if_is_dirty=False):
        raise RuntimeError("Could not save " + name)
    return {"asset": mesh.get_path_name(), "source": report["source"], "dimensions_cm": measured,
            "material_slots": labels, "lod_triangles": [mesh.get_num_triangles(i) for i in range(EDITOR.get_lod_count(mesh))],
            "nanite": False}


def main():
    unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.Enable 0")
    meshes = [import_mesh(name, slots) for name, slots in MESHES.items()]
    meshes += [import_mesh(name, slots, PROPS) for name, slots in PROP_MESHES.items()]
    decals = [build_decal(name) for name in DECALS]
    surfaces = [build_material(name, files, options).get_path_name() for name, (files, options) in SURFACES.items()]
    boundary = unreal.load_asset(DESTINATION + "/SM_Ishibashiri_BoundaryStone_A")
    if boundary is None:
        raise RuntimeError("Run ImportIshibashiriEnvironmentKit.py first for the boundary stone mesh")
    boundary.set_material(0, unreal.load_asset(DESTINATION + "/M_Ishibashiri_BoundaryStone"))
    ASSETS.save_loaded_asset(boundary, only_if_is_dirty=False)
    ASSETS.save_directory(DESTINATION, only_if_is_dirty=False, recursive=True)
    result = {"status": "PASS", "engine_version": unreal.SystemLibrary.get_engine_version(), "license": "CC0 1.0 (Poly Haven)",
              "static_meshes": meshes, "surface_materials": surfaces, "decal_materials": decals,
              "runtime_visual_review": "SEE Docs"}
    REPORT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    unreal.log("ISHIBASHIRI_POLYHAVEN_IMPORT_PASS")


if __name__ == "__main__":
    if EDITOR is None:
        raise RuntimeError("StaticMeshEditorSubsystem unavailable; use -ExecutePythonScript, not -run=pythonscript")
    try:
        main()
    finally:
        unreal.SystemLibrary.quit_editor()
