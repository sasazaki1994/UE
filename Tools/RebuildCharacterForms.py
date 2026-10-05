"""Independent unskinned form-review candidates. Blender 3.6.23; never adopts/imports.
Run: blender -b --factory-startup --disable-autoexec --python-exit-code 2
     --python Tools/RebuildCharacterForms.py -- [--build-only] [--samples 24]
All geometry is newly authored here; old rigged geometry is comparison only.
"""
import argparse
import bpy
import hashlib
import json
import math
import re
import sys
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'Art/Characters/ProductionCandidates/FormReview01'
ARGS = argparse.ArgumentParser()
ARGS.add_argument('--build-only', action='store_true')
ARGS.add_argument('--samples', type=int, default=24)
ARGS.add_argument('--character', choices=['Shirotsura','Ishibashiri'])
args = ARGS.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
TAU = math.tau


def material(name, grey):
    m = bpy.data.materials.new(name)
    m.diffuse_color = (*grey, 1)
    m.use_nodes = True
    p = m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value = (*grey, 1)
    p.inputs['Roughness'].default_value = .78
    return m


def finish(ob, mat, region=None):
    ob.data.materials.clear()
    ob.data.materials.append(mat)
    if region:
        ob['proposed_weight_region'] = region
    ob['stage'] = 'UNSKINNED_FORM_REVIEW; NOT_ADOPTED'
    return ob


def mesh(name, verts, faces, mat, region=None, smooth=True):
    data = bpy.data.meshes.new(name)
    data.from_pydata(verts, [], faces)
    data.update()
    ob = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(ob)
    finish(ob, mat, region)
    for p in data.polygons:
        p.use_smooth = smooth
    return ob


def sub(ob, levels=2):
    mod = ob.modifiers.new('Editable large surfaces', 'SUBSURF')
    mod.levels = levels
    mod.render_levels = levels
    return ob


def solid(ob, thickness):
    mod = ob.modifiers.new('Real cloth / shell thickness', 'SOLIDIFY')
    mod.thickness = thickness
    mod.offset = 0
    return ob


def ellipsoid(name, loc, scale, mat, region=None):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=32, ring_count=20, location=loc)
    ob = bpy.context.object
    ob.name = name
    ob.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    finish(ob, mat, region)
    for p in ob.data.polygons:
        p.use_smooth = True
    return ob


def tube(name, pts, radii, mat, region=None, sides=16, subdiv=2):
    # Endpoint support rings prevent closed subdivision caps from shrinking joints.
    pts = list(pts)
    radii = list(radii) if isinstance(radii, (tuple,list)) else [radii]*len(pts)
    if subdiv:
        pts = [pts[0], Vector(pts[0]).lerp(Vector(pts[1]), .035), *pts[1:-1],
               Vector(pts[-1]).lerp(Vector(pts[-2]), .035), pts[-1]]
        radii = [radii[0], radii[0], *radii[1:-1], radii[-1], radii[-1]]
    vs = []
    for i, pt in enumerate(pts):
        p = Vector(pt)
        tangent = Vector(pts[min(i+1, len(pts)-1)])-Vector(pts[max(0, i-1)])
        tangent.normalize()
        ref = Vector((0,1,0)) if abs(tangent.y) < .9 else Vector((0,0,1))
        u = tangent.cross(ref).normalized()
        v = tangent.cross(u).normalized()
        radius = radii[i] if isinstance(radii, (tuple,list)) else radii
        rx, ry = radius if isinstance(radius, (tuple,list)) else (radius,radius)
        for j in range(sides):
            a = j*TAU/sides
            vs.append(p+u*rx*math.cos(a)+v*ry*math.sin(a))
    fs = [(i*sides+j, i*sides+(j+1)%sides, (i+1)*sides+(j+1)%sides, (i+1)*sides+j)
          for i in range(len(pts)-1) for j in range(sides)]
    fs += [tuple(reversed(range(sides))), tuple((len(pts)-1)*sides+j for j in range(sides))]
    ob = mesh(name, vs, fs, mat, region)
    return sub(ob, subdiv) if subdiv else ob


def vertical_loft(name, rings, mat, region, folds=0, sides=40):
    # x/y/z centers and anatomical ellipse radii. Folds are broad hanging masses.
    vs = []
    for k, (x,y,z,rx,ry) in enumerate(rings):
        for j in range(sides):
            a = TAU*j/sides
            bulge = folds * (.65*math.cos(6*a+.16*k)+.35*math.cos(9*a-.1*k))
            vs.append((x+(rx+bulge)*math.cos(a), y+(ry+bulge)*math.sin(a), z))
    fs = [(k*sides+j,k*sides+(j+1)%sides,(k+1)*sides+(j+1)%sides,(k+1)*sides+j)
          for k in range(len(rings)-1) for j in range(sides)]
    return sub(solid(mesh(name,vs,fs,mat,region), .006),2)


