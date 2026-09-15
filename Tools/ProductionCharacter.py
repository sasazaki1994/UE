"""Shared, Blender-independent production intake contracts. No implicit adoption."""
import hashlib
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NAMES = ('Shirotsura', 'Ishibashiri')
FORMATS = {'.glb', '.gltf', '.fbx', '.blend'}
BLOCKED = 'BLOCKED — TRIPO SOURCE REQUIRED'
REVIEW_CHECKS = ('orientation', 'topology_uv_materials', 'silhouette',
                 'weights_and_joints', 'socket_alignment', 'route_clearance',
                 'no_integrated_kakon', 'no_duplicate_sense_geometry')


def folder(name):
    if name not in NAMES:
        raise ValueError('Only Shirotsura and Ishibashiri are in scope')
    return ROOT / 'Art/Characters/Production' / name


def read(path):
    return json.loads(Path(path).read_text(encoding='utf-8'))


def write(path, data):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2, allow_nan=False) + '\n', encoding='utf-8')


def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def confined(base, relative):
    path = (Path(base) / relative).resolve()
    if not path.is_relative_to(Path(base).resolve()):
        raise ValueError('Path escapes intake directory: ' + str(relative))
    return path


def source_path(name, manifest):
    if manifest.get('asset_name') != name or manifest.get('source') != 'Tripo AI':
        raise ValueError('Manifest asset/source mismatch')
    value = manifest.get('source_file')
    if not value:
        return None
    path = confined(folder(name) / 'TripoSource', value)
    if path.suffix.lower() not in FORMATS:
        raise ValueError('Unsupported source format: ' + path.suffix)
    return path


def source_fingerprint(name, manifest):
    """Include sidecar textures/buffers: changing only a .bin invalidates gates too."""
    path = source_path(name, manifest)
    if path is None or not path.is_file():
        return None
    base = folder(name) / 'TripoSource'
    return {str(p.relative_to(base)).replace('\\', '/'): sha(p)
            for p in sorted(base.rglob('*')) if p.is_file() and p.name != '.gitkeep'}


def assess(metrics, name, strict=False):
    """Inspect measured geometry; import/export requires stricter, normalized units."""
    errors, warnings = [], []
    if metrics.get('mesh_count', 0) == 0 or metrics.get('triangle_count', 0) == 0:
        errors.append('Geometry missing')
    dims = metrics.get('dimensions_m', [])
    if len(dims) != 3 or not all(math.isfinite(x) and x > 0 for x in dims):
        errors.append('Invalid/empty dimensions')
    else:
        lo, hi = (1.55, 1.90) if name == 'Shirotsura' else (7.5, 10.5)
        if not lo <= dims[2] <= hi:
            (errors if strict or dims[2] < lo / 3 or dims[2] > hi * 3 else warnings).append(
                'Height outside contract: %.4fm (expected %.2f..%.2f)' % (dims[2], lo, hi))
        if name == 'Ishibashiri' and not 10 <= dims[1] <= 14:
            (errors if strict or max(dims) > 50 else warnings).append('Ishibashiri length must fit existing ~12m route')
    if metrics.get('missing_textures'):
        errors.append('Missing texture references: ' + ', '.join(metrics['missing_textures']))
    if not metrics.get('material_count') or metrics.get('empty_material_slots'):
        errors.append('Missing material / empty material slots')
    for key in ('meshes_without_uv', 'invalid_normals', 'degenerate_triangles'):
        if metrics.get(key):
            (errors if strict else warnings).append(key + ': ' + str(metrics[key]))
    if metrics.get('non_finite_vertices'):
        errors.append('Non-finite vertex coordinates')
    for ob in metrics.get('objects', []):
        scale = ob['scale']
        if any(not math.isfinite(v) or v <= 0 or v < .01 or v > 100 for v in scale):
            errors.append(ob['name'] + ': invalid/extreme scale')
        elif any(abs(v - 1) > .001 for v in scale):
            (errors if strict else warnings).append(ob['name'] + ': unapplied scale')
        if ob.get('is_root') and any(abs(v) > .01 for v in ob['rotation_deg']):
            (errors if strict else warnings).append(ob['name'] + ': root rotation needs orientation review')
        if ob.get('is_root') and any(abs(v) > .01 for v in ob['origin_m']):
            (errors if strict else warnings).append(ob['name'] + ': origin is not gameplay origin')
    for key in ('non_manifold_edges', 'loose_vertices', 'disconnected_components'):
        if metrics.get(key, 0):
            warnings.append(key + ': ' + str(metrics[key]) + ' (inspect intentionally separate clothing/rocks)')
    budget = 100000 if name == 'Shirotsura' else 350000
    if metrics.get('triangle_count', 0) > budget:
        (errors if strict else warnings).append('Triangle budget exceeded: ' + str(budget))
    if metrics.get('material_count', 0) > 16:
        (errors if strict else warnings).append('Material budget exceeded: 16')
    if not metrics.get('bone_count'):
        warnings.append('No skeleton; rig/weight stage required')
    if not metrics.get('has_animation'):
        warnings.append('No source animation; reuse baseline clips after compatible rigging')
    return errors, warnings


def require_report(name, stage, artifact=None):
    data = read(folder(name) / 'Validation' / (stage + '.json'))
    if data.get('status') != 'pass':
        raise ValueError(stage + ' gate has not passed')
    manifest = read(folder(name) / 'manifest.json')
    fingerprint = source_fingerprint(name, manifest)
    if not fingerprint or data.get('source_fingerprint') != fingerprint:
        raise ValueError(stage + ' source changed; rerun pipeline')
    if artifact and data.get('artifact_sha256') != sha(artifact):
        raise ValueError(stage + ' artifact changed; rerun pipeline')
    return data


def require_review(name, rigged):
    review = read(folder(name) / 'Validation/rig-review.json')
    if review.get('rigged_sha256') != sha(rigged) or not review.get('reviewer'):
        raise ValueError('Rig review must name reviewer and current rigged .blend hash')
    if not all(review.get('checks', {}).get(key) is True for key in REVIEW_CHECKS):
        raise ValueError('Complete deformation, socket, route and visual review before export')
    return review


def invalidate(name, status):
    path = folder(name) / 'manifest.json'
    manifest = read(path)
    manifest.update(processing_status=status, adopted=False, ue_import_status='not_run',
                    gameplay_validation_status='not_run', visual_validation_status='not_run')
    if status == 'missing_source':
        for key in ('ue_import_status', 'gameplay_validation_status', 'visual_validation_status'):
            manifest[key] = 'blocked_missing_source'
    write(path, manifest)
