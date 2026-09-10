"""Run with Blender 3.6: blender -b --disable-autoexec --python Tools/ExportFreeModels.py."""
from pathlib import Path
import bpy

ROOT = Path(__file__).resolve().parents[1] / 'Art/FreeModels'
EXPORT = ROOT / 'Export'
EXPORT.mkdir(parents=True, exist_ok=True)

def export_model(name, clips):
    source = ROOT / 'Sources' / name
    bpy.ops.wm.open_mainfile(filepath=str(source / (name + '.blend')), load_ui=False, use_scripts=False)
    armature = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
    armature.animation_data_create()
    armature.animation_data.action = None
    for track in armature.animation_data.nla_tracks:
        track.mute = True
    armature.data.pose_position = 'REST'
    bpy.context.view_layer.update()
    # Convert rigid bone attachments (sword and pauldrons) into skinned parts,
    # preserving their rest transforms, so UE imports one complete character.
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    for obj in meshes:
        # Joining UV layers with different names creates empty channels on the
        # other parts. Keep each part's render UVs in the same channel zero.
        # Boar includes an unused planar "projection-test" layer in front of
        # the authored texture atlas; the atlas is explicitly named UVMap.
        render_uv = obj.data.uv_layers.get('UVMap') if name == 'Boar' else next((uv for uv in obj.data.uv_layers if uv.active_render), None)
        if render_uv:
            for uv in list(obj.data.uv_layers):
                if uv != render_uv:
                    obj.data.uv_layers.remove(uv)
            render_uv.name = 'UVMap'
        if obj.parent_type == 'BONE':
            bone = obj.parent_bone
            world = obj.matrix_world.copy()
            obj.parent_type = 'OBJECT'
            obj.parent = armature
            obj.matrix_world = world
            group = obj.vertex_groups.get(bone) or obj.vertex_groups.new(name=bone)
            group.add(list(range(len(obj.data.vertices))), 1.0, 'REPLACE')
            modifier = obj.modifiers.new('Skin', 'ARMATURE')
            modifier.object = armature
    bpy.ops.object.select_all(action='DESELECT')
    for obj in meshes:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    if len(meshes) > 1:
        bpy.ops.object.join()
    mesh = bpy.context.object
    mesh.name = name
    for img in bpy.data.images:
        if img.packed_file:
            img.filepath_raw = str(EXPORT / (name + '_Texture.png'))
            img.file_format = 'PNG'
            img.save()
    if name == 'Warrior':
        import shutil
        for texture in source.glob('*.png'):
            shutil.copy2(texture, EXPORT / texture.name)
    # Materials are created explicitly in UE from the source textures.
    bpy.ops.object.select_all(action='DESELECT')
    mesh.select_set(True)
    armature.select_set(True)
    bpy.context.view_layer.objects.active = armature
    options = dict(use_selection=True, object_types={'ARMATURE', 'MESH'},
                   add_leaf_bones=False, axis_forward='-Y', axis_up='Z', mesh_smooth_type='FACE',
                   use_armature_deform_only=False, bake_anim_use_all_actions=False,
                   bake_anim_use_nla_strips=False, bake_anim_simplify_factor=0.0)
    bpy.ops.export_scene.fbx(filepath=str(EXPORT / (name + '.fbx')), bake_anim=False, **options)
    armature.data.pose_position = 'POSE'
    for target, action_name in clips.items():
        action = bpy.data.actions[action_name]
        armature.animation_data.action = action
        bpy.context.scene.frame_start = int(action.frame_range[0])
        bpy.context.scene.frame_end = int(action.frame_range[1])
        bpy.context.scene.frame_set(bpy.context.scene.frame_start)
        bpy.ops.export_scene.fbx(filepath=str(EXPORT / (name + '_' + target + '.fbx')), bake_anim=True, **options)
    print('FREE_MODEL_EXPORTED', name)

export_model('Warrior', {'Idle':'Idle_Weapon', 'Run':'Run_Weapon', 'Attack':'Sword_Attack', 'Dodge':'Roll', 'Hit':'RecieveHit', 'Death':'Death'})
export_model('Boar', {'Idle':'default', 'Walk':'walk', 'Attack':'attack'})