def cloth_panel(name, rows, mat, region, fold=.008, thickness=.006):
    vs=[]
    n=14
    for k,(x1,x2,y,z) in enumerate(rows):
        for j in range(n):
            t=j/(n-1)
            x=x1+(x2-x1)*t
            drape_y=y
            if name.startswith('SplitCoat'):
                drape_y=y*math.sqrt(max(.08,1-(x/.23)**2))
            vs.append((x, drape_y-fold*math.sin(t*math.pi)*math.cos(t*3*math.pi+.25*k),
                       z-.008*math.sin(t*math.pi)))
    fs=[(k*n+j,k*n+j+1,(k+1)*n+j+1,(k+1)*n+j) for k in range(len(rows)-1) for j in range(n-1)]
    return sub(solid(mesh(name,vs,fs,mat,region),thickness),2)


def shirotsura():
    body=vertical_loft('Anatomy_Torso',[(0,0,.85,.14,.10),(0,0,.93,.14,.105),
        (0,0,1.08,.132,.088),(0,0,1.23,.17,.105),(0,.008,1.36,.193,.10),
        (0,.012,1.405,.145,.075)],GREY,'torso_deform')
    ellipsoid('Anatomy_Neck',(0,.007,1.442),(.047,.046,.075),GREY,'head')
    ellipsoid('Anatomy_Head',(0,.009,1.595),(.079,.087,.122),GREY,'head')
    # A quiet mask: rounded temples, chin taper and continuous carved nose plane.
    vs=[]; rows=23; cols=25
    for i in range(rows):
        t=i/(rows-1); z=1.477+.228*t
        width=.073*(.64+.36*math.sin(math.pi*t)**.7)
        for j in range(cols):
            u=2*j/(cols-1)-1
            nose=.020*math.exp(-(u/.20)**2)*math.exp(-((t-.46)/.19)**2)
            vs.append((u*width,-.079-.035*(1-u*u)-nose,z))
    fs=[(i*cols+j,i*cols+j+1,(i+1)*cols+j+1,(i+1)*cols+j) for i in range(rows-1) for j in range(cols-1)]
    solid(mesh('White_Mask_CarvedPlanes',vs,fs,GREY,'head'),.009)
    for sign in [-1,1]:
        tube('Mask_EyeRecess',[(sign*.016,-.118,1.605),(sign*.043,-.108,1.61),(sign*.061,-.096,1.609)],
             [.0024,.0024,.0014],DARK,'head',8,1)
    tube('Mask_MouthRecess',[(-.016,-.106,1.514),(0,-.11,1.512),(.016,-.106,1.514)],.0016,DARK,'head',8,1)
    ellipsoid('Hair_Cap',(0,.039,1.607),(.083,.078,.113),GREY,'head')
    ellipsoid('Hair_RearKnot',(0,.102,1.663),(.043,.038,.04),GREY,'head')
    for sign,side in [(-1,'R'),(1,'L')]:
        # Correct shoulder-to-wrist lengths (~.29 + .235m); hands reach upper thigh.
        shoulder=(sign*.183,0,1.366); elbow=(sign*.279,-.012,1.10)
        wrist=(sign*.334,-.027,.876)
        tube('Anatomy_Arm_'+side,[shoulder,(sign*.235,.0,1.26),elbow,(sign*.314,-.02,.98),wrist],
             [.071,.061,.043,.047,.030],GREY,'arm_deform' if side=='L' else 'upperarm_R')
        ellipsoid('WristTransition_'+side,(sign*.334,-.027,.864),(.031,.028,.042),GREY,'hand_'+side)
        tube('Palm_'+side,[(sign*.333,-.025,.877),(sign*.344,-.03,.825),(sign*.346,-.028,.795)],
             [(.031,.025),(.039,.024),(.033,.021)],GREY,'hand_'+side)
        for f in range(4):
            x=sign*(.317+f*.019)
            tube('Finger_'+side+str(f),[(x,-.025,.806),(x,-.033,.77-(.012 if f in (1,2) else 0)),
                 (x,-.044,.751-(.012 if f in (1,2) else 0))],[.010,.008,.006],GREY,'hand_'+side,10,1)
        tube('Thumb_'+side,[(sign*.313,-.031,.838),(sign*.292,-.049,.817),(sign*.288,-.060,.792)],
             [.014,.011,.008],GREY,'hand_'+side,10,1)
        # Legs retain clear separation; short coat does not bridge the knees.
        x=sign*.107
        vertical_loft('Trousers_Draped_'+side,[(x,0,.29,.051,.058),(x,-.003,.35,.07,.076),
          (x,-.009,.49,.078,.084),(x,.0,.61,.090,.093),(x,.008,.78,.11,.105),
          (sign*.092,0,.90,.106,.104),(sign*.088,0,.95,.10,.10)],GREY,'leg_deform',.008)
        vertical_loft('Gaiter_'+side,[(x,0,.075,.043,.044),(x,.003,.14,.043,.044),
          (x,0,.22,.05,.049),(x,0,.32,.058,.055)],GREY,'shin_'+side,.003)
        ellipsoid('Foot_'+side,(x,-.042,.060),(.064,.128,.055),GREY,'foot_'+side)
        sole=ellipsoid('SandalSole_'+side,(x,-.042,.018),(.067,.133,.015),GREY,'foot_'+side)
    vertical_loft('Coat_DrapedTorso',[(0,.008,.89,.166,.116),(0,.01,.94,.168,.114),
       (0,.004,1.08,.151,.104),(0,.008,1.23,.183,.121),(0,.016,1.35,.211,.119),
       (0,.02,1.384,.163,.090),(0,.01,1.415,.073,.054)],GREY,'torso_deform',.007)
    cloth_panel('Left_OverlappingLapel',[(.078,.125,-.062,1.40),(.017,.085,-.125,1.31),
       (-.055,.04,-.132,1.18),(-.122,-.01,-.117,1.03)],GREY,'torso_deform',.003)
    cloth_panel('Right_UnderLapel',[(-.126,-.075,-.065,1.40),(-.10,-.035,-.126,1.32),
       (-.04,.065,-.13,1.21),(.005,.114,-.115,1.05)],GREY,'torso_deform',.003)
    for sign,side in [(-1,'R'),(1,'L')]:
        # Split hem with thickness and a sparse set of folds hung from the belt.
        cloth_panel('SplitCoatFront_'+side,[(sign*.01,sign*.176,-.12,.95),
          (sign*.018,sign*.181,-.132,.90),(sign*.039,sign*.20,-.132,.79),
          (sign*.062,sign*.194,-.113,.704)],GREY,'robe_deform',.012)
        cloth_panel('SplitCoatBack_'+side,[(sign*.008,sign*.17,.13,.95),
          (sign*.02,sign*.18,.132,.89),(sign*.04,sign*.195,.14,.77),
          (sign*.055,sign*.181,.135,.70)],GREY,'robe_deform',.012)
    tube('Right_Sleeve_Drape',[(-.175,0,1.36),(-.208,.0,1.33),(-.251,.003,1.215),(-.278,-.002,1.137)],
          [.078,.078,.069,.055],GREY,'upperarm_R',24,2)
    tube('Left_Sleeve_Rolled',[(.177,0,1.36),(.207,0,1.315),(.239,0,1.233)],
          [.078,.076,.062],GREY,'upperarm_L',24,2)
    # Existing two campaign stages are material-driven. These ridges mark the left arm in clay.
    for f in range(3):
        tube('LeftArm_StructuralVein_'+str(f),[(.255+f*.012,-.043,1.22),(.279+f*.014,-.055,1.10),
              (.304+f*.013,-.056,.99),(.324+f*.014,-.045,.885)], [.005,.009,.007,.003],GREY,'arm_deform',8,1)
    tube('Neck_ClothFold',[(-.07,-.052,1.418),(0,-.069,1.401),(.07,-.052,1.418)],.013,GREY,'chest',16,2)
    for z in [.933,.951]:
        tube('WaistCord',[(.178*math.cos(j*TAU/64),.122*math.sin(j*TAU/64),z) for j in range(65)],
             .009,GREY,'pelvis',8,1)
    # Boundary blade down and outward, separate from leg contour. No costume clutter.
    a=Vector((-.341,-.074,.831)); b=Vector((-.80,-.105,.08))
    axis=(b-a).normalized(); normal=Vector((axis.z,0,-axis.x))
    tube('Sakaidachi_Grip',[a-axis*.13,a-axis*.02,a], [.012,.013,.013],GREY,'weapon',12,1)
    tube('Sakaidachi_PlainGuard',[a-normal*.033,a,a+normal*.033],.007,GREY,'weapon',8,1)
    vs=[]
    for i in range(24):
        t=i/23; c=a.lerp(b,t)+normal*.022*t*t; w=.016*(1-.3*t) if i<23 else .0002
        vs.extend([c-normal*w,c+Vector((0,-.003,0)),c+normal*w,c+Vector((0,.003,0))])
    fs=[(i*4+j,i*4+(j+1)%4,(i+1)*4+(j+1)%4,(i+1)*4+j) for i in range(23) for j in range(4)]
    mesh('Sakaidachi_Blade',vs,fs,GREY,'weapon')


