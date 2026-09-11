"""Build deforming skeletons and authored in-place animation clips, Blender 3.6."""
import bpy
import math
import json
import importlib.util
import sys
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]/'Art'/'Characters'
PBR_SPEC=importlib.util.spec_from_file_location('character_pbr',Path(__file__).with_name('CharacterPBR.py'))
PBR=importlib.util.module_from_spec(PBR_SPEC);PBR_SPEC.loader.exec_module(PBR)
PBR_MODE='fallback'
if '--pbr-mode' in sys.argv:
    PBR_MODE=sys.argv[sys.argv.index('--pbr-mode')+1]

def bone(name,head,tail,parent=None):
    b=bpy.context.object.data.edit_bones.new(name)
    b.head=head; b.tail=tail
    if parent: b.parent=bpy.context.object.data.edit_bones[parent]

def build(name):
    bpy.ops.wm.open_mainfile(filepath=str(ROOT/name/(name+'.blend')))
    bpy.context.preferences.filepaths.save_version=0
    bpy.ops.object.select_all(action='DESELECT')
    for ob in list(bpy.context.scene.objects):
        if ob.name.startswith('STUDIO_'): bpy.data.objects.remove(ob,do_unlink=True)
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    bpy.ops.object.armature_add(enter_editmode=True)
    rig=bpy.context.object; rig.name=name+'_Rig'
    rig.data.edit_bones.remove(rig.data.edit_bones[0])
    bone('root',(0,0,0),(0,0,.18))
    hero=name=='Shirotsura'
    if hero:
        bone('pelvis',(0,0,.86),(0,0,1.04),'root')
        bone('spine',(0,0,1.04),(0,0,1.25),'pelvis')
        bone('chest',(0,0,1.25),(0,0,1.40),'spine')
        bone('head',(0,0,1.40),(0,0,1.70),'chest')
        for s,x in [('R',-.115),('L',.115)]:
            sign=-1 if s=='R' else 1
            bone('thigh_'+s,(x,0,.86),(x,0,.49),'pelvis')
            bone('shin_'+s,(x,0,.49),(x,0,.10),'thigh_'+s)
            bone('foot_'+s,(x,0,.10),(x,-.12,.05),'shin_'+s)
            bone('upperarm_'+s,(sign*.19,0,1.32),(sign*.274,-.01,1.15),'chest')
            bone('forearm_'+s,(sign*.274,-.01,1.15),(sign*.317,-.025,.984),'upperarm_'+s)
            bone('hand_'+s,(sign*.317,-.025,.984),(sign*.325,-.025,.91),'forearm_'+s)
        bone('weapon',(-.323,-.055,.989),(-.80,-.10,.22),'hand_R')
    else:
        bone('back',(0,.8,4.1),(0,.8,5.5),'root')
        bone('head',(0,-2.8,3.9),(0,-5.3,2.6),'back')
        bone('jaw',(0,-4.4,2.2),(0,-5.8,2.1),'head')
        for s,sign in [('L',1),('R',-1)]:
            for pos,y in [('front',-1.97),('rear',3.28)]:
                x=sign*(1.69 if pos=='front' else 1.59)
                bone(pos+'_thigh_'+s,(x,y,3.5),(x+sign*.18,y-.18,1.83),'back')
                bone(pos+'_shin_'+s,(x+sign*.18,y-.18,1.83),(x+sign*.27,y-.26,.58),pos+'_thigh_'+s)
                bone(pos+'_foot_'+s,(x+sign*.27,y-.26,.58),(x+sign*.27,y-.65,.3),pos+'_shin_'+s)
        bone('tail',(0,4.92,3.52),(.26,5.67,2.86),'back')
        for i,p in enumerate([(2.18,-1.55,6.11),(0,.60,7.84),(-1.22,3.05,6.88)]):
            bone('core_'+str(i),p,Vector(p)+Vector((0,0,.4)),'back')
    bpy.ops.object.mode_set(mode='OBJECT')
    rig.show_in_front=True
    for ob in meshes:
        label=ob.name.lower()
        center=ob.matrix_world@Vector((0,0,0))
        if ob.data.vertices:
            center=sum((ob.matrix_world@v.co for v in ob.data.vertices),Vector())/len(ob.data.vertices)
        side='L' if center.x>0 else 'R'
        default='chest' if hero else 'back'
        if hero:
            if any(k in label for k in ['hair','mask','eye_slit','quiet_mouth','vermilion']): default='head'
            elif any(k in label for k in ['blade','grip','plain_guard']): default='weapon'
            elif 'scabbard' in label or any(k in label for k in ['waist','utility_pouch','jacket_split','rear_knot']): default='pelvis'
            elif 'paper_talisman' in label or 'talisman_ink' in label: default='chest' if center.z>1.05 else 'pelvis'
            elif 'indigo_visible_repair' in label or 'repair_stitch' in label: default='chest' if center.z>1.02 else 'pelvis'
            elif 'straw_mantle' in label or 'shoulder_straw' in label: default='chest'
            elif any(k in label for k in ['boot','sandal']): default='foot_'+side
            elif any(k in label for k in ['gaiter','leg_wrap']): default='shin_'+side
            elif 'finger' in label or 'thumb' in label or 'palm' in label or 'right_hand' in label: default='hand_'+side
            elif 'sleeve' in label: default='upperarm_'+side
            elif 'wrist' in label or 'right_forearm' in label: default='forearm_R'
            elif any(k in label for k in ['corrupted_left_arm','arm_black','arm_red','root_spur','forearm_fissure']): default='arm_deform'
            elif 'trouser' in label: default='leg_deform'
            elif label=='work_jacket': default='torso_deform'
        else:
            if any(k in label for k in ['head','muzzle','nose','nostril','brow','eye','ear','tusk','cheek','prayer']): default='head'
            elif 'small_tail' in label: default='tail'
            elif any(k in label for k in ['leg_','hoof','foreleg_grab']): default='leg_deform'
            elif any(k in label for k in ['magane','redblack_crystal','core_emissive','spreading_black','root_crimson']):
                cores=[Vector(p) for p in [(2.18,-1.55,6.11),(0,.60,7.84),(-1.22,3.05,6.88)]]
                default='core_'+str(min(range(3),key=lambda i:(center-cores[i]).length))
        groups={}
        def weight(index,bname,w):
            if w<=0: return
            if bname not in groups: groups[bname]=ob.vertex_groups.new(name=bname)
            groups[bname].add([index],w,'REPLACE')
        for v in ob.data.vertices:
            p=ob.matrix_world@v.co
            if default=='arm_deform':
                w=max(0,min(1,(p.z-1.10)/.10))
                weight(v.index,'upperarm_L',w); weight(v.index,'forearm_L',1-w)
            elif default=='torso_deform':
                w=max(0,min(1,(p.z-1.02)/.26))
                weight(v.index,'spine',1-w); weight(v.index,'chest',w)
            elif default=='leg_deform':
                if hero:
                    w=max(0,min(1,(p.z-.42)/.15))
                    weight(v.index,'thigh_'+side,w); weight(v.index,'shin_'+side,1-w)
                else:
                    pos='front' if center.y<0 else 'rear'
                    if 'hoof' in label: weight(v.index,pos+'_foot_'+side,1)
                    else:
                        w=max(0,min(1,(p.z-1.55)/.60))
                        weight(v.index,pos+'_thigh_'+side,w); weight(v.index,pos+'_shin_'+side,1-w)
            else: weight(v.index,default,1)
        ob.parent=rig
        modifier=ob.modifiers.new('Skeleton deformation','ARMATURE'); modifier.object=rig
    bpy.ops.object.select_all(action='DESELECT')
    for ob in meshes: ob.select_set(True)
    bpy.context.view_layer.objects.active=meshes[0]
    bpy.ops.object.join()
    body=bpy.context.object; body.name='SK_'+name
    # Merge duplicate material slots so UE uses one section per material.
    mats=[]; indices={}
    for slot in body.material_slots:
        if slot.material not in mats: mats.append(slot.material)
        indices[len(indices)]=mats.index(slot.material)
    assignments=[indices[p.material_index] for p in body.data.polygons]
    body.data.materials.clear()
    for mat in mats: body.data.materials.append(mat)
    for p,idx in zip(body.data.polygons,assignments): p.material_index=idx
    return rig,body

