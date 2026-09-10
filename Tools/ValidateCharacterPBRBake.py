"""Offline Blender 3.6 validation fixture for the production PBR bake path.

Run with: blender --background --factory-startup --python Tools/ValidateCharacterPBRBake.py
Generated maps are directional test data, not acquired production/CC0 assets.
"""
import hashlib
import importlib.util
import json
from pathlib import Path

import bpy

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'Artifacts'/'PBRBakeValidation';OUT.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('rig_pbr_validation',ROOT/'Tools'/'RigCharacterModels.py')
rig=importlib.util.module_from_spec(spec);spec.loader.exec_module(rig)

def image_file(name,kind,size=64):
    image=bpy.data.images.new(name,width=size,height=size,alpha=False)
    pixels=[]
    for y in range(size):
        for x in range(size):
            if kind=='base_color':
                value=.15 if ((x//8)+(y//8))%2 else .85; rgba=(value,.2,1-value,1)
            elif kind=='normal_gl':
                # +Y (green) makes accidental DirectX conversion visually obvious.
                rgba=(.5,.85,.72,1)
            else: rgba=(x/(size-1),)*3+(1,)
            pixels.extend(rgba)
    image.pixels[:]=pixels;image.filepath_raw=str(OUT/(name+'.png'));image.file_format='PNG';image.save()
    return Path(image.filepath_raw)

maps={kind:image_file('fixture_'+kind,kind) for kind in ('base_color','normal_gl','roughness')}
assets=[]
for role in sorted(rig.PBR.MANAGER.ROLES):
    records={kind:{'url':'generated://validation/'+kind,
                   'path':str(path.relative_to(ROOT)),
                   'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
             for kind,path in maps.items()}
    assets.append({'role':role,'asset_id':'generated-directional-fixture',
                   'source_url':'generated://validation','resolution':'64px','maps':records})
manifest=OUT/'fixture-manifest.json'
manifest.write_text(json.dumps({'schema_version':1,
    'license':{'name':'Repository-generated validation fixture; not production CC0',
               'verification_url':'generated://validation','verified':True},
    'assets':assets},indent=2),encoding='utf-8')

# A curved mesh with no UVs, procedural authored bump, and a recognizable ground plane.
bpy.ops.mesh.primitive_uv_sphere_add(segments=32,ring_count=16,location=(0,0,0))
body=bpy.context.object;body.name='PBRValidationMesh'
for uv in list(body.data.uv_layers): body.data.uv_layers.remove(uv)
mat=bpy.data.materials.new('granite shoulder');mat.use_nodes=True
nodes=mat.node_tree.nodes;links=mat.node_tree.links;p=nodes.get('Principled BSDF')
noise=nodes.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=8
bump=nodes.new('ShaderNodeBump');bump.name='Authored_Bump';bump.inputs['Strength'].default_value=.45
links.new(noise.outputs['Fac'],bump.inputs['Height']);links.new(bump.outputs['Normal'],p.inputs['Normal'])
body.data.materials.append(mat)

scene=bpy.context.scene;scene.render.engine='BLENDER_EEVEE';scene.render.resolution_x=512;scene.render.resolution_y=512
bpy.ops.object.camera_add(location=(3,-4,2.4));camera=bpy.context.object;scene.camera=camera
camera.rotation_euler=(1.15,0,.65)
bpy.ops.object.light_add(type='AREA',location=(2,-2,4));bpy.context.object.data.energy=900;bpy.context.object.data.size=4

detail,atlas=rig.ensure_bake_uvs(body)
assert detail != atlas and rig._uv_has_area(body.data,detail) and rig._uv_has_area(body.data,atlas)
result=rig.PBR.apply_external_pbr([mat],bpy,'Ishibashiri','strict',manifest,ROOT)
assert result['role_counts']=={'weathered_stone':1}
assert bump.inputs['Normal'].is_linked,'external normal was not layered below authored Bump'
scene.render.filepath=str(OUT/'before-bake.png');bpy.ops.render.render(write_still=True)

node_count=len(nodes)
rig.bake_surface(body,OUT,'Validation',manifest,ROOT)
scene.render.filepath=str(OUT/'after-bake.png');bpy.ops.render.render(write_still=True)
rig.bake_surface(body,OUT,'Validation',manifest,ROOT)
assert len(nodes)==node_count+5,'second application grew the persistent node graph'
assert body.data.uv_layers[0].name=='AtlasUV','FBX/UE channel 0 is not the baked atlas'

body.select_set(True);bpy.context.view_layer.objects.active=body
exports={
    'fbx':OUT/'validation.fbx',
    'glb':OUT/'validation.glb',
}
bpy.ops.export_scene.fbx(filepath=str(exports['fbx']),use_selection=True,bake_anim=False)
bpy.ops.export_scene.gltf(filepath=str(exports['glb']),use_selection=True,export_format='GLB')
reload_results={}
for kind,path in exports.items():
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    (bpy.ops.import_scene.fbx(filepath=str(path)) if kind=='fbx' else bpy.ops.import_scene.gltf(filepath=str(path)))
    imported=next(obj for obj in scene.objects if obj.type=='MESH')
    valid=[uv.name for uv in imported.data.uv_layers if rig._uv_has_area(imported.data,uv)]
    assert valid,kind+' reload has no usable exported UV'
    reload_results[kind]={'valid_uv_layers':valid,'vertices':len(imported.data.vertices)}

(OUT/'validation-result.json').write_text(json.dumps({
    'blender_version':bpy.app.version_string,'status':'pass','normal_convention':'OpenGL; no green flip in Blender',
    'uv_layers':['PBRDetailUV','AtlasUV'],'reload':reload_results,
    'renders':['before-bake.png','after-bake.png'],
    'tolerance':'UV face area > 1e-10; render comparison is visual because Cycles bake filtering changes pixels.'
},indent=2),encoding='utf-8')
print('CHARACTER_PBR_BAKE_VALIDATION_PASS')