def y_loft(name, rings, mat):
    stone = name=='Rooted_DorsalStone'
    vs=[]; n=16 if stone else 48
    for y,z,rx,rz in rings:
        for j in range(n):
            a=j*TAU/n
            # A broad dorsum, narrower ventral keel: weight carried across the chest.
            x=rx*math.cos(a)*(1-.13*max(0,-math.sin(a)))
            height = min(math.sin(a), .68) if stone else math.sin(a)
            vs.append((x,y,z+rz*height))
    fs=[(k*n+j,k*n+(j+1)%n,(k+1)*n+(j+1)%n,(k+1)*n+j) for k in range(len(rings)-1) for j in range(n)]
    fs += [tuple(reversed(range(n))), tuple((len(rings)-1)*n+j for j in range(n))]
    ob=mesh(name,vs,fs,mat,'back',not stone)
    return ob if stone else sub(ob,2)


def unified_hide(parts):
    dg=bpy.context.evaluated_depsgraph_get()
    evaluated=[]
    for ob in parts:
        data=bpy.data.meshes.new_from_object(ob.evaluated_get(dg),depsgraph=dg)
        dup=bpy.data.objects.new('TEMP',data); bpy.context.collection.objects.link(dup)
        dup.matrix_world=ob.matrix_world.copy(); evaluated.append(dup)
        ob.hide_render=True; ob.hide_set(True)
        ob['role']='Editable anatomical mass control; hidden under continuous sculpt mesh'
    bpy.ops.object.select_all(action='DESELECT')
    for ob in evaluated:ob.select_set(True)
    bpy.context.view_layer.objects.active=evaluated[0]
    bpy.ops.object.join(); ob=bpy.context.object; ob.name='Ishibashiri_ContinuousAnatomy'
    mod=ob.modifiers.new('Union anatomical transitions', 'REMESH');mod.mode='VOXEL';mod.voxel_size=.055
    bpy.ops.object.modifier_apply(modifier=mod.name)
    sm=ob.modifiers.new('Relax large anatomical planes','SMOOTH');sm.factor=1.25;sm.iterations=7
    bpy.ops.object.modifier_apply(modifier=sm.name)
    finish(ob,GREY,'back')
    for p in ob.data.polygons:p.use_smooth=True
    return ob