def key(rig,frame,rotations,locations=None):
    for p in rig.pose.bones:
        p.rotation_mode='XYZ'
        p.rotation_euler=[math.radians(v) for v in rotations.get(p.name,(0,0,0))]
        p.location=(locations or {}).get(p.name,(0,0,0))
        p.keyframe_insert('rotation_euler',frame=frame,group=p.name)
        p.keyframe_insert('location',frame=frame,group=p.name)

def make_actions(rig,hero):
    clips={}
    names={'Idle':(60,True),'Walk':(30,True),'Run':(22,True),'Slash':(16,False),'Dodge':(18,False),
       'Climb':(36,True),'Hang':(60,True),'Grip':(30,True),'Jump':(24,True),'Death':(48,False)} if hero else {
       'Idle':(60,True),'Walk':(48,True),'Charge':(24,True),'Buck':(36,True),'Calmed':(60,False)}
    for name,(frames,loop) in names.items():
        action=bpy.data.actions.new(name); action.use_fake_user=True
        rig.animation_data_create(); rig.animation_data.action=action
        for f in range(1,frames+2):
            t=(f-1)/frames; a=2*math.pi*t; s=math.sin(a); c=math.cos(a)
            r={}; loc={}
            if hero:
                if name=='Idle':
                    r={'chest':(1.3*s,0,.6*s),'head':(-.8*s,0,0)}
                elif name in ['Walk','Run']:
                    amp=24 if name=='Walk' else 42
                    r={'thigh_L':(amp*s,0,0),'thigh_R':(-amp*s,0,0),
                       'shin_L':(-max(0,-s)*amp*1.25,0,0),'shin_R':(-max(0,s)*amp*1.25,0,0),
                       'upperarm_L':(-amp*.65*s,0,0),'upperarm_R':(amp*.65*s,0,0),
                       'forearm_L':(-15,0,0),'forearm_R':(-20,0,0),'chest':(5 if name=='Run' else 1,0,3*s)}
                    loc={'pelvis':(0,.012*math.cos(a*2),0)}
                elif name=='Slash':
                    swing=math.sin(pi_clamp(t)*math.pi)
                    r={'upperarm_R':(-80*swing,0,-50*swing),'forearm_R':(-45*swing,0,20*swing),
                       'chest':(0,0,35*math.sin(a)), 'upperarm_L':(-25*swing,0,15*swing)}
                elif name=='Dodge':
                    k=math.sin(t*math.pi)
                    r={'pelvis':(28*k,0,0),'thigh_L':(38*k,0,0),'thigh_R':(38*k,0,0),
                       'shin_L':(-65*k,0,0),'shin_R':(-65*k,0,0),'upperarm_L':(-45*k,0,0),'upperarm_R':(-45*k,0,0)}
                    loc={'pelvis':(0,-.13*k,0)}
                elif name in ['Climb','Hang','Grip']:
                    cycle=s if name=='Climb' else .08*s
                    r={'upperarm_L':(-138+23*cycle,0,-9),'upperarm_R':(-138-23*cycle,0,9),
                       'forearm_L':(-28-20*cycle,0,0),'forearm_R':(-28+20*cycle,0,0),
                       'thigh_L':(30+25*cycle,0,0),'thigh_R':(30-25*cycle,0,0),
                       'shin_L':(-55-20*cycle,0,0),'shin_R':(-55+20*cycle,0,0),'head':(-10,0,0)}
                    if name=='Grip': r['chest']=(6*s,0,4*s)
                elif name=='Jump':
                    r={'thigh_L':(25,0,0),'thigh_R':(5,0,0),'shin_L':(-55,0,0),'shin_R':(-30,0,0),
                       'upperarm_L':(-70,0,0),'upperarm_R':(-55,0,0)}
                elif name=='Death':
                    k=min(1,t*1.6)
                    r={'pelvis':(80*k,0,15*k),'shin_L':(-40*k,0,0),'shin_R':(-45*k,0,0),'head':(15*k,0,0)}
                    loc={'pelvis':(0,-.55*k,0)}
            else:
                if name in ['Walk','Charge']:
                    amp=17 if name=='Walk' else 29
                    for side,sign in [('L',1),('R',-1)]:
                        for pos,phase in [('front',1),('rear',-1)]:
                            v=s*sign*phase
                            r[pos+'_thigh_'+side]=(amp*v,0,0)
                            r[pos+'_shin_'+side]=(-amp*max(0,-v)*.8,0,0)
                    r['head']=(0,2*s,0)
                elif name=='Buck':
                    r['head']=(12*s,0,14*s)
                    for side in ['L','R']:
                        r['front_thigh_'+side]=(16*s,0,0)
                        r['front_shin_'+side]=(-18*abs(s),0,0)
                elif name=='Calmed':
                    k=min(1,t*1.7)
                    r['head']=(0,12*k,0)
                    for side in ['L','R']:
                        for pos in ['front','rear']:
                            r[pos+'_thigh_'+side]=(15*k,0,0); r[pos+'_shin_'+side]=(-30*k,0,0)
                else: r={'head':(0,1.5*s,0),'tail':(0,0,6*s)}
            key(rig,f,r,loc)
        for fc in action.fcurves:
            for point in fc.keyframe_points: point.interpolation='LINEAR'
        clips[name]={'frames':frames,'seconds':frames/30,'loop':loop}
    return clips

