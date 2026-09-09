"""Import the static design studies into their own UE review level.
UnrealEditor-Cmd <project> -run=pythonscript -script=<this file> -unattended -nullrhi
Existing game maps and actors are not edited.
"""
from pathlib import Path
import json
import unreal

root = Path(unreal.Paths.project_dir()).resolve()
destination = '/Game/Characters/DesignStudies'
map_path = '/Game/Maps/L_CharacterStudies'
tasks = []
for name in ['Shirotsura', 'Ishibashiri']:
    task = unreal.AssetImportTask()
    task.filename = str(root / 'Art' / 'Characters' / name / (name + '.fbx'))
    task.destination_path = destination + '/' + name
    task.destination_name = 'SM_' + name
    task.automated = True
    task.replace_existing = True
    task.save = True
    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.automated_import_should_detect_type = False
    options.import_materials = True
    options.import_textures = False
    options.static_mesh_import_data.combine_meshes = True
    options.static_mesh_import_data.auto_generate_collision = False
    options.static_mesh_import_data.generate_lightmap_u_vs = False
    task.options = options
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
report = []
assets = []
for name, task in zip(['Shirotsura', 'Ishibashiri'], tasks):
    loaded = [unreal.load_asset(p) for p in task.imported_object_paths]
    meshes = [a for a in loaded if isinstance(a, unreal.StaticMesh)]
    if len(meshes) != 1:
        raise RuntimeError('Expected one combined mesh for ' + name + ': ' + str(task.imported_object_paths))
    asset = meshes[0]
    box = asset.get_bounding_box()
    extent = box.max - box.min
    if name == 'Shirotsura' and not 165 < extent.z < 180:
        raise RuntimeError('Unexpected hero height in centimeters: ' + str(extent.z))
    if name == 'Ishibashiri' and not 900 < extent.z < 1000:
        raise RuntimeError('Unexpected boar height in centimeters: ' + str(extent.z))
    report.append({'name': name, 'asset': asset.get_path_name(),
                   'dimensions_cm': [extent.x, extent.y, extent.z]})
    assets.append(asset)

world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
if not world:
    raise RuntimeError('Could not create the study level')
# A review map uses the base GameMode and never starts the combat prototype.
world.get_world_settings().set_editor_property('default_game_mode', unreal.GameModeBase.static_class())
actor_system = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for name, asset, position in zip(['Shirotsura_172cm', 'Ishibashiri_12m'], assets,
                                [unreal.Vector(440, -390, 0), unreal.Vector(0, 0, 0)]):
    actor = actor_system.spawn_actor_from_class(unreal.StaticMeshActor, position)
    actor.set_actor_label(name)
    actor.static_mesh_component.set_static_mesh(asset)
    actor.tags = ['StaticDesignStudy', 'NoRigOrGameplay']
floor = actor_system.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -12))
floor.set_actor_label('Review_Floor')
floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
floor.set_actor_scale3d(unreal.Vector(40, 40, .20))
sun = actor_system.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1500))
sun.set_actor_rotation(unreal.Rotator(-45, -35, 0), False)
actor_system.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1200))
actor_system.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
if not unreal.EditorLoadingAndSavingUtils.save_map(world, map_path):
    raise RuntimeError('Could not save ' + map_path)
unreal.EditorAssetLibrary.save_directory(destination, only_if_is_dirty=False, recursive=True)
(root / 'Art' / 'Characters' / 'ue-import-report.json').write_text(
    json.dumps({'level': map_path, 'static_meshes': report, 'render_checked': False}, indent=2), encoding='utf-8')
unreal.log('CHARACTER_STUDIES_IMPORT_PASS')