def slab(name,x,y,z,rx,ry,depth,angle=0):
    # Cut rock slabs, flat walkable top and sloped rooted underside, not scattered spheres.
    outline=[(-1,-.48),(-.72,-.90),(.12,-1),(.87,-.69),(1,.10),(.68,.87),(-.30,1),(-.96,.55)]
    vs=[]
    for layer in range(3):
        for i,(u,v) in enumerate(outline):
            shrink=[.72,1,.93][layer]; zz=[-depth, -.14*depth, 0][layer]
            X=rx*u*shrink;Y=ry*v*shrink
            vs.append((x+X*math.cos(angle)-Y*math.sin(angle),y+X*math.sin(angle)+Y*math.cos(angle),
                       z+zz + (.025*math.sin(i*2.1) if layer==1 else 0)))
    fs=[tuple(reversed(range(8))),tuple(16+i for i in range(8))]
    fs += [(k*8+i,k*8+(i+1)%8,(k+1)*8+(i+1)%8,(k+1)*8+i) for k in range(2) for i in range(8)]
    ob=mesh(name,vs,fs,GREY,'back',False)
    bevel=ob.modifiers.new('Weathered broad edges','BEVEL');bevel.width=.07;bevel.segments=3
    ob.data.use_auto_smooth=True
    norm=ob.modifiers.new('Rock plane normals','WEIGHTED_NORMAL')
    return ob


