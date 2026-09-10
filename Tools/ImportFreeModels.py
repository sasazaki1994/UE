"""Import the checked-in FBX exports with the UE editor Python commandlet."""
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve() / 'Art/FreeModels/Export'
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
BASE = '/Game/Characters/FreeModels'
# ExecCmds can be processed after a Python commandlet has already begun. Set
# this here so FbxImportUI's animation-only settings are always respected.
unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.Enable 0')

def import_file(filename, folder, name, options=None):
    task = unreal.AssetImportTask()
    task.filename = str(ROOT / filename)
    task.destination_path = folder
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    if options:
        task.options = options
        task.factory = unreal.FbxFactory()
    TOOLS.import_asset_tasks([task])
    assets = [unreal.load_asset(p) for p in task.imported_object_paths]
    if not assets:
        raise RuntimeError('Nothing imported: ' + filename)
    return assets

def fbx_options(animation=False, skeleton=None):
    ui = unreal.FbxImportUI()
    ui.automated_import_should_detect_type = False
    ui.import_as_skeletal = True
    ui.import_mesh = not animation
    ui.import_animations = animation
    ui.import_materials = False
    ui.import_textures = False
    ui.create_physics_asset = False
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION if animation else unreal.FBXImportType.FBXIT_SKELETAL_MESH
    if skeleton:
        ui.skeleton = skeleton
    data = ui.anim_sequence_import_data if animation else ui.skeletal_mesh_import_data
    data.set_editor_property('convert_scene', True)
    if hasattr(unreal, 'CoordinateSystemPolicy'):
        data.set_editor_property('coordinate_system_policy', unreal.CoordinateSystemPolicy.MATCH_UP_FORWARD_AXES)
    data.set_editor_property('force_front_x_axis', True)
    data.set_editor_property('convert_scene_unit', True)
    if not animation:
        data.set_editor_property('normal_import_method', unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    return ui

def material(folder, name, texture):
    path = folder + '/' + name
    # Meshes referenced by native constructors root their material expressions.
    # Reuse that graph on reimport; replacing the texture at its stable path is
    # sufficient and avoids deleting rooted expressions in a commandlet.
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    mat = TOOLS.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    sample = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -600, 0)
    sample.set_editor_property('parameter_name', 'Albedo')
    sample.set_editor_property('texture', texture)
    tint = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -600, 250)
    tint.set_editor_property('parameter_name', 'Tint')
    tint.set_editor_property('default_value', unreal.LinearColor(1, 1, 1, 1))
    multiply = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 0)
    unreal.MaterialEditingLibrary.connect_material_expressions(sample, 'RGB', multiply, 'A')
    unreal.MaterialEditingLibrary.connect_material_expressions(tint, '', multiply, 'B')
    unreal.MaterialEditingLibrary.connect_material_property(multiply, '', unreal.MaterialProperty.MP_BASE_COLOR)
    rough = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 350)
    rough.set_editor_property('r', 0.85)
    unreal.MaterialEditingLibrary.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    mat.set_editor_property('two_sided', True)
    mat.set_editor_property('used_with_skeletal_mesh', True)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
    return mat

for name, clips in [('Warrior', ['Idle', 'Run', 'Attack', 'Dodge', 'Hit', 'Death']), ('Boar', ['Idle', 'Walk', 'Attack'])]:
    folder = BASE + '/' + name
    assets = import_file(name + '.fbx', folder, 'SK_' + name, fbx_options())
    mesh = next(a for a in assets if isinstance(a, unreal.SkeletalMesh))
    expected = folder + '/SK_' + name
    if mesh.get_path_name().split('.')[0] != expected:
        if not unreal.EditorAssetLibrary.rename_asset(mesh.get_path_name(), expected):
            raise RuntimeError('Cannot normalize skeletal mesh name')
    skeleton = mesh.get_editor_property('skeleton')
    textures = [name + '_Texture'] + (['Warrior_Sword_Texture'] if name == 'Warrior' else [])
    mats = {}
    for tex_name in textures:
        tex = next(a for a in import_file(tex_name + '.png', folder, 'T_' + tex_name) if isinstance(a, unreal.Texture2D))
        mats[tex_name] = material(folder, 'M_' + tex_name, tex)
    slots = mesh.get_editor_property('materials')
    unreal.log('FREE_MODEL_SLOTS ' + name + ' ' + str(slots))
    if not slots:
        raise RuntimeError('Skeletal mesh has no material slots: ' + name)
    for index, slot in enumerate(slots):
        key = 'Warrior_Sword_Texture' if 'Sword' in str(slot.material_slot_name) else name + '_Texture'
        slot.material_interface = mats[key]
        slots[index] = slot
    mesh.set_editor_property('materials', slots)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
    unreal.log('FREE_MODEL_MESH ' + mesh.get_path_name() + ' bounds=' + str(mesh.get_bounds()))
    for clip in clips:
        asset_name = 'AN_' + name + '_' + clip
        assets = import_file(name + '_' + clip + '.fbx', folder, asset_name, fbx_options(True, skeleton))
        if any(isinstance(a, unreal.SkeletalMesh) for a in assets):
            raise RuntimeError('Animation-only import unexpectedly replaced the model: ' + clip)
        anim = next(a for a in assets if isinstance(a, unreal.AnimSequence))
        expected = folder + '/' + asset_name
        if anim.get_path_name().split('.')[0] != expected:
            if not unreal.EditorAssetLibrary.rename_asset(anim.get_path_name(), expected):
                raise RuntimeError('Cannot normalize animation name')
        anim.set_editor_property('force_root_lock', True)
        unreal.EditorAssetLibrary.save_loaded_asset(anim, only_if_is_dirty=False)
        unreal.log('FREE_MODEL_ANIMATION ' + anim.get_path_name() + ' seconds=' + str(anim.get_editor_property('sequence_length')))
unreal.EditorAssetLibrary.save_directory(BASE, only_if_is_dirty=True, recursive=True)
for name in ['Warrior', 'Boar']:
    mesh = unreal.load_asset(BASE + '/' + name + '/SK_' + name)
    unreal.log('FREE_MODEL_FINAL_SLOTS ' + name + ' ' + str(mesh.get_editor_property('materials')))
    if not mesh.get_editor_property('materials') or any(s.material_interface is None for s in mesh.get_editor_property('materials')):
        raise RuntimeError('Missing materials on ' + name)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
unreal.log('FREE_MODELS_IMPORT_PASS')