def pi_clamp(t): return max(0,min(1,t))

def _uv_has_area(mesh, layer, epsilon=1e-10):
    """Check the rendered triangles; signed n-gon areas can cancel on twists."""
    mesh.calc_loop_triangles()
    for triangle in mesh.loop_triangles:
        if triangle.area <= epsilon: continue
        points=[layer.data[index].uv for index in triangle.loops]
        area=abs((points[1].x-points[0].x)*(points[2].y-points[0].y)-
                 (points[2].x-points[0].x)*(points[1].y-points[0].y))*.5
        # UVs are dimensionless: the world-space degeneracy threshold would
        # reject valid sub-texel bevels on the 12 m creature. Still reject collapse.
        if not math.isfinite(area) or area <= 1e-16: return False
    return bool(mesh.polygons)

def _copy_uv(mesh, source, name):
    source_name=source.name
    target=mesh.uv_layers.get(name) or mesh.uv_layers.new(name=name)
    source=mesh.uv_layers[source_name]
    for src,dst in zip(source.data,target.data): dst.uv=src.uv
    return target

def ensure_bake_uvs(body):
    """Create independent source/detail and destination/atlas UV sets."""
    mesh=body.data
    # Project the same triangles that will be baked/exported. Non-planar n-gons
    # can otherwise project an individual bevel triangle exactly edge-on.
    if any(len(p.vertices)>3 for p in mesh.polygons):
        bpy.context.view_layer.objects.active=body
        modifier=body.modifiers.new('Bake triangulation','TRIANGULATE')
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    detail=mesh.uv_layers.get('PBRDetailUV')
    if not detail or not _uv_has_area(mesh,detail):
        source=next((uv for uv in mesh.uv_layers
                     if uv.name not in {'PBRDetailUV','AtlasUV'} and _uv_has_area(mesh,uv)),None)
        if source:
            detail=_copy_uv(mesh,source,'PBRDetailUV')
        else:
            detail=detail or mesh.uv_layers.new(name='PBRDetailUV')
            mesh.uv_layers.active=detail
            bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
            bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.002)
            bpy.ops.object.mode_set(mode='OBJECT')
    atlas=mesh.uv_layers.get('AtlasUV') or mesh.uv_layers.new(name='AtlasUV')
    mesh.uv_layers.active=atlas
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.002)
    bpy.ops.object.mode_set(mode='OBJECT')
    # Edit-mode round trips and adding UV layers invalidate Blender RNA handles.
    # Reacquire both layers before reading their per-loop coordinates.
    detail=mesh.uv_layers['PBRDetailUV']
    atlas=mesh.uv_layers['AtlasUV']
    atlas.active_render=True
    assert _uv_has_area(mesh,detail),'PBRDetailUV contains missing/collapsed faces'
    assert _uv_has_area(mesh,atlas),'AtlasUV contains missing/collapsed faces'
    return detail,atlas

