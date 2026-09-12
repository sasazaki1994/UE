"""Rebuild the masked ritual warrior from the supplied front/side/back reference.

Blender 3.6.23: --background --python-exit-code 1 --python this_file.py
Coordinates remain metres, -Y forward, compatible with the existing 18-bone rig.
"""
import importlib.util
import math
import random
import sys
from pathlib import Path

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'Tools'))
spec = importlib.util.spec_from_file_location('maker', ROOT/'Tools/CreateCharacterModels.py')
m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
from PolishCharacterSurfaces import polish_materials
from ReferenceShirotsuraHead import build as build_head
from ReferenceShirotsuraCostume import build as build_costume


def smooth(ob, region=None):
    if ob.type == 'MESH':
        for p in ob.data.polygons:
            p.use_smooth = True
    if region:
        ob['rig_region'] = region
    return ob


def strand(name, points, radii, material, region=None, sides=10):
    return smooth(m.tube(name, points, radii, material, sides), region)


def ellipsoid(name, location, scale, material, region=None, sub=3):
    return smooth(m.ico(name, location, scale, material, sub), region)


def make_materials():
    # Names also communicate the surface role to the atlas/UE material pipeline.
    params = {
        'cloth': ('01 • indigo work cloth', (.021,.024,.027), .89, 0),
        'cloth_edge': ('02 • worn fold edges', (.061,.052,.041), .92, 0),
        'linen': ('03 • unbleached linen', (.42,.34,.245), .85, 0),
        'leather': ('04 • worn leather', (.018,.016,.014), .56, 0),
        'skin': ('05 • exposed right hand', (.24,.135,.080), .64, 0),
        'hair': ('06 • black hair', (.009,.008,.007), .43, 0),
        'mask': ('07 • aged whitewood mask', (.80,.775,.704), .53, 0),
        'red': ('08 • cinnabar cord', (.30,.009,.007), .65, 0),
        'root': ('09 • petrified corruption', (.013,.012,.011), .55, 0),
        'ember': ('10 • crimson fissure', (.65,.005,.001), .42, 0),
        'metal': ('11 • aged iron fittings', (.065,.051,.035), .40, .72),
        'steel': ('12 • boundary blade', (.34,.39,.40), .24, .88),
        'paper': ('13 • paper offerings', (.65,.51,.36), .93, 0),
        'gold': ('14 • bronze ritual bells', (.23,.135,.059), .34, .78),
    }
    mats = {key: m.material(name, color, rough, metal,
            emission=3.0 if key == 'ember' else 0)
            for key, (name, color, rough, metal) in params.items()}
    polish_materials(mats, True)
    # Subtle worn leather pores and varying roughness survive the atlas bake too.
    return mats


def spiral_wrap(name, center_bottom, center_top, radius_bottom, radius_top,
                width, turns, mat, region):
    a, b = Vector(center_bottom), Vector(center_top)
    tangent = (b-a).normalized()
    u = tangent.cross(Vector((0,1,0))).normalized()
    v = tangent.cross(u).normalized()
    steps = int(turns*40)
    verts = []
    for i in range(steps+1):
        t = i/steps
        c = a.lerp(b,t)
        radius = radius_bottom*(1-t)+radius_top*t
        angle = math.tau*turns*t
        for side in (-1,1):
            p = c+tangent*width*.5*side + (u*math.cos(angle)+v*math.sin(angle))*radius
            verts.append(p)
    ob = smooth(m.mesh(name, verts, [(2*i,2*i+1,2*i+3,2*i+2) for i in range(steps)], mat), region)
    mod = ob.modifiers.new('Overlapping cloth thickness', 'SOLIDIFY'); mod.thickness=.0016
    return ob


