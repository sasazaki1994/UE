"""Tripo geometry intake, run inside Blender 3.6+ with --disable-autoexec.

Stages: preflight, cleanup, rig, export. Source geometry is never generated here.
Rig uses the existing armature/actions and provisional surface weight transfer.
Manual weight review is mandatory; no name-based prototype geometry weights.
"""
import argparse
import importlib.util
import math
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bpy
import bmesh
from mathutils import Vector, Matrix
from ProductionCharacter import (NAMES, ROOT, folder, read, write, sha, source_path,
    source_fingerprint, assess, require_report, require_review, invalidate)


def load(path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    if path.suffix.lower() == '.blend':
        bpy.ops.wm.open_mainfile(filepath=str(path), load_ui=False, use_scripts=False)
    elif path.suffix.lower() in ('.glb', '.gltf'):
        bpy.ops.import_scene.gltf(filepath=str(path))
    elif path.suffix.lower() == '.fbx':
        bpy.ops.import_scene.fbx(filepath=str(path), use_anim=True)
    else:
        raise ValueError('Unsupported source')
    bpy.context.scene.frame_set(1)


def meshes():
    return [o for o in bpy.context.scene.objects if o.type == 'MESH']


def source_normals(path):
    if path.suffix.lower() not in ('.glb', '.gltf'):
        return {'status': 'importer_generated_or_authored', 'note': 'FBX/Blend normals are inspected after import; authored origin needs review'}
    if path.suffix.lower() == '.glb':
        with path.open('rb') as stream:
            header = stream.read(20)
            if len(header) != 20 or header[:4] != b'glTF':
                raise ValueError('Invalid GLB header')
            size = struct.unpack_from('<I', header, 12)[0]
            if size > path.stat().st_size - 20:
                raise ValueError('Invalid GLB JSON length')
            document = json.loads(stream.read(size))
    else:
        document = json.loads(path.read_text(encoding='utf-8'))
    primitives = [p for mesh in document.get('meshes', []) for p in mesh.get('primitives', [])]
    missing = sum('NORMAL' not in p.get('attributes', {}) for p in primitives)
    return {'status': 'missing' if missing else 'present', 'primitives_without_normals': missing}


def inspect():
    scene = bpy.context.scene
    unit = scene.unit_settings.scale_length
    result = dict(mesh_count=0, vertex_count=0, triangle_count=0,
        object_count=len(scene.objects), material_count=0, texture_count=0,
        bone_count=0, has_animation=False, skeletons=[], objects=[], textures=[],
        missing_textures=[], meshes_without_uv=[], invalid_normals=[],
        empty_material_slots=[], non_finite_vertices=0, degenerate_triangles=0,
        non_manifold_edges=0, loose_vertices=0, disconnected_components=0,
        unit_scale_m=unit, normals_note='Evaluated Blender normals; importer may synthesize absent source normals')
    points, materials, images = [], set(), set()
    for ob in scene.objects:
        result['objects'].append(dict(name=ob.name, type=ob.type,
            is_root=ob.parent is None, origin_m=list(ob.matrix_world.translation * unit),
            rotation_deg=[math.degrees(v) for v in ob.matrix_local.to_euler()], scale=list(ob.scale)))
        if ob.animation_data and (ob.animation_data.action or ob.animation_data.nla_tracks):
            result['has_animation'] = True
        if ob.type == 'ARMATURE':
            result['bone_count'] += len(ob.data.bones)
            result['skeletons'].append({'name': ob.name, 'bones': [b.name for b in ob.data.bones]})
        if ob.type != 'MESH':
            continue
        result['mesh_count'] += 1
        evaluated = ob.evaluated_get(bpy.context.evaluated_depsgraph_get())
        mesh = evaluated.to_mesh()
        try:
            mesh.calc_loop_triangles()
            result['vertex_count'] += len(mesh.vertices)
            result['triangle_count'] += len(mesh.loop_triangles)
            for v in mesh.vertices:
                point = evaluated.matrix_world @ v.co * unit
                if all(math.isfinite(c) for c in point):
                    points.append(point)
                else:
                    result['non_finite_vertices'] += 1
            if not mesh.uv_layers or not any(
                abs((uv[1].uv - uv[0].uv).cross(uv[2].uv - uv[0].uv)) > 1e-10
                for t in mesh.loop_triangles
                for uv in [[mesh.uv_layers.active.data[i] for i in t.loops]]):
                result['meshes_without_uv'].append(ob.name)
            if any(not all(math.isfinite(c) for c in v.normal) or v.normal.length < .5 for v in mesh.vertices):
                result['invalid_normals'].append(ob.name)
            result['degenerate_triangles'] += sum(t.area < 1e-12 for t in mesh.loop_triangles)
            bm = bmesh.new()
            bm.from_mesh(mesh)
            result['non_manifold_edges'] += sum(not e.is_manifold for e in bm.edges)
            result['loose_vertices'] += sum(not v.link_edges for v in bm.verts)
            unseen = set(bm.verts)
            components = 0
            while unseen:
                components += 1
                stack = [unseen.pop()]
                while stack:
                    for edge in stack.pop().link_edges:
                        for v in edge.verts:
                            if v in unseen:
                                unseen.remove(v)
                                stack.append(v)
            result['disconnected_components'] += max(0, components - 1)
            bm.free()
        finally:
            evaluated.to_mesh_clear()
        if not ob.material_slots:
            result['empty_material_slots'].append(ob.name)
        for slot in ob.material_slots:
            if slot.material is None:
                result['empty_material_slots'].append(ob.name)
            else:
                materials.add(slot.material)
    def scan_nodes(tree, visited):
        if not tree or tree in visited:
            return
        visited.add(tree)
        for node in tree.nodes:
            if node.type == 'TEX_IMAGE':
                if node.image:
                    images.add(node.image)
                else:
                    result['missing_textures'].append('Unassigned image node: ' + node.name)
            if node.type == 'GROUP':
                scan_nodes(node.node_tree, visited)
    for mat in materials:
        scan_nodes(mat.node_tree, set())
    for image in images:
        packed = bool(image.packed_file or image.packed_files)
        path = bpy.path.abspath(image.filepath, library=image.library)
        exists = packed or (bool(path) and Path(path).is_file())
        # Generated images must contain pixels and be packed during cleanup.
        if image.source == 'GENERATED':
            exists = image.has_data
        if not exists or image.size[0] == 0 or image.size[1] == 0:
            result['missing_textures'].append(image.name + ': ' + path)
        result['textures'].append(dict(name=image.name, path=path, packed=packed,
            size=list(image.size), channels=image.channels))
    result['texture_count'] = len(images)
    result['material_count'] = len(materials)
    if points:
        lower = [min(p[i] for p in points) for i in range(3)]
        upper = [max(p[i] for p in points) for i in range(3)]
        result['bounding_box_m'] = {'min': lower, 'max': upper}
        result['dimensions_m'] = [b - a for a, b in zip(lower, upper)]
    else:
        result['bounding_box_m'] = None
        result['dimensions_m'] = [0, 0, 0]
    result['estimated_uncompressed_texture_bytes'] = sum(
        t['size'][0] * t['size'][1] * max(1, t['channels']) * 4 / 3 for t in result['textures'])
    return result


def select(objects, active=None):
    bpy.ops.object.select_all(action='DESELECT')
    for ob in objects:
        ob.select_set(True)
    bpy.context.view_layer.objects.active = active or objects[0]


def cleanup(args, manifest):
    """Explicit uniform conversion, no auto-fit, no decimation or voxel remeshing."""
    src = source_path(args.character, manifest)
    load(src)
    if any(o.type == 'ARMATURE' for o in bpy.context.scene.objects):
        raise ValueError('Rigged source: export an unrigged rest-pose mesh for cleanup; preserve original rig in TripoSource')
    if not meshes():
        raise ValueError('No geometry to clean')
    conversion = (Matrix.Translation(Vector(args.offset)) @
                  Matrix.Rotation(math.radians(args.yaw), 4, 'Z') @
                  Matrix.Scale(args.scale * bpy.context.scene.unit_settings.scale_length, 4))
    # Evaluate modifiers before deleting parents/auxiliary objects. No pose fitting.
    deps = bpy.context.evaluated_depsgraph_get()
    for i, ob in enumerate(meshes()):
        world = conversion @ ob.matrix_world
        data = bpy.data.meshes.new_from_object(ob.evaluated_get(deps), depsgraph=deps)
        ob.modifiers.clear()
        ob.data = data
        ob.parent = None
        data.transform(world)
        ob.matrix_world = Matrix.Identity(4)
        ob.name = args.character + '_Tripo_%03d' % i
        ob.animation_data_clear()
        bm = bmesh.new()
        bm.from_mesh(data)
        # Exact duplicate vertices/faces only. UV seams and cloth gaps must survive.
        bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=1e-7)
        loose = [v for v in bm.verts if not v.link_edges]
        if loose:
            bmesh.ops.delete(bm, geom=loose, context='VERTS')
        bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
        bm.to_mesh(data)
        bm.free()
        data.update()
        # Remove only unused/duplicate references, never merge different shaders.
        old = list(data.materials)
        used = sorted({p.material_index for p in data.polygons})
        unique = []
        remap = {}
        for index in used:
            if index >= len(old) or old[index] is None:
                raise ValueError('Mesh contains missing material')
            if old[index] not in unique:
                unique.append(old[index])
            remap[index] = unique.index(old[index])
        assignments = [remap[p.material_index] for p in data.polygons]
        data.materials.clear()
        for mat in unique:
            data.materials.append(mat)
        for polygon, index in zip(data.polygons, assignments):
            polygon.material_index = index
    for ob in list(bpy.context.scene.objects):
        if ob.type != 'MESH':
            bpy.data.objects.remove(ob, do_unlink=True)
    bpy.context.scene.unit_settings.system = 'METRIC'
    bpy.context.scene.unit_settings.scale_length = 1.0
    metrics = inspect()
    errors, warnings = assess(metrics, args.character, strict=True)
    if errors:
        raise ValueError('; '.join(errors))
    bpy.ops.file.pack_all()
    out = folder(args.character) / 'Blender' / (args.character + '_Clean.blend')
    bpy.ops.wm.save_as_mainfile(filepath=str(out))
    return out, metrics, warnings


