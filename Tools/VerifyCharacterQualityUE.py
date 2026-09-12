"""Audit the imported character library in UE 5.6, without changing assets.

Run after ImportRiggedCharacters.py and ApplyRiggedMaterials.py with the UE
Python commandlet. Add --repeat-apply to the Python script's arguments only
when a second material application is wanted as an idempotence check.
Set CHARACTER_ASSET_FILTER to Shirotsura or Ishibashiri to audit only that model.
"""
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import runpy
import struct
import sys

import unreal

ASSET_FILTER = os.environ.get('CHARACTER_ASSET_FILTER', '').strip()
if ASSET_FILTER and ASSET_FILTER not in ('Shirotsura', 'Ishibashiri'):
    raise ValueError('CHARACTER_ASSET_FILTER must be Shirotsura or Ishibashiri, or unset for both')
CHARACTERS = (ASSET_FILTER,) if ASSET_FILTER else ('Shirotsura', 'Ishibashiri')


ROOT = Path(unreal.Paths.project_dir()).resolve()
REPORT = ROOT / 'Art/Characters/quality-ue-validation.json'
LIB = unreal.MaterialEditingLibrary
CHANNELS = (
    ('BaseColor', unreal.MaterialProperty.MP_BASE_COLOR,
     unreal.MaterialSamplerType.SAMPLERTYPE_COLOR),
    ('Normal', unreal.MaterialProperty.MP_NORMAL,
     unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL),
    ('Roughness', unreal.MaterialProperty.MP_ROUGHNESS,
     unreal.MaterialSamplerType.SAMPLERTYPE_MASKS),
)


def require(condition, description):
    if not condition:
        raise RuntimeError(description)