def make_atlas_export_uv0(mesh):
    """Put AtlasUV in FBX/UE channel 0 while retaining the detail UV by name."""
    atlas=mesh.uv_layers.get('AtlasUV');detail=mesh.uv_layers.get('PBRDetailUV')
    assert atlas and detail
    if mesh.uv_layers[0] == atlas:
        atlas.active_render=True;return
    saved={name:[tuple(item.uv) for item in layer.data]
           for name,layer in (('AtlasUV',atlas),('PBRDetailUV',detail))}
    while mesh.uv_layers: mesh.uv_layers.remove(mesh.uv_layers[0])
    for name in ('AtlasUV','PBRDetailUV'):
        layer=mesh.uv_layers.new(name=name)
        for coords,item in zip(saved[name],layer.data): item.uv=coords
    mesh.uv_layers.get('AtlasUV').active_render=True

def bake_surface(body,out,name,manifest_path=PBR.MANIFEST,asset_root=PBR.ROOT):
    # Never rename an authored UV. Source sampling and bake destination are explicit.
    detail,atlas=ensure_bake_uvs(body)
    pbr_result=PBR.apply_external_pbr(body.data.materials,bpy,character=name,mode=PBR_MODE,
                                      manifest_path=manifest_path,root=asset_root)
    print('EXTERNAL_PBR_'+pbr_result['status'].upper(),name,pbr_result['reason'] or '',pbr_result['role_counts'])
    bpy.ops.object.select_all(action='DESELECT');body.select_set(True)
    bpy.context.view_layer.objects.active=body
    body.data.uv_layers.active=atlas
    scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=1
    scene.render.bake.use_pass_direct=False;scene.render.bake.use_pass_indirect=False
    scene.render.bake.use_pass_color=True;scene.render.bake.margin=8
    images={}
    for kind in ['BaseColor','Normal','Roughness']:
        image=bpy.data.images.get('T_'+name+'_'+kind) or bpy.data.images.new(
            'T_'+name+'_'+kind,width=2048,height=2048,alpha=False)
        if kind!='BaseColor':image.colorspace_settings.name='Non-Color'
        for mat in body.data.materials:
            nt=mat.node_tree
            node=nt.nodes.get('BakeTarget_'+kind) or nt.nodes.new('ShaderNodeTexImage')
            node.image=image;node.name='BakeTarget_'+kind
            for n in nt.nodes:n.select=False
            node.select=True;nt.nodes.active=node
        bpy.ops.object.bake(type=('DIFFUSE' if kind=='BaseColor' else kind.upper()))
        image.filepath_raw=str(out/(image.name+'.png'));image.file_format='PNG';image.save();image.pack()
        images[kind]=image
    for mat in body.data.materials:
        nt=mat.node_tree;p=nt.nodes.get('Principled BSDF')
        col=nt.nodes.get('BakeTarget_BaseColor');nor=nt.nodes.get('BakeTarget_Normal');rough=nt.nodes.get('BakeTarget_Roughness')
        uv=nt.nodes.get('BakedPBR_AtlasUV') or nt.nodes.new('ShaderNodeUVMap');uv.name='BakedPBR_AtlasUV';uv.uv_map='AtlasUV'
        for node in (col,nor,rough): nt.links.new(uv.outputs['UV'],node.inputs['Vector'])
        nt.links.new(col.outputs['Color'],p.inputs['Base Color'])
        normal=nt.nodes.get('BakedPBR_Normal') or nt.nodes.new('ShaderNodeNormalMap');normal.name='BakedPBR_Normal';normal.uv_map='AtlasUV';normal.space='TANGENT'
        nt.links.new(nor.outputs['Color'],normal.inputs['Color'])
        nt.links.new(normal.outputs['Normal'],p.inputs['Normal'])
        nt.links.new(rough.outputs['Color'],p.inputs['Roughness'])
    make_atlas_export_uv0(body.data)
    print('SURFACE_BAKE_PASS',name)
    return pbr_result