def ishibashiri(route, cores):
    anatomy=[]
    anatomy.append(y_loft('Control_ChestToHaunch',[( -3.25,4.0,.75,1.10),(-2.80,4.05,1.93,1.65),
      (-2.05,4.15,2.50,2.15),(-.85,4.00,2.56,2.20),(.7,3.93,2.49,2.02),
      (2.45,3.93,2.29,1.87),(3.65,3.83,1.96,1.64),(4.65,3.71,1.25,1.26),(5.10,3.65,.20,.35)],GREY))
    anatomy.append(y_loft('Control_HeadToNose',[(-6.55,2.38,.68,.49),(-6.3,2.40,.83,.59),
       (-5.63,2.61,.98,.73),(-4.91,2.98,1.22,.99),(-4.12,3.30,1.60,1.40),
       (-3.44,3.47,1.77,1.61),(-2.79,3.58,1.56,1.49),(-2.40,3.70,1.13,1.17)],GREY))
    for sign in [-1,1]:
        anatomy.append(ellipsoid('Control_Scapula',(sign*1.60,-1.92,3.73),(1.12,1.30,1.83),GREY))
        anatomy.append(ellipsoid('Control_Haunch',(sign*1.53,3.27,3.14),(.99,1.13,1.36),GREY))
        anatomy.append(tube('Control_FrontLeg',[(sign*1.79,-2.04,3.56),(sign*2.04,-2.02,2.7),
            (sign*2.15,-2.03,1.80),(sign*2.18,-2.29,.98),(sign*2.18,-2.42,.39)],
             [.87,.79,.60,.48,.40],GREY,None,24,2))
        anatomy.append(tube('Control_RearLeg',[(sign*1.63,3.26,3.36),(sign*1.76,2.88,2.47),
            (sign*1.85,3.31,1.69),(sign*1.91,3.30,1.00),(sign*1.94,3.02,.39)],
             [.88,.73,.53,.40,.34],GREY,None,24,2))
        anatomy.append(ellipsoid('Control_Cheek',(sign*1.08,-4.0,2.85),(.70,.99,.90),GREY))
    unified_hide(anatomy)
    for sign in [-1,1]:
        for which,x,y in [('front',2.18,-2.48),('rear',1.94,2.99)]:
            for toe in [-1,1]:
                ob=ellipsoid('ClovenHoof_'+which,(sign*x+toe*.205,y,.23),(.204,.49,.225),GREY,'leg_deform')
                for v in ob.data.vertices:
                    if v.co.z<-.18:v.co.z=-.225
        # Small non-luminous eyes under the natural brow; tusks remain incidental.
        ellipsoid('Eye_Recess',(sign*1.29,-4.57,3.67),(.105,.115,.080),DARK,'head')
        ellipsoid('Eye_Quiet',(sign*1.35,-4.63,3.67),(.034,.045,.036),GREY,'head')
        tube('NaturalBrow',[(sign*1.17,-4.68,3.83),(sign*1.40,-4.35,3.90),(sign*1.50,-4.06,3.94)],
             [.10,.135,.08],GREY,'head',16,2)
        vs=[(sign*1.25,-3.12,4.44),(sign*1.87,-2.97,5.33),(sign*1.94,-3.47,4.65),
            (sign*1.57,-3.31,4.63),(sign*1.63,-3.14,4.83)]
        sub(solid(mesh('Ear_Leaf',vs,[(0,1,3),(1,2,3),(2,0,3)],GREY,'head'),.11),2)
        tube('NaturalTusk',[(sign*.99,-5.13,2.23),(sign*1.29,-5.45,2.08),(sign*1.58,-5.53,2.22),
             (sign*1.80,-5.44,2.56),(sign*1.82,-5.24,2.93)], [.19,.18,.14,.08,.008],GREY,'head',16,2)
        ellipsoid('Nostril',(sign*.35,-6.50,2.49),(.175,.061,.116),DARK,'head')
    # The nose plate is a broad, flattened end, continuous with the wedge-shaped head.
    ellipsoid('Nose_Plane',(0,-6.47,2.40),(.79,.17,.52),GREY,'head')
    for sign in [-1,1]:
        ellipsoid('Nostril_Open',(sign*.35,-6.635,2.49),(.16,.035,.095),DARK,'head')
    # One connected rock spine above dorsal muscle, with route terraces rooted into it.
    y_loft('Rooted_DorsalStone', [(-2.9,5.54,.15,.26),(-2.5,5.85,1.46,.58),
       (-1.4,6.10,2.04,.72),(-.3,6.24,2.17,1.00),(.75,6.39,2.16,1.16),
       (2.05,6.08,1.93,.96),(3.25,5.76,1.55,.69),(4.35,5.32,.74,.42)], GREY)
    for i,p in enumerate(route):
        if i<3:
            slab('RouteRoot_Foreleg_%02d'%i,p[0]+.24,p[1]+.03,p[2],.43,.67,.48,-.10)
        else:
            slab('RootedTerrace_%02d'%i,*p,.90 if i!=6 else 1.48,1.00 if i!=6 else 1.43,
                 .64 if i!=6 else .96,.13*math.sin(i))
    # Fill the stepped mountain around the fixed route; tilted strata follow back curvature.
    for i,(x,y,z,rx,ry,d) in enumerate([(0,-1.8,6.37,1.58,1.25,.82),(.18,-.45,7.17,1.65,1.35,.78),
       (.10,.55,7.70,1.55,1.46,.83),(.22,2.0,7.11,1.66,1.35,.84),(-.30,3.3,6.63,1.53,1.20,.77),
       (1.68,-1.51,5.92,.92,1.2,.71),(-1.61,-1.38,5.58,1.1,1.36,.78)]):
        slab('BackStratum_%02d'%i,x,y,z,rx,ry,d,.03*(i-3))
    # Union connected strata into one rock shell; preserve slab controls for editing.
    rock_parts=[o for o in bpy.context.scene.objects if o.name.startswith(('Rooted_DorsalStone','RootedTerrace','BackStratum'))]
    dg=bpy.context.evaluated_depsgraph_get()
    rock_shell=None
    for control in rock_parts:
        data=bpy.data.meshes.new_from_object(control.evaluated_get(dg),depsgraph=dg)
        part=bpy.data.objects.new('RockUnionPart',data);bpy.context.collection.objects.link(part)
        if rock_shell is None:
            rock_shell=part;rock_shell.name='Ishibashiri_ConnectedRockShell'
        else:
            boolean=rock_shell.modifiers.new('Unite rooted strata','BOOLEAN')
            boolean.operation='UNION';boolean.solver='EXACT';boolean.object=part
            bpy.context.view_layer.objects.active=rock_shell
            bpy.ops.object.modifier_apply(modifier=boolean.name)
            bpy.data.objects.remove(part,do_unlink=True)
        control.hide_render=True;control.hide_set(True)
        control['role']='Editable stratum control; hidden under connected rock shell'
    # Keep the fixed standing anchors on the outside of the unified rock shell.
    # Authored foot-sized recesses remove overburden, without moving gameplay nodes.
    for idx in (3,4,5,10):
        p=Vector(route[idx])
        bpy.ops.mesh.primitive_cube_add(size=1,location=p+Vector((0,0,1.35)))
        cutter=bpy.context.object;cutter.name='TEMP_FootingClearance'
        cutter.scale=(.92,1.02,2.70)
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
        boolean=rock_shell.modifiers.new('Fixed route footing clearance %02d'%idx,'BOOLEAN')
        boolean.operation='DIFFERENCE';boolean.solver='EXACT';boolean.object=cutter
        bpy.context.view_layer.objects.active=rock_shell
        bpy.ops.object.modifier_apply(modifier=boolean.name)
        bpy.data.objects.remove(cutter,do_unlink=True)
    finish(rock_shell,GREY,'back')
    # Handholds emerge from the load-bearing left foreleg. No gameplay anchors are moved.
    tube('RootedForelegButtress',[(-2.31,-2.1,.60),(-2.51,-2.02,1.40),(-2.52,-1.96,2.34),
        (-2.48,-1.99,3.29),(-2.08,-1.73,4.35)], [.32,.39,.45,.46,.49],GREY,'leg_deform',12,1)
    # Sparse backward-flowing mane masses along shoulder and haunch, no repeated belly spikes.
    for sign in [-1,1]:
        for i in range(9):
            y=-2.7+i*.7; x=sign*(2.16 if i<6 else 1.95); z=4.55 if i<6 else 4.18
            tube('Mane_LaidBack',[(x,y,z),(x+sign*.10,y+.23,z-.05),(x+sign*.12,y+.61,z-.24)],
                 [(.13,.26),(.12,.19),(.007,.035)],GREY,'back',10,1)
    tube('Tail',[(0,4.87,3.5),(.20,5.28,3.19),(.30,5.61,2.97),(.24,5.78,3.03)],
         [.13,.12,.09,.025],GREY,'tail',12,2)
    # Modest geological nodules at the unchanged Kakon positions, without a villain glow.
    for i,p in enumerate(cores):
        ellipsoid('Kakon_Form_%d'%i,p,(.22,.23,.19),GREY,'core_'+str(i))


