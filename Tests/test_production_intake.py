"""No production models are needed. Missing source is optional; invalid isn't."""
import copy
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'Tools'))
import ProductionCharacter as P
import ValidateProductionCharacter as V
import ReviewProductionCharacter as R


def healthy(name='Shirotsura'):
    return {'mesh_count': 1, 'vertex_count': 100, 'triangle_count': 100,
        'dimensions_m': [1, .5, 1.72] if name == 'Shirotsura' else [7, 12, 9.4],
        'material_count': 1, 'bone_count': 20, 'objects': [{'name': 'root',
            'scale': [1, 1, 1], 'rotation_deg': [0, 0, 0], 'origin_m': [0, 0, 0], 'is_root': True}]}


class PreflightRules(unittest.TestCase):
    def test_extreme_heights_fail(self):
        for name, height in [('Shirotsura', .05), ('Shirotsura', 100), ('Ishibashiri', 1000)]:
            metrics = healthy(name)
            metrics['dimensions_m'][2] = height
            self.assertTrue(P.assess(metrics, name)[0])

    def test_geometry_texture_scale_rotation_uv_failures(self):
        for field, value in [('triangle_count', 0), ('mesh_count', 0),
                ('missing_textures', ['normal.png']), ('material_count', 0),
                ('non_finite_vertices', 1), ('meshes_without_uv', ['mesh'])]:
            metrics = healthy()
            metrics[field] = value
            self.assertTrue(P.assess(metrics, 'Shirotsura', strict=True)[0], field)
        for key, value in [('scale', [.001] * 3), ('scale', [-1, 1, 1]),
                           ('rotation_deg', [90, 0, 0]), ('origin_m', [10, 0, 0])]:
            metrics = healthy()
            metrics['objects'][0][key] = value
            self.assertTrue(P.assess(metrics, 'Shirotsura', strict=True)[0], key)

    def test_valid_and_expensive(self):
        self.assertFalse(P.assess(healthy(), 'Shirotsura', True)[0])
        metrics = healthy('Ishibashiri')
        metrics['triangle_count'] = 1000000
        self.assertTrue(P.assess(metrics, 'Ishibashiri', True)[0])

    def test_pipeline_gates(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(P, 'ROOT', Path(tmp)):
            base = P.folder('Shirotsura')
            manifest = {'asset_name': 'Shirotsura', 'source': 'Tripo AI', 'source_file': None}
            P.write(base / 'manifest.json', manifest)
            (base / 'TripoSource').mkdir()
            self.assertEqual(V.main(['--character', 'Shirotsura', '--allow-missing']), 0)
            self.assertEqual(P.read(base / 'Validation/preflight.json')['status'], 'missing_source')
            self.assertEqual(V.main(['--character', 'Shirotsura']), 3)
            manifest['source_file'] = 'absent.glb'
            P.write(base / 'manifest.json', manifest)
            self.assertEqual(V.main(['--character', 'Shirotsura', '--allow-missing']), 2)
            manifest['source_file'] = '../not-source.glb'
            P.write(base / 'manifest.json', manifest)
            self.assertEqual(V.main(['--character', 'Shirotsura', '--allow-missing']), 2)
            manifest.update(source_file='source.glb', generated_by='TEST_FIXTURE', generation_date='2026-09-14')
            P.write(base / 'manifest.json', manifest)
            (base / 'TripoSource/source.glb').write_bytes(b'not a model')
            with patch.object(V.subprocess, 'call', return_value=2) as process:
                self.assertEqual(V.main(['--character', 'Shirotsura', '--blender', 'blender', '--allow-missing']), 2)
                self.assertIn('--disable-autoexec', process.call_args.args[0])
            P.write(base / 'Validation/cleanup.json', {'status': 'pass',
                    'source_fingerprint': P.source_fingerprint('Shirotsura', manifest)})
            P.require_report('Shirotsura', 'cleanup')
            (base / 'TripoSource/texture.png').write_bytes(b'changed sidecar')
            with self.assertRaisesRegex(ValueError, 'source changed'):
                P.require_report('Shirotsura', 'cleanup')

    def test_unregistered_and_empty_are_invalid(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(P, 'ROOT', Path(tmp)):
            base = P.folder('Shirotsura')
            P.write(base / 'manifest.json', {'asset_name': 'Shirotsura', 'source': 'Tripo AI', 'source_file': None})
            (base / 'TripoSource').mkdir()
            (base / 'TripoSource/source.glb').touch()
            self.assertEqual(V.main(['--character', 'Shirotsura', '--allow-missing']), 2)

    def test_review_requires_current_hash_and_every_check(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(P, 'ROOT', Path(tmp)):
            base = P.folder('Shirotsura')
            base.mkdir(parents=True)
            rig = base / 'rig.blend'
            rig.write_bytes(b'test fixture')
            P.write(base / 'Validation/rig-review.json', {'reviewer': 'fixture',
                'rigged_sha256': P.sha(rig), 'checks': {k: True for k in P.REVIEW_CHECKS}})
            P.require_review('Shirotsura', rig)
            rig.write_bytes(b'edited')
            with self.assertRaises(ValueError):
                P.require_review('Shirotsura', rig)


class GameplayContract(unittest.TestCase):
    def test_existing_gameplay_authority_matches_main(self):
        contract = P.read(ROOT / 'Tests/production-gameplay-contract.json')
        for filename, digest in contract['sha256'].items():
            source = (ROOT / filename).read_text(encoding='utf-8')
            source = source.replace('#include "ProductionVisuals.h"\n', '')
            if filename.endswith('PrototypePlayer.cpp'):
                source = source.replace('#include "KakonActor.h"\n', '')
                source = source.replace('#include "Misc/PackageName.h"\n', '')
                source = source.replace('    // This optional asset is not supplied by main; preserve the authored base pose without a CDO load error.\n'
                    '    if (FPackageName::DoesPackageExist(TEXT("/Game/Characters/Rigged/Shirotsura/CR_Shirotsura_Climbing")))\n'
                    '    {\n', '')
                source = source.replace('        static ConstructorHelpers::FClassFinder<UControlRig>',
                                        '    static ConstructorHelpers::FClassFinder<UControlRig>')
                source = source.replace('        if (ClimbingRig.Succeeded()) ClimbingControlRig->SetControlRigClass(ClimbingRig.Class);\n    }',
                                        '    if (ClimbingRig.Succeeded()) ClimbingControlRig->SetControlRigClass(ClimbingRig.Class);')
                source = source.replace('    ProductionVisuals::ApplyAtBeginPlay(GetMesh(), TEXT("Shirotsura"));\n', '')
            if filename.endswith('IshibashiriBoss.cpp'):
                source = source.replace('    ProductionVisuals::ApplyAtBeginPlay(Creature, TEXT("Ishibashiri"));\n', '')
            if filename.endswith('CampaignGameInstance.cpp'):
                source = source.replace('ChapterWorldContext', 'WorldContext')
            self.assertEqual(hashlib.sha256(source.encode()).hexdigest(), digest, filename)

    def test_eleven_routes_three_cores(self):
        source = (ROOT / 'Source/IshibashiriPrototype/Private/IshibashiriBoss.cpp').read_text(encoding='utf-8')
        self.assertEqual(source.split('const FVector Route[] = {')[1].split('};')[0].count('{'), 11)
        self.assertEqual(source.split('const FVector Cores[] = {')[1].split('};')[0].count('{'), 3)

    def test_switch_has_no_gameplay_dependencies_or_setters(self):
        source = (ROOT / 'Source/IshibashiriPrototype/Private/ProductionVisuals.cpp').read_text(encoding='utf-8')
        for forbidden in ('KakonActor.h', 'CampaignGameInstance.h', 'PlayerSenseComponent.h',
                          'SetRelative', 'SetCollision', 'SetActor', 'ResetNushi', 'PlayAnimation'):
            self.assertNotIn(forbidden, source)
        for name in ('Fuchimatoi', 'Minedaki', 'Magatsune'):
            self.assertNotIn(name, source)


class AdoptionGate(unittest.TestCase):
    def test_missing_source_never_adopts(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(P, 'ROOT', Path(tmp)):
            P.write(P.folder('Shirotsura') / 'manifest.json', {
                'asset_name': 'Shirotsura', 'source': 'Tripo AI', 'source_file': None})
            result = R.evaluate('Shirotsura', {'scores': {k: 5 for k in R.SCORES}})
            self.assertEqual(result['decision'], 'BLOCKED')
            self.assertFalse(result['adopted'])

    def test_evidence_and_performance_are_required(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(P, 'ROOT', Path(tmp)):
            base = P.folder('Shirotsura')
            (base / 'TripoSource').mkdir(parents=True)
            (base / 'TripoSource/fixture.glb').write_bytes(b'unit fixture, never production')
            P.write(base / 'manifest.json', {'asset_name': 'Shirotsura', 'source': 'Tripo AI', 'source_file': 'fixture.glb'})
            metrics = dict(vertices=1, triangles=1, materials=1, texture_bytes=100, draw_calls=1,
                           fps=60, frame_ms_p50=16, frame_ms_p95=20, trials=3, successes=3)
            report = {'artifact_sha256': 'export-fixture', 'export_sha256': 'export-fixture'}
            review = {'reviewer': 'UNIT_TEST_ONLY', 'export_sha256': 'export-fixture',
                      'capture_conditions': {k: 'fixture' for k in ('camera_transform', 'fov', 'lighting',
                        'resolution', 'hardware', 'driver', 'fixed_scenario')},
                      'captures': {}, 'regression': {k: 'pass' for k in (*R.PLAYER, *R.CAMPAIGN)},
                      'no_missing_or_pink_materials': True, 'no_deformation_breaks': True,
                      'scores': {k: 5 for k in R.SCORES}, 'performance': {
                          'fallback': copy.deepcopy(metrics), 'candidate': copy.deepcopy(metrics)}}
            review['capture_conditions']['render_profile'] = 'DX12_SM6_Lumen_VSM'
            image = base / 'Validation/fixture.png'
            image.parent.mkdir()
            image.write_bytes(b'unit fixture only')
            for view in R.CAPTURES['Shirotsura']:
                review['captures'][view] = {variant: {'file': 'fixture.png', 'sha256': P.sha(image)}
                                            for variant in ('fallback', 'candidate')}
            with patch.object(R, 'require_report', return_value=report):
                self.assertEqual(R.evaluate('Shirotsura', review)['decision'], 'ADOPT')
                review['performance']['candidate'].update(trials=1, successes=1)
                self.assertEqual(R.evaluate('Shirotsura', review)['decision'], 'BLOCKED')
                review['performance']['candidate'].update(trials=3, successes=3)
                review['performance']['candidate']['frame_ms_p95'] = 40
                self.assertEqual(R.evaluate('Shirotsura', review)['decision'], 'REJECT')
                review['performance']['candidate']['frame_ms_p95'] = float('nan')
                self.assertEqual(R.evaluate('Shirotsura', review)['decision'], 'BLOCKED')
                review['performance']['candidate']['frame_ms_p95'] = 20
                review['performance']['candidate']['successes'] = 2
                self.assertEqual(R.evaluate('Shirotsura', review)['decision'], 'REJECT')
                review['regression']['Jump'] = 'not_run'
                self.assertEqual(R.evaluate('Shirotsura', review)['decision'], 'BLOCKED')


if __name__ == '__main__':
    unittest.main()
