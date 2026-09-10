"""Blender helpers that layer verified microsurface maps before atlas baking."""
import importlib.util
import re
import unicodedata
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / 'Art/Materials/CC0PBR/manifest.json'
MANAGER_SPEC = importlib.util.spec_from_file_location(
    'manage_character_pbr', Path(__file__).with_name('ManageCharacterPBR.py'))
MANAGER = importlib.util.module_from_spec(MANAGER_SPEC)
MANAGER_SPEC.loader.exec_module(MANAGER)

ROLE_TOKENS = {
    'weathered_stone': (('granite',),),
    'aged_whitewood': (('aged', 'whitewood'),),
    'coarse_indigo_cloth': (('indigo', 'work', 'cloth'),),
}
EXPECTED_ROLES = {
    'Shirotsura': {'aged_whitewood', 'coarse_indigo_cloth'},
    'Ishibashiri': {'weathered_stone'},
}
TINTS = {
    'weathered_stone': (.42, .43, .39, 1),
    'aged_whitewood': (.82, .72, .55, 1),
    'coarse_indigo_cloth': (.035, .075, .16, 1),
}


def material_tokens(name):
    """Normalize numbered Blender labels without broad substring matching."""
    normalized = unicodedata.normalize('NFKC', name).casefold().replace('_', ' ')
    return tuple(re.findall(r'[a-z]+', normalized))


def role_for_material(name):
    tokens = set(material_tokens(name))
    return next((role for role, alternatives in ROLE_TOKENS.items()
                 if any(set(words).issubset(tokens) for words in alternatives)), None)


def _not_applied(mode, errors, expected_roles):
    result = {
        'status': 'not_applied', 'mode': mode, 'reason': '; '.join(errors),
        'errors': list(errors), 'applied_materials': [],
        'role_counts': {role: 0 for role in expected_roles},
    }
    if mode == 'strict':
        raise MANAGER.ManifestValidationError(errors)
    return result


def apply_external_pbr(materials, bpy, character=None, mode='fallback',
                       manifest_path=MANIFEST, root=ROOT):
    """Validate the complete cache and targets before any Blender side effect."""
    if mode not in {'strict', 'fallback'}:
        raise ValueError("PBR mode must be 'strict' or 'fallback'")
    expected_roles = EXPECTED_ROLES.get(character, set(MANAGER.ROLES))
    try:
        data = MANAGER.load_validated_manifest(manifest_path, root=root)
    except MANAGER.ManifestValidationError as exc:
        return _not_applied(mode, exc.errors, expected_roles)

    matched = [(mat, role_for_material(mat.name)) for mat in materials]
    target_counts = {role: sum(role == found for _, found in matched)
                     for role in expected_roles}
    missing = [role for role, count in target_counts.items() if count == 0]
    if missing:
        return _not_applied(mode,
                            [f'{character or "model"}: expected material target missing: {role}'
                             for role in sorted(missing)], expected_roles)

    assets = {asset['role']: asset for asset in data['assets']}
    applied = []
    role_counts = {role: 0 for role in expected_roles}
    for mat, role in matched:
        if role not in expected_roles:
            continue
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        p = nodes.get('Principled BSDF')
        for node in list(nodes):
            if node.name.startswith('ExternalPBR_'):
                nodes.remove(node)
        uv = nodes.new('ShaderNodeUVMap'); uv.name = 'ExternalPBR_Coordinates'; uv.uv_map = 'PBRDetailUV'
        mapping = nodes.new('ShaderNodeMapping'); mapping.name = 'ExternalPBR_Mapping'
        scale = {'weathered_stone': 4, 'aged_whitewood': 7, 'coarse_indigo_cloth': 18}[role]
        mapping.inputs['Scale'].default_value = (scale, scale, scale)
        links.new(uv.outputs['UV'], mapping.inputs['Vector']); loaded = {}
        for kind in ('base_color', 'normal_gl', 'roughness'):
            image = bpy.data.images.load(str(Path(root) / assets[role]['maps'][kind]['path']),
                                         check_existing=True)
            if kind != 'base_color': image.colorspace_settings.name = 'Non-Color'
            node = nodes.new('ShaderNodeTexImage'); node.name = 'ExternalPBR_' + kind; node.image = image
            links.new(mapping.outputs['Vector'], node.inputs['Vector']); loaded[kind] = node
        tint = nodes.new('ShaderNodeMixRGB'); tint.name = 'ExternalPBR_DesignTint'; tint.blend_type = 'MULTIPLY'
        tint.inputs[0].default_value = .72; tint.inputs[2].default_value = TINTS[role]
        links.new(loaded['base_color'].outputs['Color'], tint.inputs[1]); links.new(tint.outputs['Color'], p.inputs['Base Color'])
        normal = nodes.new('ShaderNodeNormalMap'); normal.name = 'ExternalPBR_NormalGL'; normal.inputs['Strength'].default_value = .32
        links.new(loaded['normal_gl'].outputs['Color'], normal.inputs['Color']); links.new(normal.outputs['Normal'], p.inputs['Normal'])
        links.new(loaded['roughness'].outputs['Color'], p.inputs['Roughness'])
        applied.append(mat.name); role_counts[role] += 1
    return {'status': 'applied', 'mode': mode, 'reason': None, 'errors': [],
            'applied_materials': applied, 'role_counts': role_counts}