def aim(ob,target):
    ob.rotation_euler=(Vector(target)-ob.location).to_track_quat('-Z','Y').to_euler()


def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.preferences.filepaths.save_version=0
    sc=bpy.context.scene;sc.unit_settings.system='METRIC';sc.unit_settings.scale_length=1
    sc.render.engine='CYCLES';sc.cycles.samples=args.samples;sc.cycles.use_denoising=True
    sc.render.resolution_x=1000;sc.render.resolution_y=1000;sc.render.resolution_percentage=100
    sc.render.image_settings.file_format='PNG';sc.render.film_transparent=False
    sc.view_settings.view_transform='Filmic';sc.view_settings.look='Medium High Contrast'
    sc.world=bpy.data.worlds.new('Neutral Studio');sc.world.use_nodes=True
    sc.world.node_tree.nodes.get('Background').inputs[0].default_value=(.15,.17,.19,1)
    sc.world.node_tree.nodes.get('Background').inputs[1].default_value=.35
    global GREY,DARK
    GREY=material('Neutral Clay',(.42,.42,.42));DARK=material('Geometric Recess',(.07,.07,.07))


def stage(name):
    hero=name=='Shirotsura';size=1 if hero else 7
    bpy.ops.mesh.primitive_plane_add(size=200)
    floor=bpy.context.object;floor.name='STUDIO_Floor';floor.location.z=-.007
    finish(floor,material('Neutral Floor',(.09,.105,.12)))
    target=(0,0,.85) if hero else (0,-.25,3.8)
    for label,loc,energy,width in [('Key',(-3*size,-4*size,6*size),500*size**2,4*size),
         ('Fill',(4*size,-2*size,3*size),170*size**2,3*size),
         ('Rim',(1*size,4*size,5*size),650*size**2,3*size)]:
        bpy.ops.object.light_add(type='AREA',location=loc);ob=bpy.context.object
        ob.name='STUDIO_'+label;ob.data.energy=energy;ob.data.shape='DISK';ob.data.size=width
        aim(ob,target)
    bpy.ops.object.camera_add();cam=bpy.context.object;cam.name='STUDIO_Camera';cam.data.type='ORTHO'
    bpy.context.scene.camera=cam
    return cam,target


def append_baseline(name):
    p=ROOT/'Art/Characters'/name/'Rigged'/(name+'_Rigged.blend')
    with bpy.data.libraries.load(str(p),link=False) as (src,dst):
        dst.objects=src.objects
    obs=[o for o in dst.objects if o and o.type in ('MESH','ARMATURE') and not o.name.startswith('STUDIO')]
    for ob in obs:
        bpy.context.collection.objects.link(ob)
        if ob.type=='ARMATURE':
            ob.data.pose_position='REST';ob.animation_data_clear()
    dg=bpy.context.evaluated_depsgraph_get();meshes=[];skeleton=[]
    for ob in obs:
        if ob.type=='ARMATURE':
            skeleton=[{'name':b.name,'parent':b.parent.name if b.parent else None,
                  'head_m':list(b.head_local),'tail_m':list(b.tail_local)} for b in ob.data.bones]
        if ob.type=='MESH':
            data=bpy.data.meshes.new_from_object(ob.evaluated_get(dg),depsgraph=dg)
            dup=bpy.data.objects.new('OLD_REST_'+ob.name,data);bpy.context.collection.objects.link(dup)
            dup.matrix_world=ob.matrix_world.copy();finish(dup,GREY);meshes.append(dup)
    for ob in obs:bpy.data.objects.remove(ob,do_unlink=True)
    return meshes,skeleton