def legs_and_sandals(mats):
    for side, sign in (('R',-1),('L',1)):
        x = sign*.115
        # Cloth mass lies under the layered split robe; longitudinal gathers
        # break up the outline without detached crease wires.
        rings = []
        for i in range(33):
            t = i/32; z=.30+.60*t
            r=.066+.042*math.sin(math.pi*t)**.7
            rings.append((x*(1-.27*max(0,(t-.75)/.25)), .009, z, r, r*.91))
        ob = smooth(m.loft('Work_trouser_'+side, rings, mats['cloth'], 40, .10), 'leg_deform')
        ob['rig_side']=side
        smooth(m.loft('Gaiter_'+side,
            [(x,0,.075,.051,.055),(x,.009,.14,.048,.048),(x,.008,.23,.051,.053),
             (x,.01,.335,.071,.071)], mats['leather'], 32, .03), 'shin_'+side)
        spiral_wrap('Leg_wrap_'+side, (x,0,.097), (x,.010,.302), .054,.064,
                    .016, 3.4, mats['linen'],'shin_'+side)
        spiral_wrap('Leg_wrap_cross_'+side, (x,0,.30), (x,0,.105), .066,.055,
                    .008, 2.7, mats['linen'],'shin_'+side)
        spiral_wrap('Leg_wrap_top_'+side, (x,.009,.301), (x,.009,.336),
                    .070,.071,.018,1.6,mats['linen'],'shin_'+side)
        # Sandals have a thin layered sole, heel and individually modeled toes.
        sole = smooth(m.loft('Sandal_sole_'+side,
            [(x,-.036,.013,.076,.132),(x,-.036,.024,.080,.136),
             (x,-.036,.034,.077,.134)],mats['leather'],48), 'foot_'+side)
        ellipsoid('Sandal_foot_'+side,(x,-.023,.060),(.060,.099,.039),mats['leather'],'foot_'+side)
        for j in range(5):
            tx=x+sign*(.042-j*.020)
            length=.034-j*.002
            ellipsoid('Sandal_toe_'+side,(tx,-.125+j*.003,.051),
                      (.012 if j else .016,length,.020),mats['leather'],'foot_'+side)
        for offset in (-.035,.035):
            pts=[(x+offset*1.75,-.070,.046),(x+offset*.7,-.058,.098),
                 (x+offset*.2,-.123,.073)]
            ob=m.rope('Sandal_strap_'+side,pts,.0055,mats['linen'],12)
            # rope creates three objects, tag below by label.
        strand('Sandal_heel_strap_'+side,[(x-.068,.035,.051),(x-.043,.074,.085),
            (x+.043,.074,.085),(x+.068,.035,.051)],.007,mats['linen'],'foot_'+side)
        for j in range(16):
            angle=math.tau*j/16
            p=(x+.076*math.cos(angle),-.036+.133*math.sin(angle),.026)
            strand('Sandal_edge_stitch_'+side,[(p[0],p[1],.019),(p[0],p[1],.031)],
                   .0016,mats['linen'],'foot_'+side,6)


def hands_and_arms(mats):
    rng=random.Random(2147)
    for side,sign in (('R',-1),('L',1)):
        # Wrist and elbow centers match the production skeleton exactly.
        region='arm_deform' if side=='L' else 'forearm_R'
        mat=mats['root'] if side=='L' else mats['leather']
        strand('Corrupted_left_arm' if side=='L' else 'Right_forearm',
            [(sign*.230,0,1.265),(sign*.267,-.011,1.18),
             (sign*.289,-.018,1.10),(sign*.317,-.025,.984)],
            [.060,.050,.044,.027],mat,region,24)
        if side=='R':
            spiral_wrap('Wrist_bandage_upper',(-.290,-.017,1.10),(-.267,-.009,1.175),
                        .046,.052,.018,3.3,mats['linen'],'forearm_R')
            spiral_wrap('Wrist_leather_lacing',(-.317,-.025,1.001),(-.290,-.017,1.09),
                        .030,.043,.006,3.1,mats['linen'],'forearm_R')
        ellipsoid('Palm_'+side,(sign*.325,-.025,.956),(.036,.024,.042),mat,'hand_'+side)
        lengths=[.063,.073,.070,.055]
        for j,length in enumerate(lengths):
            x=sign*(.296+j*.019)
            z=.941-(.007 if j in (0,3) else 0)
            pts=[(x,-.026,z),(x+sign*.002,-.030,z-length*.37),
                 (x+sign*.003,-.045,z-length*.76),(x+sign*.001,-.056,z-length)]
            strand('Finger_'+side+str(j),pts,[.0105,.010,.008,.0058],mat,'hand_'+side,12)
            ellipsoid('Finger_knuckle_'+side+str(j),pts[1],(.011,.009,.009),mat,'hand_'+side,2)
            ellipsoid('Finger_plate_'+side+str(j),(x,-.045,z+.003),(.007,.003,.010),
                      mats['metal'] if side=='R' else mat,'hand_'+side,2)
        strand('Thumb_'+side,[(sign*.352,-.022,.975),(sign*.373,-.037,.955),
            (sign*.382,-.052,.932),(sign*.378,-.057,.919)],
            [.015,.012,.010,.006],mat,'hand_'+side,14)
        if side=='R':
            for j in range(4):
                x=-.294-j*.019
                strand('Palm_seam_'+str(j),[(x,-.047,.975),(x-.004,-.050,.952),
                    (x-.002,-.045,.939)],[.0012,.0012,.0007],mats['metal'],'hand_R',6)

    # Branch plates and hot fissures track the forearm. Most area remains black.
    for i in range(13):
        angle=math.tau*i/13
        pts=[]
        for j in range(9):
            t=j/8; z=1.245-.30*t
            center=Vector((.246+.080*t,-.005-.021*t,z))
            radius=.055*(1-t)+.030*t
            angle_t=angle+.16*math.sin(j*1.8+i)
            pts.append(center+Vector((radius*math.cos(angle_t),radius*math.sin(angle_t),0)))
        strand('Arm_black_root_'+str(i),pts,[.009,.012,.013,.010,.008,.010,.007,.005,.002],
               mats['root'],'arm_deform',9)
        if i in (2,6,8,10):
            # Offset onto the outward-facing ridge rather than underneath it.
            glowpts=[p+Vector((.003*math.cos(angle),.009*math.sin(angle),0)) for p in pts]
            strand('Arm_red_fissure_'+str(i),glowpts,
                   [.001,.003,.002,.0035,.002,.0025,.001,.002,.0005],mats['ember'],'arm_deform',7)
    for i in range(8):
        z=1.08+i*.020
        x=.30+(1.15-z)*.2
        root=Vector((x+.038,.018,z))
        top=root+Vector((.035+rng.random()*.045,.015+rng.random()*.026,.115+rng.random()*.075))
        mid=root.lerp(top,.5)+Vector((-.008,.006,-.012))
        strand('Root_spur_'+str(i),[root,mid,top], [.013,.007,.0008],mats['root'],'forearm_L',8)
        tip=mid+Vector((.025,.012,.048))
        strand('Root_spur_twig_'+str(i),[mid,mid.lerp(tip,.55),tip],
               [.0055,.0027,.0004],mats['root'],'forearm_L',7)
    for j in range(4):
        x=.298+j*.019
        strand('Palm_fissure_'+str(j),[(x,-.050,.973),(x+.008,-.054,.948),
            (x+.003,-.050,.922)],[.0012,.0018,.0005],mats['ember'],'hand_L',6)