def export(name):
    rig,body=build(name)
    out=ROOT/name/'Rigged'; out.mkdir(exist_ok=True)
    pbr_result=bake_surface(body,out,name)
    clips=make_actions(rig,name=='Shirotsura')
    scene=bpy.context.scene; scene.render.fps=30
    bpy.ops.object.select_all(action='DESELECT'); body.select_set(True); rig.select_set(True)
    bpy.context.view_layer.objects.active=rig
    rig.animation_data.action=None
    key(rig,1,{})
    rig.animation_data_clear()
    bpy.ops.export_scene.fbx(filepath=str(out/('SK_'+name+'.fbx')),use_selection=True,
       object_types={'MESH','ARMATURE'},axis_forward='-Y',axis_up='Z',add_leaf_bones=False,
       bake_anim=False,armature_nodetype='NULL')
    for clip,meta in clips.items():
        rig.animation_data_create(); rig.animation_data.action=bpy.data.actions[clip]
        scene.frame_start=1; scene.frame_end=meta['frames']+1
        scene.frame_set(1)
        bpy.ops.export_scene.fbx(filepath=str(out/('AN_'+name+'_'+clip+'.fbx')),use_selection=True,
          object_types={'MESH','ARMATURE'},axis_forward='-Y',axis_up='Z',add_leaf_bones=False,
          bake_anim=True,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,
          bake_anim_simplify_factor=0,armature_nodetype='NULL')
    rig.animation_data.action=bpy.data.actions['Idle']; scene.frame_end=61; scene.frame_set(1)
    # Store actions as NLA tracks for GLB animation export, muted in the editing file.
    for clip,meta in clips.items():
        track=rig.animation_data.nla_tracks.new(); track.name=clip
        track.strips.new(clip,1,bpy.data.actions[clip]); track.mute=True
    bpy.ops.export_scene.gltf(filepath=str(out/(name+'_Animated.glb')),use_selection=True,export_format='GLB',
       export_animations=True,export_nla_strips=True,export_force_sampling=True)
    for area in bpy.context.screen.areas:
        if area.type=='VIEW_3D':
            area.spaces.active.region_3d.view_distance=3 if name=='Shirotsura' else 19
            area.spaces.active.region_3d.view_location=Vector((0,0,.9 if name=='Shirotsura' else 4))
            area.spaces.active.region_3d.view_perspective='PERSP'
            area.spaces.active.shading.color_type='MATERIAL'
    bpy.ops.wm.save_as_mainfile(filepath=str(out/(name+'_Rigged.blend')))
    unweighted=[v.index for v in body.data.vertices if abs(sum(g.weight for g in v.groups)-1)>0.001]
    assert not unweighted, 'Unweighted or unnormalized vertices'
    report={'bone_count':len(rig.data.bones),'vertices':len(body.data.vertices),'weight_validation':'pass',
        'animations':clips,'root_motion':False,'rig_style':'deformation skeleton; procedural in-place clips; no facial rig or IK',
        'external_pbr':pbr_result}
    (out/'rig-info.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('RIG_EXPORT_PASS',name,len(rig.data.bones))

if __name__ == '__main__':
    export('Shirotsura')
    export('Ishibashiri')