def import_rig_helper():
    spec = importlib.util.spec_from_file_location('baseline_rig', ROOT / 'Tools/RigCharacterModels.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def required_baseline_actions(character, available):
    """Validate baseline action names against rig-info without generating clips."""
    required = read(ROOT / 'Art/Characters' / character / 'Rigged/rig-info.json')['animations']
    required_names = list(required)
    if len(required_names) != len(set(required_names)):
        raise ValueError('Duplicate required action in rig-info.json')
    missing = [name for name in required_names if name not in available]
    renamed = [name for name in available
               if any(name.startswith(required_name + '.') for required_name in required_names)]
    if missing or renamed:
        raise ValueError('Baseline action contract mismatch: missing=%s unexpected_renames=%s' %
                         (missing, renamed))
    return required_names


def prepare_rig(args):
    base = folder(args.character)
    clean = base / 'Blender' / (args.character + '_Clean.blend')
    require_report(args.character, 'cleanup', clean)
    load(clean)
    for action in list(bpy.data.actions):
        if action.users == 0:
            bpy.data.actions.remove(action)
    target_meshes = meshes()
    baseline = ROOT / 'Art/Characters' / args.character / 'Rigged' / (args.character + '_Rigged.blend')
    with bpy.data.libraries.load(str(baseline), link=False) as (src, dst):
        required_names = required_baseline_actions(args.character, list(src.actions))
        dst.objects = src.objects
        # Actions are independent datablocks. Loading objects does not guarantee that
        # unassigned clips follow them, so append the rig-info contract explicitly.
        dst.actions = required_names
    loaded_actions = [action for action in dst.actions if action]
    loaded_names = [action.name for action in loaded_actions]
    if len(loaded_actions) != len(required_names) or set(loaded_names) != set(required_names):
        raise ValueError('Required action load count/name mismatch: expected %s, loaded %s' %
                         (required_names, loaded_names))
    for action in loaded_actions:
        # Preserve explicitly appended, currently unassigned actions in the prepared blend.
        action.use_fake_user = True
    appended = [o for o in dst.objects if o]
    for ob in appended:
        bpy.context.collection.objects.link(ob)
    rigs = [o for o in appended if o.type == 'ARMATURE']
    donors = [o for o in appended if o.type == 'MESH']
    if len(rigs) != 1 or len(donors) != 1:
        raise ValueError('Baseline must contain one armature and one weighted mesh')
    rig, donor = rigs[0], donors[0]
    rig.data.pose_position = 'REST'
    # Shared skeleton/actions are loaded from the baseline; geometry stays Tripo sourced.
    for body in target_meshes:
        select([body])
        for group in donor.vertex_groups:
            body.vertex_groups.new(name=group.name)
        mod = body.modifiers.new('Provisional baseline weights', 'DATA_TRANSFER')
        mod.object = donor
        mod.use_vert_data = True
        mod.data_types_verts = {'VGROUP_WEIGHTS'}
        mod.vert_mapping = 'POLYINTERP_NEAREST'
        mod.layers_vgroup_select_src = 'ALL'
        mod.layers_vgroup_select_dst = 'NAME'
        bpy.ops.object.modifier_apply(modifier=mod.name)
        body.parent = rig
        armature = body.modifiers.new('Skeleton deformation', 'ARMATURE')
        armature.object = rig
    for ob in appended:
        if ob != rig:
            bpy.data.objects.remove(ob, do_unlink=True)
    select(target_meshes)
    bpy.ops.object.join()
    body = bpy.context.object
    body.name = 'SK_' + args.character
    rig.data.pose_position = 'POSE'
    bpy.context.scene.render.fps = 30
    bpy.context.scene.frame_set(1)
    out = base / 'Blender' / (args.character + '_Rigged.blend')
    bpy.ops.wm.save_as_mainfile(filepath=str(out))
    return out, inspect(), ['Provisional surface weights: inspect and edit every required joint and action before export']


def verify_rig(name, rig, body):
    baseline = ROOT / 'Art/Characters' / name / 'Rigged' / (name + '_Rigged.blend')
    with bpy.data.libraries.load(str(baseline), link=False) as (src, dst):
        dst.armatures = src.armatures
    if len(dst.armatures) != 1:
        raise ValueError('Expected one baseline armature')
    reference = dst.armatures[0]
    if len(rig.data.bones) != len(reference.bones):
        raise ValueError('Bone count changed; retarget to baseline before export')
    for bone in reference.bones:
        other = rig.data.bones.get(bone.name)
        if other is None or (other.parent.name if other.parent else None) != (bone.parent.name if bone.parent else None):
            raise ValueError('Skeleton hierarchy changed: ' + bone.name)
        if any(abs(other.matrix_local[i][j] - bone.matrix_local[i][j]) > 1e-5 for i in range(4) for j in range(4)):
            raise ValueError('Rest pose changed: ' + bone.name)
    bpy.data.armatures.remove(reference)
    bone_names = {b.name for b in rig.data.bones}
    for vertex in body.data.vertices:
        weights = [g.weight for g in vertex.groups if body.vertex_groups[g.group].name in bone_names]
        if not weights or any(not math.isfinite(w) or w < 0 for w in weights) or abs(sum(weights) - 1) > .001:
            raise ValueError('Unweighted/unnormalized vertex: ' + str(vertex.index))
    required = read(ROOT / 'Art/Characters' / name / 'Rigged/rig-info.json')['animations']
    required_names = list(required)
    present = [action.name for action in bpy.data.actions if action.name in required]
    renamed = [action.name for action in bpy.data.actions
               if any(action.name.startswith(clip + '.') for clip in required_names)]
    missing = [clip for clip in required_names if clip not in present]
    if missing or len(present) != len(required_names) or renamed:
        raise ValueError('Baseline action contract mismatch: missing=%s renamed=%s expected_count=%d actual_count=%d' %
                         (missing, renamed, len(required_names), len(present)))
    deformation = {}
    rig.data.pose_position = 'POSE'
    rig.animation_data_clear()
    for p in rig.pose.bones:
        p.matrix_basis = Matrix.Identity(4)
    bpy.context.view_layer.update()
    first = [v.co.copy() for v in body.evaluated_get(bpy.context.evaluated_depsgraph_get()).data.vertices]
    for clip, metadata in required.items():
        action = bpy.data.actions.get(clip)
        if action is None:
            raise ValueError('Missing baseline action: ' + clip)
        rig.animation_data_create()
        for track in rig.animation_data.nla_tracks:
            track.mute = True
        rig.animation_data.action = action
        bpy.context.scene.frame_set(1)
        maximum = 0
        for fraction in (.25, .5, .75, 1):
            bpy.context.scene.frame_set(1 + int(metadata['frames'] * fraction))
            current = body.evaluated_get(bpy.context.evaluated_depsgraph_get()).data.vertices
            if len(current) != len(first):
                raise ValueError('Animated topology changed')
            maximum = max(maximum, sum((p - v.co).length > .0001 for p, v in zip(first, current)))
        if maximum == 0:
            raise ValueError('No skin deformation in ' + clip)
        deformation[clip] = {'deformed_vertices': maximum, 'seconds': metadata['seconds']}
    rig.animation_data_clear()
    for p in rig.pose.bones:
        p.matrix_basis = Matrix.Identity(4)
    bpy.context.scene.frame_set(1)
    return deformation


def export(args):
    base = folder(args.character)
    rigged = base / 'Blender' / (args.character + '_Rigged.blend')
    # Artist may modify the rigged draft; review binds the final file to the source lineage.
    rig_report = require_report(args.character, 'rig')
    clean = base / 'Blender' / (args.character + '_Clean.blend')
    require_report(args.character, 'cleanup', clean)
    if rig_report.get('cleanup_sha256') != sha(clean):
        raise ValueError('Cleanup changed after rig transfer')
    baseline_rig = ROOT / 'Art/Characters' / args.character / 'Rigged' / (args.character + '_Rigged.blend')
    if rig_report.get('baseline_rig_sha256') != sha(baseline_rig):
        raise ValueError('Baseline rig changed after weight transfer')
    require_review(args.character, rigged)
    load(rigged)
    rigs = [o for o in bpy.context.scene.objects if o.type == 'ARMATURE']
    if len(rigs) != 1 or len(meshes()) != 1:
        raise ValueError('Expected one reviewed mesh and baseline rig')
    rig, body = rigs[0], meshes()[0]
    clips = verify_rig(args.character, rig, body)
    metrics = inspect()
    errors, warnings = assess(metrics, args.character, strict=True)
    if errors:
        raise ValueError('; '.join(errors))
    helper = import_rig_helper()
    # Reuse established atlas/UV/FBX conventions without substituting external PBR.
    # Appended baseline images and identically named source images are read inputs,
    # never bake targets. Keep their datablocks but reserve fresh output names.
    for kind in ('BaseColor', 'Normal', 'Roughness'):
        image = bpy.data.images.get('T_' + args.character + '_' + kind)
        if image:
            image.name = 'IntakeReadOnly_' + image.name
    helper.bake_surface(body, base / 'Textures', args.character, apply_external=False)
    select([body, rig], rig)
    out = base / 'Export' / ('SK_' + args.character + '.fbx')
    bpy.ops.export_scene.fbx(filepath=str(out), use_selection=True,
        object_types={'MESH', 'ARMATURE'}, axis_forward='-Y', axis_up='Z',
        add_leaf_bones=False, bake_anim=False, armature_nodetype='NULL')
    textures = {p.name: sha(p) for p in (base / 'Textures').glob('T_*.png')}
    write(base / 'Validation/export-detail.json', {'animations': clips, 'textures': textures,
        'reviewed_rig_sha256': sha(rigged), 'normal_convention': 'OpenGL'})
    return out, metrics, warnings


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('stage', choices=('preflight', 'cleanup', 'rig', 'export'))
    parser.add_argument('--character', choices=NAMES, required=True)
    parser.add_argument('--scale', type=float, default=1.0, help='Explicit uniform conversion after source unit scale')
    parser.add_argument('--yaw', type=float, default=0.0, help='Degrees about Z; target -Y forward, Z up')
    parser.add_argument('--offset', type=float, nargs=3, default=(0, 0, 0), help='Target origin translation in meters')
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:])
    base = folder(args.character)
    manifest = read(base / 'manifest.json')
    report = {'asset_name': args.character, 'stage': args.stage, 'status': 'invalid_source', 'adopted': False}
    try:
        source = source_path(args.character, manifest)
        if source is None or not source.is_file():
            report['status'] = 'missing_source' if source is None else 'invalid_source'
            raise ValueError('BLOCKED — TRIPO SOURCE REQUIRED')
        if not manifest.get('generated_by') or not manifest.get('generation_date'):
            raise ValueError('Manifest generation provenance is required')
        report['source_fingerprint'] = source_fingerprint(args.character, manifest)
        report['file_size_bytes'] = source.stat().st_size
        if not all(math.isfinite(v) for v in (args.scale, args.yaw, *args.offset)) or args.scale <= 0:
            raise ValueError('Invalid conversion transform')
        if args.stage == 'preflight':
            load(source)
            metrics = inspect()
            metrics['source_normal_attributes'] = source_normals(source)
            errors, warnings = assess(metrics, args.character)
            if metrics['source_normal_attributes']['status'] != 'present':
                warnings.append('Source normals require review; Blender may generate normals on import')
            if report['file_size_bytes'] > 512 * 1024 * 1024:
                warnings.append('Source file exceeds 512 MiB; inspect texture/geometry budget')
            report.update(metrics=metrics, errors=errors, warnings=warnings)
            if errors:
                raise ValueError('; '.join(errors))
        else:
            if args.stage == 'cleanup':
                # Preflight must be measured, but explicit unit correction can repair its failures.
                preflight = read(base / 'Validation/preflight.json')
                if not preflight.get('metrics') or preflight.get('source_fingerprint') != report['source_fingerprint']:
                    raise ValueError('Run preflight for this source before cleanup')
                artifact, metrics, warnings = cleanup(args, manifest)
            elif args.stage == 'rig':
                artifact, metrics, warnings = prepare_rig(args)
                report['cleanup_sha256'] = sha(base / 'Blender' / (args.character + '_Clean.blend'))
                report['baseline_rig_sha256'] = sha(ROOT / 'Art/Characters' / args.character / 'Rigged' / (args.character + '_Rigged.blend'))
            else:
                artifact, metrics, warnings = export(args)
            report.update(artifact=str(artifact.relative_to(base)), artifact_sha256=sha(artifact),
                          metrics=metrics, warnings=warnings)
        report['status'] = 'pass'
        # These are processing facts, never rig/visual/gameplay acceptance.
        manifest['processing_status'] = {'preflight': 'preflight_pass', 'cleanup': 'cleaned',
                                        'rig': 'rig_review_required', 'export': 'exported'}[args.stage]
        if args.stage == 'preflight':
            manifest.update(original_format=source.suffix[1:], original_dimensions=metrics['dimensions_m'],
                original_vertex_count=metrics['vertex_count'], original_triangle_count=metrics['triangle_count'],
                texture_count=metrics['texture_count'], has_rig=metrics['bone_count'] > 0,
                has_animation=metrics['has_animation'])
        manifest.update(adopted=False, ue_import_status='not_run', gameplay_validation_status='not_run',
                        visual_validation_status='not_run')
        write(base / 'manifest.json', manifest)
    except Exception as exc:
        report['message'] = str(exc)
        invalidate(args.character, report['status'])
        write(base / 'Validation' / (args.stage + '.json'), report)
        raise
    write(base / 'Validation' / (args.stage + '.json'), report)
    print('PRODUCTION_' + args.stage.upper() + '_PASS ' + args.character)


if __name__ == '__main__':
    main()
