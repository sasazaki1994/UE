import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


MANAGE = load_module('manage_pbr_test', ROOT / 'Tools/ManageCharacterPBR.py')
PBR = load_module('character_pbr_test', ROOT / 'Tools/CharacterPBR.py')


class Socket:
    def __init__(self):
        self.default_value = None
        self.links = []
        self.is_linked = False


class Sockets(dict):
    def __init__(self, owner):
        super().__init__()
        self.owner = owner

    def __getitem__(self, key):
        if key not in self:
            self[key] = Socket()
            self[key].node = self.owner
        return super().__getitem__(key)


class Node:
    def __init__(self, name):
        self.name = name
        self.type = 'BUMP' if name == 'ShaderNodeBump' else name
        self.inputs = Sockets(self)
        self.outputs = Sockets(self)


class Nodes(list):
    def __init__(self):
        super().__init__([Node('Principled BSDF')])

    def get(self, name):
        return next((node for node in self if node.name == name), None)

    def new(self, kind):
        node = Node(kind)
        self.append(node)
        return node


class Links:
    def new(self, output, input_):
        link = type('Link', (), {'from_node': getattr(output, 'node', None)})()
        input_.links[:] = [link]
        input_.is_linked = True


class Material:
    def __init__(self, name):
        self.name = name
        self.node_tree = type('Tree', (), {'nodes': Nodes(), 'links': Links()})()


class Images:
    def __init__(self):
        self.loads = []

    def load(self, path, check_existing):
        self.loads.append(path)
        return type('Image', (), {'colorspace_settings': type('Color', (), {'name': ''})()})()


class FakeBpy:
    def __init__(self):
        self.data = type('Data', (), {'images': Images()})()


class PBRTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def manifest(self):
        assets = []
        for role in sorted(MANAGE.ROLES):
            maps = {}
            for kind in sorted(MANAGE.MAPS):
                relative = f'files/{role}-{kind}.bin'
                path = self.root / relative
                path.parent.mkdir(exist_ok=True)
                content = f'{role}/{kind}'.encode()
                path.write_bytes(content)
                maps[kind] = {'url': f'https://example.test/{role}/{kind}',
                              'path': relative,
                              'sha256': hashlib.sha256(content).hexdigest()}
            assets.append({'role': role, 'asset_id': role, 'source_url': 'https://example.test',
                           'resolution': '2K', 'maps': maps})
        return {'schema_version': 1, 'license': {'name': 'CC0 1.0',
                'verification_url': 'https://example.test/license', 'verified': True},
                'assets': assets}

    def write_manifest(self, data):
        path = self.root / 'manifest.json'
        path.write_text(json.dumps(data), encoding='utf-8')
        return path

    def test_real_material_names_are_classified(self):
        cases = {
            '01 • indigo work cloth': 'coarse_indigo_cloth',
            '05 • aged whitewood mask': 'aged_whitewood',
            '03 • GRANITE shoulder stone': 'weathered_stone',
            '01_indigo_work_cloth': 'coarse_indigo_cloth',
        }
        for name, expected in cases.items():
            with self.subTest(name=name):
                self.assertEqual(PBR.role_for_material(name), expected)

    def test_unrelated_materials_are_not_classified(self):
        for name in ('paper talisman', 'rice_straw rope', 'moss', 'exposed skin',
                     'boundary blade', 'petrified corruption'):
            with self.subTest(name=name):
                self.assertIsNone(PBR.role_for_material(name))

    def test_valid_files_and_computed_hashes_pass(self):
        data = self.manifest()
        self.assertEqual(MANAGE.validate(data, root=self.root), [])
        self.assertEqual(MANAGE.load_validated_manifest(self.write_manifest(data), root=self.root), data)

    def test_missing_and_modified_files_are_reported(self):
        data = self.manifest()
        first = data['assets'][0]['maps']['base_color']['path']
        (self.root / first).unlink()
        modified = data['assets'][1]['maps']['normal_gl']['path']
        (self.root / modified).write_bytes(b'modified')
        errors = MANAGE.validate(data, root=self.root)
        self.assertTrue(any('file is missing' in error for error in errors))
        self.assertTrue(any('SHA-256 mismatch' in error for error in errors))

    def test_duplicate_role_and_missing_required_fields_are_reported(self):
        data = self.manifest()
        data['assets'].append(dict(data['assets'][0]))
        data['assets'][1].pop('source_url')
        data['assets'][2]['maps'].pop('roughness')
        errors = MANAGE.validate(data, root=self.root)
        self.assertTrue(any('expected exactly one asset, found 2' in error for error in errors))
        self.assertTrue(any('missing or invalid source_url' in error for error in errors))
        self.assertTrue(any('maps must contain exactly' in error for error in errors))

    def test_invalid_json_and_unexpected_types_have_clear_errors(self):
        path = self.root / 'bad.json'
        path.write_text('{bad', encoding='utf-8')
        with self.assertRaisesRegex(MANAGE.ManifestValidationError, 'invalid JSON'):
            MANAGE.load_manifest(path)
        self.assertIn('manifest: assets must be an array', MANAGE.validate({'schema_version': 1,
            'license': {'name': 'CC0', 'verification_url': 'https://example.test',
                        'verified': True}, 'assets': {}}))

    def test_strict_fails_and_fallback_returns_reason_without_side_effects(self):
        invalid = self.manifest()
        invalid['license']['verified'] = False
        path = self.write_manifest(invalid)
        bpy = FakeBpy()
        material = Material('01 • indigo work cloth')
        before = len(material.node_tree.nodes)
        result = PBR.apply_external_pbr([material], bpy, 'Shirotsura', 'fallback', path, self.root)
        self.assertEqual(result['status'], 'not_applied')
        self.assertIn('unverified', result['reason'])
        self.assertEqual(bpy.data.images.loads, [])
        self.assertEqual(len(material.node_tree.nodes), before)
        with self.assertRaises(PBR.MANAGER.ManifestValidationError):
            PBR.apply_external_pbr([material], bpy, 'Shirotsura', 'strict', path, self.root)

    def test_character_expectations_and_applied_counts(self):
        path = self.write_manifest(self.manifest())
        bpy = FakeBpy()
        hero = [Material('05 • aged whitewood mask'), Material('01 • indigo work cloth')]
        result = PBR.apply_external_pbr(hero, bpy, 'Shirotsura', 'strict', path, self.root)
        self.assertEqual(result['status'], 'applied')
        self.assertEqual(result['applied_materials'], [m.name for m in hero])
        self.assertEqual(result['role_counts'], {'aged_whitewood': 1, 'coarse_indigo_cloth': 1})
        stone = PBR.apply_external_pbr([Material('granite shoulder')], FakeBpy(),
                                       'Ishibashiri', 'strict', path, self.root)
        self.assertEqual(stone['role_counts'], {'weathered_stone': 1})
        missing = PBR.apply_external_pbr([Material('05 • aged whitewood mask')], FakeBpy(),
                                         'Shirotsura', 'fallback', path, self.root)
        self.assertEqual(missing['status'], 'not_applied')
        self.assertIn('coarse_indigo_cloth', missing['reason'])

    def test_external_normal_explicitly_uses_detail_uv(self):
        path = self.write_manifest(self.manifest())
        material = Material('granite shoulder')
        result = PBR.apply_external_pbr([material], FakeBpy(), 'Ishibashiri',
                                        'strict', path, self.root)
        self.assertEqual(result['status'], 'applied')
        normal = material.node_tree.nodes.get('ExternalPBR_NormalGL')
        self.assertEqual(normal.uv_map, 'PBRDetailUV')
        self.assertEqual(normal.space, 'TANGENT')

    def test_external_normal_is_layered_into_existing_bump(self):
        path = self.write_manifest(self.manifest())
        material = Material('granite shoulder')
        tree = material.node_tree
        bump = tree.nodes.new('ShaderNodeBump')
        tree.links.new(bump.outputs['Normal'], tree.nodes.get('Principled BSDF').inputs['Normal'])
        PBR.apply_external_pbr([material], FakeBpy(), 'Ishibashiri',
                               'strict', path, self.root)
        self.assertTrue(bump.inputs['Normal'].is_linked)
        self.assertEqual(bump.inputs['Normal'].links[0].from_node.name,
                         'ExternalPBR_NormalGL')


if __name__ == '__main__':
    unittest.main()
