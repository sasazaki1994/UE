"""Round-trip GLBs and render an honest, same-scale comparison in Blender."""
import bpy
import json
import math
import importlib.util
from pathlib import Path
from mathutils import Vector

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('models', root/'Tools'/'CreateCharacterModels.py')
models = importlib.util.module_from_spec(spec)
spec.loader.exec_module(models)
art = root/'Art'/'Characters'
results = []
for name in ['Shirotsura', 'Ishibashiri']:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(art/name/(name+'.glb')))
    meshes = [ob for ob in bpy.context.scene.objects if ob.type == 'MESH']
    assert meshes, name + ': empty import'
    points = [ob.matrix_world @ v.co for ob in meshes for v in ob.data.vertices]
    assert all(math.isfinite(c) for p in points for c in p), 'Non-finite coordinates'
    actual = [max(p[i] for p in points)-min(p[i] for p in points) for i in range(3)]
    expected = json.loads((art/name/'model-info.json').read_text())
    assert all(abs(a-b)<.001 for a,b in zip(actual,expected['dimensions_xyz_m'])), (name,actual,expected)
    assert all(len(p.vertices)>=3 for ob in meshes for p in ob.data.polygons)
    results.append({'name':name,'glb_roundtrip':'pass','dimensions_xyz_m':actual,
                    'nonempty_meshes':len(meshes),'finite_vertices':True})
(art/'validation.json').write_text(json.dumps(results,indent=2),encoding='utf-8')

bpy.ops.wm.open_mainfile(filepath=str(art/'Ishibashiri'/'Ishibashiri.blend'))
with bpy.data.libraries.load(str(art/'Shirotsura'/'Shirotsura.blend'), link=False) as (src,dst):
    dst.objects = [name for name in src.objects if not name.startswith('STUDIO_')]
hero_collection = bpy.data.collections.new('SCALE_REFERENCE • Shirotsura 1.72 m')
bpy.context.scene.collection.children.link(hero_collection)
for ob in dst.objects:
    if ob:
        hero_collection.objects.link(ob)
        ob.location += Vector((4.4,-3.9,0))
scene = bpy.context.scene
scene.render.resolution_x=1800
scene.render.resolution_y=1200
cam=scene.camera
cam.location=(18,-25,12.5)
cam.data.ortho_scale=19.7
models.aim(cam,(.5,-.3,4.0))
bpy.ops.object.select_all(action='DESELECT')
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':
            area.spaces.active.region_3d.view_perspective='CAMERA'
            area.spaces.active.shading.color_type='MATERIAL'
bpy.ops.wm.save_as_mainfile(filepath=str(art/'CharacterScaleComparison.blend'))
models.render(art/'Previews'/'ScaleComparison.png')
print('CHARACTER_ROUNDTRIP_PASS')
