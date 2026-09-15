"""Real Blender round trips using labeled test cubes ONLY, never production assets."""
import sys
import tempfile
from pathlib import Path
from types import SimpleNamespace
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Tools'))
import bpy
import ProductionCharacter as P
import ProductionCharacterBlender as B


def main():
    results = []
    with tempfile.TemporaryDirectory(prefix='intake-fixture-') as tmp:
        tmp = Path(tmp)
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.mesh.primitive_cube_add(size=1)
        ob = bpy.context.object
        ob.name = 'TEST_FIXTURE_NOT_TRIPO'
        ob.scale = (1, .5, 1.72)
        ob.location.z = .86
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        mat = bpy.data.materials.new('TEST_MATERIAL')
        mat.use_nodes = True
        ob.data.materials.append(mat)
        bpy.context.scene.unit_settings.scale_length = 1
        bpy.ops.wm.save_as_mainfile(filepath=str(tmp / 'fixture.blend'))
        bpy.ops.export_scene.fbx(filepath=str(tmp / 'fixture.fbx'), use_selection=True, bake_anim=False)
        bpy.ops.export_scene.gltf(filepath=str(tmp / 'fixture.glb'), export_format='GLB')
        bpy.ops.export_scene.gltf(filepath=str(tmp / 'fixture.gltf'), export_format='GLTF_SEPARATE')
        for suffix in ('.blend', '.fbx', '.glb', '.gltf'):
            B.load(tmp / ('fixture' + suffix))
            metrics = B.inspect()
            errors, warnings = P.assess(metrics, 'Shirotsura')
            assert not errors, (suffix, errors)
            assert metrics['mesh_count'] == 1 and metrics['triangle_count'] == 12
            assert abs(metrics['dimensions_m'][2] - 1.72) < .001
            results.append({'format': suffix, 'status': 'pass', 'triangles': 12})
        ob = B.meshes()[0]
        ob.scale = (.001,) * 3
        bpy.context.view_layer.update()
        assert P.assess(B.inspect(), 'Shirotsura')[0]
        B.load(tmp / 'fixture.blend')
        tex = bpy.data.images.new('missing fixture', 8, 8)
        tex.source = 'FILE'
        tex.filepath = str(tmp / 'missing.png')
        node = B.meshes()[0].data.materials[0].node_tree.nodes.new('ShaderNodeTexImage')
        node.image = tex
        assert B.inspect()['missing_textures']
        # Run actual cleanup, writing only to isolated fixture paths.
        old_root = P.ROOT
        P.ROOT = tmp
        try:
            base = P.folder('Shirotsura')
            for sub in ('TripoSource', 'Blender', 'Validation'):
                (base / sub).mkdir(parents=True, exist_ok=True)
            import shutil
            shutil.copyfile(tmp / 'fixture.glb', base / 'TripoSource/fixture.glb')
            args = SimpleNamespace(character='Shirotsura', scale=1, yaw=0, offset=(0, 0, 0))
            out, metrics, warnings = B.cleanup(args, {'asset_name': 'Shirotsura',
                'source': 'Tripo AI', 'source_file': 'fixture.glb', 'generated_by': 'TEST_FIXTURE_NOT_TRIPO'})
            assert out.is_file() and not P.assess(metrics, 'Shirotsura', strict=True)[0]
            results.append({'stage': 'cleanup', 'status': 'pass'})
            manifest = {'asset_name': 'Shirotsura', 'source': 'Tripo AI',
                        'source_file': 'fixture.glb', 'generated_by': 'TEST_FIXTURE_NOT_TRIPO'}
            P.write(base / 'manifest.json', manifest)
            P.write(base / 'Validation/cleanup.json', {'status': 'pass',
                'source_fingerprint': P.source_fingerprint('Shirotsura', manifest),
                'artifact_sha256': P.sha(out)})
            rigged, rig_metrics, _ = B.prepare_rig(args)
            assert rigged.is_file() and rig_metrics['bone_count'] == 18
            rig = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
            deform = B.verify_rig('Shirotsura', rig, B.meshes()[0])
            assert len(deform) == 10
            results.append({'stage': 'provisional_weight_transfer_and_deformation', 'status': 'pass'})
            for sub in ('Textures', 'Export'):
                (base / sub).mkdir()
            P.write(base / 'Validation/rig.json', {'status': 'pass',
                'source_fingerprint': P.source_fingerprint('Shirotsura', manifest),
                'cleanup_sha256': P.sha(base / 'Blender/Shirotsura_Clean.blend'),
                'baseline_rig_sha256': P.sha(B.ROOT / 'Art/Characters/Shirotsura/Rigged/Shirotsura_Rigged.blend')})
            P.write(base / 'Validation/rig-review.json', {'reviewer': 'TEST_FIXTURE_ONLY',
                'rigged_sha256': P.sha(rigged), 'checks': {k: True for k in P.REVIEW_CHECKS}})
            original_helper = B.import_rig_helper
            def small_fixture_atlas():
                helper = original_helper()
                helper.ATLAS_SIZE = 32
                return helper
            B.import_rig_helper = small_fixture_atlas
            try:
                fbx, _, _ = B.export(args)
                assert fbx.is_file()
                assert len(list((base / 'Textures').glob('T_*.png'))) == 3
                assert tuple(bpy.data.images['T_Shirotsura_BaseColor'].size) == (32, 32)
                results.append({'stage': 'review_gate_bake_fbx_export', 'status': 'pass', 'atlas_size': 32})
            finally:
                B.import_rig_helper = original_helper
        finally:
            P.ROOT = old_root
    P.write(P.ROOT / 'Saved/ProductionIntake/blender-smoke.json', {
        'fixture': 'TEST_FIXTURE_NOT_TRIPO', 'results': results,
        'negative_checks': ['extreme scale/height', 'missing texture'], 'status': 'pass'})
    print('PRODUCTION_BLENDER_FIXTURE_PASS')


if __name__ == '__main__':
    main()