def audit():
    characters = {}
    for name in CHARACTERS:
        destination = '/Game/Characters/Rigged/' + name
        source = ROOT / 'Art/Characters' / name / 'Rigged'
        mesh = unreal.load_asset(destination + '/SK_' + name)
        require(isinstance(mesh, unreal.SkeletalMesh), name + ': missing skeletal mesh')
        skeleton = mesh.get_editor_property('skeleton')
        require(skeleton is not None, name + ': missing skeleton')
        textures = {}
        for kind, _, _ in CHANNELS:
            path = destination + '/T_' + name + '_' + kind
            texture = unreal.load_asset(path)
            require(isinstance(texture, unreal.Texture2D), path + ': missing texture')
            compression = texture.get_editor_property('compression_settings')
            srgb = texture.get_editor_property('srgb')
            flip_green = texture.get_editor_property('flip_green_channel')
            require(srgb == (kind == 'BaseColor'), path + ': wrong color space')
            if kind == 'Normal':
                require(compression == unreal.TextureCompressionSettings.TC_NORMALMAP,
                        path + ': wrong normal compression')
                require(flip_green, path + ': OpenGL normal requires green-channel flip')
            elif kind == 'Roughness':
                require(compression == unreal.TextureCompressionSettings.TC_MASKS,
                        path + ': wrong roughness compression')
            image = source / ('T_' + name + '_' + kind + '.png')
            data = image.read_bytes()
            require(data[:8] == b'\x89PNG\r\n\x1a\n', str(image) + ': invalid PNG')
            width, height = struct.unpack('>II', data[16:24])
            require(width == height and width >= 2048, str(image) + ': undersized atlas')
            textures[kind] = {
                'path': texture.get_path_name(), 'srgb': srgb,
                'compression': str(compression), 'flip_green': flip_green,
                'source_pixels': [width, height],
                'source_sha256': hashlib.sha256(data).hexdigest(),
            }
        rig_info = json.loads((source / 'rig-info.json').read_text(encoding='utf-8'))
        clips = {}
        require(len(rig_info['animations']) == (10 if name == 'Shirotsura' else 5),
                name + ': gameplay clip count changed')
        for clip, metadata in rig_info['animations'].items():
            path = destination + '/AN_' + name + '_' + clip
            animation = unreal.load_asset(path)
            require(isinstance(animation, unreal.AnimSequence), path + ': missing animation')
            require(animation.get_editor_property('skeleton') == skeleton,
                    path + ': skeleton mismatch')
            length = animation.get_play_length()
            require(abs(length - metadata['seconds']) < .06, path + ': duration mismatch')
            clips[clip] = length
        materials = []
        for index, slot in enumerate(mesh.get_editor_property('materials')):
            material = slot.material_interface
            require(isinstance(material, unreal.Material), name + ': missing baked material')
            require(material.get_name() == 'M_Baked_' + str(index),
                    name + ': wrong material assigned to slot ' + str(index))
            require(material.get_editor_property('used_with_skeletal_mesh'),
                    material.get_name() + ': skeletal usage disabled')
            properties = {}
            for kind, prop, sampler in CHANNELS:
                node = LIB.get_material_property_input_node(material, prop)
                require(isinstance(node, unreal.MaterialExpressionTextureSample),
                        material.get_name() + ': missing ' + kind + ' sample')
                texture = node.get_editor_property('texture')
                require(texture is not None and texture.get_path_name() == textures[kind]['path'],
                        material.get_name() + ': wrong ' + kind + ' texture')
                require(node.get_editor_property('sampler_type') == sampler,
                        material.get_name() + ': wrong ' + kind + ' sampler')
                properties[kind] = texture.get_path_name()
            label = str(slot.material_slot_name).lower()
            emissive = LIB.get_material_property_input_node(material, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
            require(isinstance(emissive, unreal.MaterialExpressionConstant3Vector),
                    material.get_name() + ': missing explicit emissive constant')
            glow = emissive.get_editor_property('constant')
            expected_glow = (.30, .003, .002) if 'crimson' in label or 'ember' in label else (0, 0, 0)
            require(all(abs(actual - expected) < 1e-6
                        for actual, expected in zip((glow.r, glow.g, glow.b), expected_glow)),
                    material.get_name() + ': stale or incorrect emissive response')
            metal = LIB.get_material_property_input_node(material, unreal.MaterialProperty.MP_METALLIC)
            require(isinstance(metal, unreal.MaterialExpressionConstant),
                    material.get_name() + ': missing explicit metallic constant')
            metallic = metal.get_editor_property('r')
            if any(k in label for k in ('bronze', 'bell')):
                expected_metallic = .78
            elif any(k in label for k in ('iron', 'fittings')):
                expected_metallic = .72
            else:
                expected_metallic = .85 if any(k in label for k in ('blade', 'sharpened')) else 0.
            require(abs(metallic - expected_metallic) < 1e-6,
                    material.get_name() + ': stale or incorrect metallic response')
            materials.append({
                'path': material.get_path_name(), 'slot': str(slot.material_slot_name),
                'nodes': LIB.get_num_material_expressions(material), 'textures': properties,
                'emissive_rgb': [glow.r, glow.g, glow.b], 'metallic': metallic,
            })
        require(bool(materials), name + ': no material slots')
        characters[name] = {
            'mesh': mesh.get_path_name(), 'skeleton': skeleton.get_path_name(),
            'slots': len(materials), 'materials': materials,
            'textures': textures, 'animations': clips,
        }
    return characters


def main():
    result = {'status': 'fail', 'checked_at_utc': datetime.now(timezone.utc).isoformat(),
              'character_filter': ASSET_FILTER or None}
    try:
        before = audit()
        repeat = '--repeat-apply' in sys.argv
        if repeat:
            runpy.run_path(str(ROOT / 'Tools/ApplyRiggedMaterials.py'), run_name='__main__')
            after = audit()
            require(before == after, 'Repeat apply changed node count, wiring, or asset properties')
        else:
            after = before
        result.update(status='pass', repeat_apply_identical=True if repeat else None,
                      characters=after)
    except Exception as error:
        result['error'] = str(error)
        raise
    finally:
        REPORT.write_text(json.dumps(result, indent=2), encoding='utf-8')
    unreal.log('CHARACTER_QUALITY_UE_AUDIT_PASS')


if __name__ == '__main__':
    main()
