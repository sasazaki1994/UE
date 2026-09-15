"""UE Editor Python intake. Set PRODUCTION_CHARACTER=Shirotsura or Ishibashiri.

Only hash-bound, cleaned, reviewed exports reach a new staging directory. A mesh
becomes selectable at the fixed candidate path only after import checks pass.
Existing fallback assets and an existing selectable candidate are never replaced.
"""
import os
import sys
from datetime import datetime, timezone
from pathlib import Path

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ProductionCharacter import ROOT, NAMES, folder, read, write, sha, require_report, require_review
from ImportRiggedCharacters import run
from ApplyRiggedMaterials import apply_character_materials


def check_import(name, mesh, baseline, dest):
    skeleton = baseline.get_editor_property('skeleton')
    if not isinstance(mesh, unreal.SkeletalMesh) or mesh.get_editor_property('skeleton') != skeleton:
        raise ValueError('Imported candidate does not share baseline Skeleton')
    if not mesh.get_editor_property('physics_asset'):
        raise ValueError('Physics Asset missing (visual runtime still uses NoCollision)')
    a = unreal.SkeletalMeshComponent()
    b = unreal.SkeletalMeshComponent()
    a.set_skeletal_mesh_asset(baseline)
    b.set_skeletal_mesh_asset(mesh)
    if a.get_num_bones() != b.get_num_bones():
        raise ValueError('Imported bone count changed')
    bones = []
    for i in range(a.get_num_bones()):
        bone = a.get_bone_name(i)
        if bone != b.get_bone_name(i) or a.get_parent_bone(bone) != b.get_parent_bone(bone):
            raise ValueError('Imported hierarchy changed')
        bones.append(str(bone))
    required = ('weapon', 'hand_L', 'hand_R', 'foot_L', 'foot_R') if name == 'Shirotsura' else ('root', 'back', 'core_0', 'core_1', 'core_2')
    if not all(b.does_socket_exist(bone) for bone in required):
        raise ValueError('Required bone/socket not available')
    # Asset bounds are available before registering a component with a world.
    # get_component_bounds on the temporary components above returns zero bounds.
    old_bounds, new_bounds = baseline.get_bounds(), mesh.get_bounds()
    old_origin, old_extent = old_bounds.origin, old_bounds.box_extent
    new_origin, new_extent, new_radius = new_bounds.origin, new_bounds.box_extent, new_bounds.sphere_radius
    for axis in ('x', 'y', 'z'):
        extent = getattr(old_extent, axis)
        ratio = getattr(new_extent, axis) / max(extent, 1)
        if not .75 <= ratio <= 1.25 or abs(getattr(new_origin, axis) - getattr(old_origin, axis)) > extent * .25:
            raise ValueError('Imported bounds/origin outside baseline envelope')
    textures = {}
    for kind in ('BaseColor', 'Normal', 'Roughness'):
        tex = unreal.load_asset(dest + '/T_' + name + '_' + kind)
        if not isinstance(tex, unreal.Texture2D):
            raise ValueError('Missing texture: ' + kind)
        if tex.get_editor_property('srgb') != (kind == 'BaseColor'):
            raise ValueError('Incorrect texture color space: ' + kind)
        if kind == 'Normal' and tex.get_editor_property('compression_settings') != unreal.TextureCompressionSettings.TC_NORMALMAP:
            raise ValueError('Normal compression mismatch')
        textures[kind] = tex
    slots = mesh.get_editor_property('materials')
    if not slots or len(slots) > 16:
        raise ValueError('Invalid material count')
    for slot in slots:
        mat = slot.material_interface
        if not isinstance(mat, unreal.Material):
            raise ValueError('Missing explicit baked Material')
        used = unreal.MaterialEditingLibrary.get_used_textures(mat)
        if not all(t in used for t in textures.values()):
            raise ValueError('Material does not reference all PBR maps')
    clips = read(ROOT / 'Art/Characters' / name / 'Rigged/rig-info.json')['animations']
    for clip, meta in clips.items():
        anim = unreal.load_asset('/Game/Characters/Rigged/' + name + '/AN_' + name + '_' + clip)
        if not isinstance(anim, unreal.AnimSequence) or anim.get_editor_property('skeleton') != skeleton:
            raise ValueError('Missing/incompatible baseline animation: ' + clip)
        if abs(anim.get_play_length() - meta['seconds']) > .06:
            raise ValueError('Baseline clip timing changed: ' + clip)
    return {'skeletal_mesh': mesh.get_path_name(), 'skeleton': skeleton.get_path_name(),
        'physics_asset': mesh.get_editor_property('physics_asset').get_path_name(),
        'bones': bones, 'sockets_or_bones': list(required), 'material_count': len(slots),
        'textures': {k: v.get_path_name() for k, v in textures.items()},
        'animations': list(clips), 'bounds': {'origin': str(new_origin), 'extent': str(new_extent), 'radius': new_radius}, 'collision': 'Runtime component NoCollision; original capsule/ledges authoritative',
        'shader_render_review': 'NOT_RUN — inspect for shader errors/pink in High Quality',
        'animation_deformation': 'NOT_RUN in UE — required gameplay and visual regression'}


