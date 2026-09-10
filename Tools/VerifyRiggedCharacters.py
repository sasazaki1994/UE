"""Check exported skinning/actions and render the actual animated meshes."""
import bpy
import struct
import json
import importlib.util
from pathlib import Path

root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('maker',root/'Tools/CreateCharacterModels.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
report=[]
for name,clip,frame in [('Shirotsura','Climb',10),('Ishibashiri','Walk',13)]:
    folder=root/'Art/Characters'/name/'Rigged'
    data=(folder/(name+'_Animated.glb')).read_bytes()
    count=struct.unpack_from('<I',data,12)[0]
    doc=json.loads(data[20:20+count])
    assert doc.get('skins') and doc.get('animations'),name+': skin/animation missing in GLB'
    bpy.ops.wm.open_mainfile(filepath=str(folder/(name+'_Rigged.blend')))
    rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    body=next(o for o in bpy.context.scene.objects if o.type=='MESH')
    rig.animation_data.action=bpy.data.actions[clip]
    scene=bpy.context.scene
    scene.frame_set(1)
    first=[v.co.copy() for v in body.evaluated_get(bpy.context.evaluated_depsgraph_get()).data.vertices]
    scene.frame_set(frame)
    second=body.evaluated_get(bpy.context.evaluated_depsgraph_get()).data.vertices
    moved=sum((a-v.co).length>.0001 for a,v in zip(first,second))
    assert moved>100,name+': bone animation does not deform skin'
    report.append({'asset':name,'glb_skin_count':len(doc['skins']),
        'glb_animations':[a.get('name') for a in doc['animations']], 'deformed_vertices':moved})
    scene.render.engine='CYCLES';scene.cycles.samples=32
    if name=='Shirotsura':m.stage(1,(0,0,.9),(2.8,-4,2),2.1)
    else:m.stage(7,(0,-.2,4.2),(16,-23,14),18.4)
    scene.render.resolution_x=1400;scene.render.resolution_y=1400;scene.render.resolution_percentage=100
    m.render(root/'Art/Characters/Previews'/(name+'_Rigged.png'))
(root/'Art/Characters/rig-validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('RIG_ANIMATION_VALIDATION_PASS')