def geometry(ob):
    dg=bpy.context.evaluated_depsgraph_get();ev=ob.evaluated_get(dg)
    data=ev.to_mesh();vs=[ob.matrix_world@v.co for v in data.vertices]
    data.calc_loop_triangles();fs=[tuple(t.vertices) for t in data.loop_triangles]
    ev.to_mesh_clear();return vs,fs


def metrics(obs):
    pts=[];tri=0;vert=0
    for ob in obs:
        vs,fs=geometry(ob);pts+=vs;tri+=len(fs);vert+=len(vs)
    lo=[min(v[i] for v in pts) for i in range(3)];hi=[max(v[i] for v in pts) for i in range(3)]
    return {'bounds_min_m':lo,'bounds_max_m':hi,'dimensions_xyz_m':[hi[i]-lo[i] for i in range(3)],
            'vertices':vert,'triangles':tri,'mesh_objects':len(obs)}


def route_source():
    path=ROOT/'Source/IshibashiriPrototype/Private/IshibashiriBoss.cpp'
    text=path.read_text(encoding='utf-8')
    def vectors(label):
        block=text.split('const FVector '+label+'[] = {')[1].split('};')[0]
        return [[float(v)/100 for v in row.split(',')] for row in re.findall(r'\{([^{}]+)\}',block)]
    neigh=text.split('const int32 Neighbors[][4] = {')[1].split('};')[0]
    neighbors=[[int(v) for v in row.split(',')] for row in re.findall(r'\{([^{}]+)\}',neigh)]
    route,cores=vectors('Route'),vectors('Cores')
    assert len(route)==11 and len(cores)==3
    return route,cores,neighbors,hashlib.sha256(path.read_bytes()).hexdigest()


def distances(obs,points):
    vertices=[];faces=[]
    for ob in obs:
        vs,fs=geometry(ob);offset=len(vertices);vertices+=vs
        faces += [tuple(i+offset for i in f) for f in fs]
    tree=BVHTree.FromPolygons(vertices,faces,all_triangles=True)
    return [round(tree.find_nearest(Vector(p))[3],5) for p in points]


def overlay(route,cores,neighbors,cam):
    mat=material('Diagnostic cyan',(.03,.72,.85))
    red=material('Diagnostic Kakon',(.9,.20,.07))
    result=[]
    for i,p in enumerate(route):
        result.append(ellipsoid('OVERLAY_Route_%02d'%i,p,(.11,.11,.11),mat))
        bpy.ops.object.text_add(location=Vector(p)+Vector((-.25,-.1,.22)))
        ob=bpy.context.object;ob.name='OVERLAY_Label_%02d'%i;ob.data.body=str(i);ob.data.size=.32
        ob.rotation_euler=cam.rotation_euler;ob.data.materials.append(mat);result.append(ob)
    edges={tuple(sorted((i,j))) for i,ns in enumerate(neighbors) for j in ns if j>=0 and i!=j}
    for i,j in sorted(edges):result.append(tube('OVERLAY_Path',[route[i],route[j]],.026,mat,None,8,0))
    for i,p in enumerate(cores):result.append(ellipsoid('OVERLAY_Kakon_%d'%i,p,(.20,.20,.20),red))
    # Grab marker = Route[0] + (0,0,.90); capsule center is (0,0,3.50) in mesh-local frame.
    result.append(ellipsoid('OVERLAY_GrabMarker',Vector(route[0])+Vector((0,0,.90)),(.17,.17,.17),red))
    return result


def export(obs,directory,name):
    bpy.ops.object.select_all(action='DESELECT')
    for ob in obs:ob.select_set(True)
    bpy.context.view_layer.objects.active=obs[0]
    bpy.ops.export_scene.gltf(filepath=str(directory/(name+'_Clay.glb')),export_format='GLB',use_selection=True,
        export_apply=True,export_animations=False)
    bpy.ops.export_scene.fbx(filepath=str(directory/(name+'_Clay.fbx')),use_selection=True,
        object_types={'MESH'},use_mesh_modifiers=True,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,
        add_leaf_bones=False,bake_anim=False)


def view(cam,name,view):
    hero=name=='Shirotsura';target=(0,0,.86) if hero else (0,-.3,3.8)
    d=6 if hero else 26
    positions={'Front':(0,-d,target[2]),'Side':(d,0,target[2]),'Back':(0,d,target[2]),
               'Oblique':(d*.7,-d,d*.5)}
    cam.location=positions[view];aim(cam,target);cam.data.ortho_scale=2.04 if hero else 14.1