def main():
    name = os.environ.get('PRODUCTION_CHARACTER')
    if name not in NAMES:
        raise ValueError('Set PRODUCTION_CHARACTER to Shirotsura or Ishibashiri')
    base = folder(name)
    manifest = read(base / 'manifest.json')
    result = {'asset_name': name, 'status': 'blocked', 'adopted': False}
    try:
        fbx = base / 'Export' / ('SK_' + name + '.fbx')
        export = require_report(name, 'export', fbx)
        require_review(name, base / 'Blender' / (name + '_Rigged.blend'))
        detail = read(base / 'Validation/export-detail.json')
        if detail['reviewed_rig_sha256'] != sha(base / 'Blender' / (name + '_Rigged.blend')):
            raise ValueError('Reviewed rig changed after export')
        for kind in ('BaseColor', 'Normal', 'Roughness'):
            filename = 'T_' + name + '_' + kind + '.png'
            if detail['textures'].get(filename) != sha(base / 'Textures' / filename):
                raise ValueError('PBR texture changed after export: ' + kind)
        target = '/Game/Characters/Production/' + name
        selectable = target + '/SK_' + name
        if unreal.EditorAssetLibrary.does_asset_exist(selectable):
            raise ValueError('Candidate exists; archive/rename it in Editor before a new intake')
        baseline = unreal.load_asset('/Game/Characters/Rigged/' + name + '/SK_' + name)
        if not isinstance(baseline, unreal.SkeletalMesh):
            raise ValueError('Baseline mesh is required for animation compatibility')
        stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S%f')
        dest = target + '/Intake_' + stamp
        opts = unreal.FbxImportUI()
        opts.automated_import_should_detect_type = False
        opts.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
        opts.import_as_skeletal = True
        opts.import_mesh = True
        opts.import_animations = False
        opts.import_materials = False
        opts.import_textures = False
        opts.create_physics_asset = True
        opts.skeleton = baseline.get_editor_property('skeleton')
        opts.skeletal_mesh_import_data.set_editor_property('update_skeleton_reference_pose', False)
        opts.skeletal_mesh_import_data.set_editor_property('use_t0_as_ref_pose', False)
        loaded = run(fbx, dest, 'SK_' + name, opts, replace_existing=False)
        meshes = [m for m in loaded if isinstance(m, unreal.SkeletalMesh)]
        if len(meshes) != 1:
            raise ValueError('Expected exactly one imported Skeletal Mesh')
        mesh = meshes[0]
        expected = dest + '/SK_' + name
        if mesh.get_path_name().split('.')[0] != expected:
            if not unreal.EditorAssetLibrary.rename_asset(mesh.get_path_name(), expected):
                raise ValueError('Cannot normalize staging mesh name')
        apply_character_materials(name, dest, base / 'Textures', replace_existing=False)
        result['inspection'] = check_import(name, mesh, baseline, dest)
        unreal.EditorAssetLibrary.save_directory(dest, only_if_is_dirty=False, recursive=True)
        # Publication means comparison candidate only, not adoption.
        if not unreal.EditorAssetLibrary.rename_asset(mesh.get_path_name(), selectable):
            raise ValueError('Cannot publish checked comparison candidate')
        unreal.EditorAssetLibrary.save_asset(selectable)
        result.update(status='pass', candidate=selectable,
                      source_fingerprint=export['source_fingerprint'], export_sha256=sha(fbx))
        manifest.update(ue_import_status='candidate_imported', adopted=False,
                        gameplay_validation_status='not_run', visual_validation_status='not_run')
        write(base / 'manifest.json', manifest)
    except Exception as exc:
        result.update(status='blocked' if manifest.get('processing_status') == 'missing_source' else 'failed', message=str(exc))
        write(base / 'Validation/ue-import.json', result)
        raise
    write(base / 'Validation/ue-import.json', result)
    unreal.log('PRODUCTION_IMPORT_PASS ' + name)


if __name__ == '__main__':
    main()
