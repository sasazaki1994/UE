"""Blender audit of regenerated geometry, rest skeletons and all animated poses.

blender -b --python-exit-code 1 --python Tools/VerifyCharacterQuality.py --
  --before Artifacts/CharacterQuality/<run>/Before/Art/Characters
  --output Artifacts/CharacterQuality/<run>/After
"""
import argparse
import importlib.util
import json
import math
from pathlib import Path
import sys

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('maker', ROOT/'Tools/CreateCharacterModels.py')
m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)


def inspect(path):
    bpy.ops.wm.open_mainfile(filepath=str(path))
    rig = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
    mesh = next(o for o in bpy.context.scene.objects if o.type == 'MESH')
    mesh.data.calc_loop_triangles()
    points = [mesh.matrix_world @ v.co for v in mesh.data.vertices]
    weights = [sum(g.weight for g in v.groups) for v in mesh.data.vertices]
    result = {
        'vertices': len(points), 'triangles': len(mesh.data.loop_triangles),
        'bounds_min': [min(p[i] for p in points) for i in range(3)],
        'bounds_max': [max(p[i] for p in points) for i in range(3)],
        'materials': len(mesh.data.materials),
        'invalid_coordinates': sum(not all(math.isfinite(c) for c in p) for p in points),
        'unweighted_or_unnormalized': sum(not math.isfinite(w) or abs(w-1) > .001 for w in weights),
        'bones': {b.name: {'head': list(b.head_local), 'tail': list(b.tail_local),
            'parent': b.parent.name if b.parent else None} for b in rig.data.bones},
    }
    assert not result['invalid_coordinates'] and not result['unweighted_or_unnormalized'], result
    return rig, mesh, result


def run(before, output, names):
    output.mkdir(parents=True, exist_ok=True)
    report = {}
    for name in names:
        filename = Path(name)/'Rigged'/(name+'_Rigged.blend')
        _, _, baseline = inspect(before/filename)
        rig, mesh, after = inspect(ROOT/'Art/Characters'/filename)
        assert baseline['bones'] == after['bones'], 'Changed rest skeleton: '+name
        after['rest_skeleton_unchanged'] = True
        info = json.loads((ROOT/'Art/Characters'/name/'Rigged/rig-info.json').read_text())
        scene = bpy.context.scene
        scene.render.engine = 'CYCLES'; scene.cycles.samples = 16
        scene.cycles.use_denoising = True
        if name == 'Shirotsura':
            m.stage(1, (0,0,.9), (2.8,-4,2), 2.3)
        else:
            m.stage(7, (0,-.2,4.2), (16,-23,14), 18.4)
        scene.render.resolution_x = scene.render.resolution_y = 640
        scene.render.resolution_percentage = 100
        poses = output/'Poses'/name; poses.mkdir(parents=True, exist_ok=True)
        after['clips'] = {}
        for clip, meta in info['animations'].items():
            rig.animation_data.action = bpy.data.actions[clip]
            invalid = 0
            max_delta = 0.
            rest = None
            for phase in (0, .25, .5, .75, 1):
                scene.frame_set(1+round(meta['frames']*phase))
                evaluated = mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
                points = [v.co.copy() for v in evaluated.data.vertices]
                invalid += sum(not all(math.isfinite(c) for c in p) for p in points)
                if rest is None:
                    rest = points
                else:
                    max_delta = max(max_delta, max((a-b).length for a,b in zip(rest,points)))
            assert not invalid, 'Non-finite animated geometry: '+name+'/'+clip
            scene.frame_set(1+round(meta['frames']*.25))
            m.render(poses/(clip+'.png'))
            after['clips'][clip] = {'finite_samples': 5, 'max_vertex_motion_m': max_delta}
        # Actual baked models: details can be judged independently of UV source shaders.
        rig.animation_data.action = bpy.data.actions['Idle']; scene.frame_set(1)
        scene.render.resolution_x = scene.render.resolution_y = 1200
        scene.cycles.samples = 32
        if name == 'Shirotsura':
            m.render(ROOT/'Art/Characters/Previews/Shirotsura_Detail.png', scene.camera,
                     (1.6,-4,1.9), (0,-.03,1.32), 1.02)
        else:
            m.render(ROOT/'Art/Characters/Previews/Ishibashiri_Detail.png', scene.camera,
                     (10,-21,8.8), (0,-3.7,3.3), 8.4)
        report[name] = {'before': baseline, 'after': after}
    baseline_layout = json.loads((before/'Ishibashiri/climb-layout.json').read_text())
    current_layout = json.loads((ROOT/'Art/Characters/Ishibashiri/climb-layout.json').read_text())
    assert baseline_layout == current_layout, 'Climbing layout changed'
    report['climbing_layout_unchanged'] = True
    (output/'geometry-validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print('CHARACTER_QUALITY_GEOMETRY_PASS')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--before', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--character', choices=['Shirotsura','Ishibashiri'])
    args = parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
    run(args.before.resolve(), args.output.resolve(),
        [args.character] if args.character else ['Shirotsura','Ishibashiri'])