def render(path):
    bpy.context.scene.render.filepath=str(path);bpy.ops.render.render(write_still=True)


def build(name):
    reset();directory=OUT/name;directory.mkdir(parents=True,exist_ok=True)
    route,cores,neighbors,source_hash=route_source()
    if name=='Shirotsura':shirotsura()
    else:ishibashiri(route,cores)
    candidates=[o for o in bpy.context.scene.objects if o.type=='MESH' and not o.hide_render]
    new_metrics=metrics(candidates)
    cam,target=stage(name);view(cam,name,'Oblique')
    export(candidates,directory,name)
    baseline,skeleton=append_baseline(name)
    for ob in baseline:ob.hide_render=True;ob.hide_set(True)
    old_metrics=metrics(baseline)
    bpy.data.orphans_purge(do_local_ids=True, do_linked_ids=True, do_recursive=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(directory/(name+'_FormReview.blend')),compress=True)
    report={'asset':name,'source':'new procedural DCC geometry; not Tripo AI','stage':'AWAITING_FORM_REVIEW',
       'adopted':False,'units':'meters','forward':'-Y','baseline':'rigged mesh evaluated in REST pose',
       'baseline_sha256':hashlib.sha256((ROOT/'Art/Characters'/name/'Rigged'/(name+'_Rigged.blend')).read_bytes()).hexdigest(),
       'candidate':new_metrics,'baseline_metrics':old_metrics,'existing_skeleton':skeleton,
       'skinning':'NOT_RUN','UE_import':'NOT_RUN','UE_contact_motion_calm':'NOT_RUN',
       'appearance_stages':['Early','Advanced'] if name=='Shirotsura' else None,
       'material_stage':'uniform clay plus geometric recesses; no textures; two-stage materials NOT_RUN',
       'baseline_commit':'6c4d3c5a9b065b779b858daa2b7d0c2048b132f8'}
    if name=='Ishibashiri':
        report['route']={'source':str(Path('Source/IshibashiriPrototype/Private/IshibashiriBoss.cpp')),
            'source_sha256':source_hash,'mesh_local_route_m':route,'mesh_local_cores_m':cores,'neighbors':neighbors,
            'grab_marker_m':list(Vector(route[0])+Vector((0,0,.90))),
            'note':'0..10 are mesh surface anchors; player capsule adds world Z .88m. Distances at static REST only.',
            'candidate_surface_distance_m':distances(candidates,route),'baseline_surface_distance_m':distances(baseline,route)}
    conditions={}
    for direction in ['Front','Side','Back','Oblique']:
        view(cam,name,direction)
        conditions[direction]={'camera_location':list(cam.location),'rotation_rad':list(cam.rotation_euler),
            'orthographic_scale_m':cam.data.ortho_scale,'resolution':[1000,1000],
            'engine':'Cycles CPU','samples':args.samples,'view_transform':'Filmic/Medium High Contrast',
            'world_strength':.35,'materials':'Neutral Clay .42, roughness .78; no textures',
            'old_pose':'REST','new_pose':'authored static standing'}
        if not args.build_only:
            for ob in candidates:ob.hide_render=True
            for ob in baseline:ob.hide_render=False
            render(directory/('Old_'+direction+'.png'))
            for ob in candidates:ob.hide_render=False
            for ob in baseline:ob.hide_render=True
            render(directory/('New_'+direction+'.png'))
    if name=='Ishibashiri' and not args.build_only:
        for direction in ['Side','Oblique']:
            view(cam,name,direction)
            os=overlay(route,cores,neighbors,cam)
            render(directory/('Route_'+direction+'.png'))
            for ob in os:bpy.data.objects.remove(ob,do_unlink=True)
    report['capture_conditions']=conditions
    (directory/'form-review.json').write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print('FORM_CANDIDATE_COMPLETE',name,new_metrics,flush=True)


def scale_comparison():
    reset();scene=bpy.context.scene;objects=[]
    for name in ['Ishibashiri','Shirotsura']:
        bpy.ops.import_scene.gltf(filepath=str(OUT/name/(name+'_Clay.glb')))
        imported=list(bpy.context.selected_objects)
        if name=='Shirotsura':
            for ob in imported:
                if not ob.parent:ob.location += Vector((3.9,-3.5,0))
        objects+=imported
    cam,target=stage('Ishibashiri');cam.location=(19,-27,15);aim(cam,(0,-.5,3.6));cam.data.ortho_scale=15.6
    # Meter ticks in the floor, actual stature versus length, without scaling either mesh.
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'ScaleComparison.blend'),compress=True)
    if not args.build_only:render(OUT/'ScaleComparison.png')


if __name__=='__main__':
    OUT.mkdir(parents=True,exist_ok=True)
    for name in ([args.character] if args.character else ['Shirotsura','Ishibashiri']):
        build(name)
    scale_comparison()