def sword(mats):
    start=Vector((-.323,-.055,.989)); end=Vector((-.79,-.093,.20))
    direction=(end-start).normalized(); normal=Vector((direction.z,0,-direction.x)).normalized()
    grip_start=start-direction*.145
    strand('Katana_grip',[grip_start,start],[.013,.013],mats['leather'],'weapon',16)
    for i in range(10):
        p=grip_start.lerp(start,(i+.5)/10)
        strand('Grip_diamond_wrap',[p-normal*.013-direction*.008,
            p+Vector((0,-.014,0)),p+normal*.013+direction*.008],.003,mats['linen'],'weapon',6)
    # Oval tsuba in the plane perpendicular to the blade.
    ring=[start+normal*.036*math.cos(math.tau*i/32)+Vector((0,.025*math.sin(math.tau*i/32),0)) for i in range(33)]
    strand('Plain_guard',ring,.005,mats['gold'],'weapon',9)
    strand('Blade_habaki',[start,start+direction*.024],[.019,.018],mats['gold'],'weapon',12)
    verts=[]
    for i in range(41):
        t=i/40; c=start.lerp(end,t)+normal*(.023*t*t)
        width=.014*(1-.30*t) if i<40 else .0003
        verts.extend((c-normal*width,c+Vector((0,-.003,0)),c+normal*width,c+Vector((0,.002,0))))
    ob=m.mesh('Sakaidachi_blade',verts,
        [(i*4+j,i*4+(j+1)%4,(i+1)*4+(j+1)%4,(i+1)*4+j) for i in range(40) for j in range(4)],mats['steel'])
    ob['rig_region']='weapon'
    # Restrained red etching echoes the reference sword along one blade bevel.
    for i in range(12):
        t=.10+i*.061; c=start.lerp(end,t)+normal*(.023*t*t)+Vector((0,-.0034,0))
        strand('Blade_ritual_etching',[c+normal*.006,c+direction*.012,c-normal*.003+direction*.018],
               .0008,mats['red'],'weapon',6)
    strand('Sakaidachi_scabbard',[(.15,.085,.94),(.25,.115,.68),(.37,.135,.35)],
           [.018,.017,.014],mats['leather'],'pelvis',16)


def build():
    m.setup(); bpy.context.preferences.filepaths.save_version=0
    random.seed(12092026)
    mats=make_materials()
    legs_and_sandals(mats)
    hands_and_arms(mats)
    build_costume(m,mats)
    build_head(m,mats)
    sword(mats)
    objects=[ob for ob in bpy.context.scene.objects if ob.type in ('MESH','CURVE')]
    m.export('Shirotsura',objects,ROOT/'Art/Characters/Shirotsura')
    scene=bpy.context.scene
    scene['reference_rebuild']='User supplied front and four-view character reference, 2026-09-12'
    scene.cycles.samples=48
    cam=m.stage(1,(0,0,.90),(2.5,-6,2.3),2.08)
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Art/Characters/Shirotsura/Shirotsura.blend'))
    m.render(ROOT/'Art/Characters/Previews/Shirotsura.png')
    m.render(ROOT/'Art/Characters/Previews/Shirotsura_Back.png',cam,(2.5,6,2.2),(0,0,.90),2.08)
    # A real orthographic turnaround of the mesh, not image-generation output.
    for label,position in [('Front',(0,-6,.9)),('Left',(6,0,.9)),('Back',(0,6,.9)),('Right',(-6,0,.9))]:
        m.render(ROOT/('Art/Characters/Previews/Shirotsura_Reference'+label+'.png'),
                 cam,position,(0,0,.9),2.04)
    print('REFERENCE_SHIROTSURA_BUILD_PASS')


if __name__ == '__main__':
    build()
