"""Original, reference-guided static character studies. Blender 3.6, meters, -Y forward.
Run: blender --background --factory-startup --python Tools/CreateCharacterModels.py
"""
import bpy
import math
import random
import json
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1] / 'Art' / 'Characters'
random.seed(9108)
pi = math.pi


def material(name, color, rough=0.8, metal=0, emission=0, noise=0):
    m = bpy.data.materials.new(name)
    m.diffuse_color = (*color, 1)
    m.use_nodes = True
    p = m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value = (*color, 1)
    p.inputs['Roughness'].default_value = rough
    p.inputs['Metallic'].default_value = metal
    if emission:
        p.inputs['Emission'].default_value = (*color, 1)
        p.inputs['Emission Strength'].default_value = emission
    if noise:
        n = m.node_tree.nodes.new('ShaderNodeTexNoise')
        n.inputs['Scale'].default_value = 23
        n.inputs['Detail'].default_value = 3
        b = m.node_tree.nodes.new('ShaderNodeBump')
        b.inputs['Strength'].default_value = 0.28
        b.inputs['Distance'].default_value = noise
        m.node_tree.links.new(n.outputs['Fac'], b.inputs['Height'])
        m.node_tree.links.new(b.outputs['Normal'], p.inputs['Normal'])
    return m


def mesh(name, verts, faces, mat):
    me = bpy.data.meshes.new(name)
    me.from_pydata(verts, [], faces)
    me.update()
    ob = bpy.data.objects.new(name, me)
    bpy.context.collection.objects.link(ob)
    ob.data.materials.append(mat)
    return ob


def ico(name, loc, scale, mat, sub=2, rough=0):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=sub, radius=1, location=loc)
    ob = bpy.context.object
    ob.name = name
    for v in ob.data.vertices:
        v.co *= 1 + random.uniform(-rough, rough)
    ob.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    ob.data.materials.append(mat)
    return ob


def tube(name, points, radii, mat, sides=8):
    pts = [Vector(p) for p in points]
    verts = []
    for i, pt in enumerate(pts):
        tangent = (pts[min(i+1, len(pts)-1)] - pts[max(i-1, 0)]).normalized()
        cross = Vector((0, 1, 0)) if abs(tangent.y) < 0.9 else Vector((1, 0, 0))
        u = tangent.cross(cross).normalized()
        v = tangent.cross(u).normalized()
        r = radii[i] if isinstance(radii, (list, tuple)) else radii
        for j in range(sides):
            verts.append(pt + r * (u*math.cos(2*pi*j/sides) + v*math.sin(2*pi*j/sides)))
    faces = [tuple(reversed(range(sides)))]
    for i in range(len(pts)-1):
        for j in range(sides):
            a = i*sides+j
            b = i*sides+(j+1)%sides
            faces.append((a,b,b+sides,a+sides))
    faces.append(tuple((len(pts)-1)*sides+j for j in range(sides)))
    return mesh(name, verts, faces, mat)


def curve(name, points, radius, mat):
    cu = bpy.data.curves.new(name, 'CURVE')
    cu.dimensions = '3D'
    cu.resolution_u = 3
    cu.bevel_depth = radius
    cu.bevel_resolution = 0
    s = cu.splines.new('BEZIER')
    s.bezier_points.add(len(points)-1)
    for b,p in zip(s.bezier_points, points):
        b.co=p
        b.handle_left_type='AUTO'
        b.handle_right_type='AUTO'
    ob=bpy.data.objects.new(name,cu)
    bpy.context.collection.objects.link(ob)
    cu.materials.append(mat)
    return ob


def loft(name, rings, mat, n=16, folds=0):
    verts=[]
    for x,y,z,rx,ry in rings:
        for j in range(n):
            a=2*pi*j/n
            f=1+(folds if j%2 else -folds)
            verts.append((x+rx*math.cos(a)*f, y+ry*math.sin(a)*f,z))
    faces=[tuple(reversed(range(n)))]
    for i in range(len(rings)-1):
        for j in range(n):
            faces.append((i*n+j,i*n+(j+1)%n,(i+1)*n+(j+1)%n,(i+1)*n+j))
    faces.append(tuple((len(rings)-1)*n+j for j in range(n)))
    return mesh(name,verts,faces,mat)


def ribbon(name, pts, width, mat):
    verts=[]
    for p in pts:
        verts.extend([(p[0]-width/2,p[1],p[2]),(p[0]+width/2,p[1],p[2])])
    ob=mesh(name,verts,[(2*i,2*i+1,2*i+3,2*i+2) for i in range(len(pts)-1)],mat)
    sol=ob.modifiers.new('Cloth thickness','SOLIDIFY')
    sol.thickness=width*0.06
    return ob


def rope(name, points, radius, mat, turns=28):
    # Three continuous twisted strands along a sampled polyline.
    pts=[Vector(p) for p in points]
    centers=[]
    for a,b in zip(pts,pts[1:]):
        for i in range(8): centers.append(a.lerp(b,i/8))
    centers.append(pts[-1])
    for strand in range(3):
        out=[]
        for i,p in enumerate(centers):
            tangent=(centers[min(i+1,len(centers)-1)]-centers[max(i-1,0)]).normalized()
            axis=Vector((0,0,1)) if abs(tangent.z)<0.9 else Vector((0,1,0))
            u=tangent.cross(axis).normalized()
            v=tangent.cross(u).normalized()
            a=2*pi*(turns*i/(len(centers)-1)+strand/3)
            out.append(p+radius*0.48*(math.cos(a)*u+math.sin(a)*v))
        tube(name+f'_strand_{strand}',out,radius*0.52,mat,5)


def aim(ob, target):
    ob.rotation_euler=(Vector(target)-ob.location).to_track_quat('-Z','Y').to_euler()


def setup():
    bpy.context.preferences.filepaths.save_version=0
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    scene=bpy.context.scene
    scene.unit_settings.system='METRIC'
    scene.unit_settings.scale_length=1
    scene.render.engine='CYCLES'
    scene.cycles.samples=32
    scene.cycles.use_denoising=True
    scene.render.resolution_x=1400
    scene.render.resolution_y=1400
    scene.render.resolution_percentage=100
    scene.world.color=(0.12,0.12,0.12)
    scene.view_settings.view_transform='Filmic'
    scene.view_settings.look='Medium High Contrast'
    scene.render.image_settings.file_format='PNG'
    scene.render.film_transparent=False
    return scene


def stage(size, target, camera, ortho):
    ground=material('Studio charcoal',(0.028,0.032,0.035))
    bpy.ops.mesh.primitive_plane_add(size=size*200,location=(0,0,0))
    bpy.context.object.name='STUDIO_Ground'
    bpy.context.object.data.materials.append(ground)
    for name,loc,power,color,s in [
        ('Key',(-3*size,-4*size,6*size),550*size*size,(1,0.88,0.72),4*size),
        ('Fill',(4*size,-2*size,3*size),350*size*size,(0.65,0.78,1),3*size),
        ('Rim',(1*size,4*size,5*size),800*size*size,(1,0.92,0.77),3*size)]:
        bpy.ops.object.light_add(type='AREA',location=loc)
        light=bpy.context.object
        light.name='STUDIO_'+name
        light.data.energy=power
        light.data.color=color
        light.data.shape='DISK'
        light.data.size=s
        aim(light,target)
    bpy.ops.object.camera_add(location=camera)
    cam=bpy.context.object
    cam.name='STUDIO_Camera'
    cam.data.type='ORTHO'
    cam.data.ortho_scale=ortho
    aim(cam,target)
    bpy.context.scene.camera=cam
    return cam


def export(name, objects, directory):
    directory.mkdir(parents=True,exist_ok=True)
    bpy.ops.object.select_all(action='DESELECT')
    for ob in objects:
        ob.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]
    # Export real mesh geometry, including ropes, roots and cloth thickness.
    bpy.ops.object.convert(target='MESH')
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    objects=list(bpy.context.selected_objects)
    for ob in objects:
        ob['asset_stage']='Static design study; unrigged'
    bpy.ops.export_scene.gltf(filepath=str(directory/(name+'.glb')),use_selection=True,export_format='GLB')
    bpy.ops.export_scene.fbx(filepath=str(directory/(name+'.fbx')),use_selection=True,
        object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,
        add_leaf_bones=False,bake_anim=False)
    pts=[ob.matrix_world@Vector(v) for ob in objects for v in ob.bound_box]
    bounds=[max(p[i] for p in pts)-min(p[i] for p in pts) for i in range(3)]
    tris=0
    for ob in objects:
        ob.data.calc_loop_triangles()
        tris+=len(ob.data.loop_triangles)
    report={'asset':name,'stage':'static initial model','rigged':False,'unit':'meter',
        'forward':'-Y','mesh_objects':len(objects),'triangles':tris,'dimensions_xyz_m':bounds,
        'material_note':'GLB/FBX contain base colors, metallic/roughness and emission. Blender-only procedural bump is not baked.'}
    (directory/'model-info.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    return objects


def render(path,cam=None,location=None,target=None,ortho=None):
    if cam and location:
        cam.location=location
        aim(cam,target)
    if ortho: cam.data.ortho_scale=ortho
    bpy.context.scene.render.filepath=str(path)
    bpy.ops.render.render(write_still=True)


def hero():
    setup()
    navy=material('01 • indigo work cloth',(0.019,0.029,0.043),noise=.002)
    navy2=material('02 • worn fold edges',(0.036,0.046,0.055))
    black=material('03 • charcoal',(0.012,0.013,0.015))
    hair=material('04 • black hair',(0.013,0.017,0.018),.54)
    bone=material('05 • aged whitewood mask',(0.75,0.70,0.59),noise=.0005)
    linen=material('06 • unbleached linen',(0.42,0.37,0.29),noise=.001)
    cord=material('07 • rice straw',(0.33,0.27,0.17))
    red=material('08 • cinnabar cord',(0.32,0.018,0.013))
    root=material('09 • petrified corruption',(0.026,0.019,0.019),.7,noise=.002)
    glow=material('10 • crimson fissure',(0.50,0.008,0.004),emission=1.6)
    steel=material('11 • boundary blade',(0.43,0.5,0.54),.24,.86)
    edge=material('12 • sharpened edge',(.73,.77,.76),.2,.9)
    skin=material('13 • exposed right hand',(.27,.20,.15))
    repair=material('14 • faded indigo repair',(.030,.038,.046),rough=.94,noise=.0015)
    # Shoes, gaiters, generous work trousers.
    for side,x in [('R',-.115),('L',.115)]:
        ico('Boot_'+side,(x,-.027,.057),(.081,.141,.059),black)
        loft('Boot_sole_'+side,[(x,-.025,.018,.078,.143),(x,-.025,.037,.082,.145)],cord)
        loft('Gaiter_'+side,[(x,0,.075,.060,.061),(x,0,.18,.052,.05),(x,0,.36,.063,.068)],black)
        for i in range(6):
            z=.11+i*.036
            pts=[(x+.061*math.cos(t*2*pi/18),.062*math.sin(t*2*pi/18),z+.02*t/18) for t in range(19)]
            curve('Leg_wrap_'+side+str(i),pts,.009,linen)
        curve('Sandal_strap_'+side,[(x-.063,-.085,.072),(x,-.04,.116),(x+.063,-.085,.072)],.012,linen)
        loft('Work_trouser_'+side,[(x,0,.31,.059,.067),(x,0,.40,.094,.086),
             (x,0,.58,.108,.097),(x,0,.76,.111,.101),(x*.70,0,.91,.114,.113)],navy,20,.10)
        for j in range(5):
            a=pi+(j+1)*pi/6
            curve('Trouser_crease',[(x+.094*math.cos(a),.094*math.sin(a),.42),
                (x+.109*math.cos(a),.104*math.sin(a),.61),(x+.103*math.cos(a),.104*math.sin(a),.78)],.003,navy2)
    loft('Work_jacket',[(0,0,.79,.20,.122),(0,0,.92,.169,.105),(0,0,1.12,.185,.105),
                         (0,0,1.32,.222,.098),(0,0,1.39,.123,.087)],navy,24,.03)
    # Overlap panels and short asymmetric tails.
    mesh('Crossed_lapel',[(-.16,-.109,1.35),(-.10,-.122,1.39),(.11,-.119,1.01),(.06,-.13,.99)],[(0,1,2,3)],navy2)
    for x,z,w in [(-.135,.65,.10),(-.045,.69,.105),(.058,.74,.09),(.143,.78,.07)]:
        ribbon('Jacket_split_tail',[(x,-.127,.94),(x,-.13,.81),(x-.016,-.124,z)],w,navy)
    ico('Neck',(0,0,1.405),(.06,.06,.085),linen)
    # Tied hair behind thick elongated white mask.
    ico('Hair_mass',(0,.025,1.584),(.100,.083,.137),hair,3)
    ico('Hair_knot',(0,.101,1.659),(.059,.055,.056),hair,2)
    for i in range(18):
        a=2*pi*i/18
        curve('Hair_lock',[(.073*math.cos(a),.018+.062*math.sin(a),1.69),
              (.093*math.cos(a),.021+.077*math.sin(a),1.60),
              (.076*math.cos(a),.056+.07*math.sin(a),1.47-random.random()*.035)],.008,hair)
    rope('Hair_tie',[(-.045,.114,1.65),(0,.152,1.64),(.048,.115,1.65)],.006,red,5)
    # Curved mask surface with solid thickness and modeled nose.
    verts=[]
    rows=13; cols=13
    for i in range(rows):
        z=1.455+.257*i/(rows-1)
        u=i/(rows-1)
        width=.082*(.64+.36*math.sin(pi*u))
        for j in range(cols):
            t=-1+2*j/(cols-1)
            nose=.018*math.exp(-(t/.24)**2)*math.exp(-((u-.43)/.19)**2)
            verts.append((width*t,-.096-.039*(1-t*t)-nose,z))
    face=mesh('Whitewood_mask',verts,[(i*cols+j,i*cols+j+1,(i+1)*cols+j+1,(i+1)*cols+j)
        for i in range(rows-1) for j in range(cols-1)],bone)
    mod=face.modifiers.new('Carved wood thickness','SOLIDIFY'); mod.thickness=.012
    for p in face.data.polygons: p.use_smooth=True
    # Shallow adze marks and two tiny edge losses break the manufactured symmetry
    # without changing the mask silhouette or the narrow-eye read at game distance.
    for i in range(7):
        x=(-.050+i*.016)+(i%2)*.002
        curve('Mask_shallow_adze_mark',[(x,-.139,1.50+i*.026),(x+.011,-.141,1.515+i*.026)],.0007,linen)
    for x,z in [(-.078,1.477),(.079,1.669)]:
        ico('Mask_edge_chip',(x,-.105,z),(.008,.008,.012),black,1)
    for s in [-1,1]:
        curve('Narrow_eye_slit',[(s*.022,-.134,1.593),(s*.042,-.131,1.598),(s*.064,-.117,1.598)],.003,black)
    curve('Quiet_mouth',[(-.022,-.134,1.497),(0,-.138,1.501),(.020,-.134,1.497)],.0016,black)
    curve('Single_vermilion_mark',[(.037,-.126,1.699),(.032,-.136,1.65),(.035,-.142,1.62),(.029,-.143,1.535)],.002,red)
    # Scarf collar in stacked irregular loops; two short rear cloth ends.
    for i in range(5):
        pts=[]
        for j in range(25):
            a=2*pi*j/24
            pts.append((.105*math.cos(a),.097*math.sin(a),1.402+i*.011-.024*max(0,-math.sin(a))))
        curve('Scarf_fold',pts,.013,linen)
    for x,z in [(-.038,1.11),(.032,1.21)]:
        ribbon('Rear_scarf_tail',[(x,.11,1.43),(x+.024,.146,1.32),(x+.004,.125,z)],.071,linen)
    # Short shoulder mantle: a coarse woven foundation and sparse silhouette straw.
    # It ends above the elbows so Climb/Hang/Grip retain their existing clearance.
    ribbon('Short_straw_mantle',[(-.205,-.005,1.355),(0,-.025,1.405),(.205,-.005,1.355)],.105,cord)
    for side in [-1,1]:
        for i in range(9):
            x=side*(.055+i*.017); y=.005+(i%3)*.007
            tube('Shoulder_straw_tuft',[(x,y,1.375),(x+side*.025,y+.012,1.305-random.random()*.025)],
                 [.006,.001],cord,5)
    # Right human sleeve, forearm bandages and hand.
    tube('Right_sleeve',[(-.175,0,1.32),(-.245,0,1.24),(-.275,-.014,1.14)],[.091,.10,.077],navy,12)
    tube('Right_forearm',[(-.273,-.014,1.17),(-.31,-.04,1.00)],[.047,.035],linen,10)
    for i in range(6):
        z=1.015+i*.023; x=-.31+(z-1.0)*.22
        curve('Wrist_bandage',[(x+.038*math.cos(a*2*pi/16),-.035+.04*math.sin(a*2*pi/16),z) for a in range(17)],.007,navy2)
    ico('Right_hand',(-.321,-.039,.964),(.038,.037,.057),skin)
    # Left arm keeps a human proportion, threaded with mineral roots.
    tube('Torn_left_sleeve',[(.174,0,1.32),(.235,0,1.27)],[.089,.08],navy,11)
    tube('Corrupted_left_arm',[(.221,0,1.29),(.274,0,1.16),(.294,-.014,1.08),(.318,-.025,.974)],[.063,.047,.051,.032],root,11)
    ico('Corrupted_palm',(.325,-.022,.944),(.040,.03,.051),root)
    for j in range(4):
        x=.296+j*.019
        tube('Left_finger_'+str(j),[(x,-.024,.929),(x+.002,-.031,.892),(x+.007,-.045,.872)],[.010,.008,.004],root,6)
    tube('Left_thumb',[(.351,-.018,.96),(.38,-.028,.932),(.384,-.047,.916)],[.014,.010,.005],root,7)
    for i in range(12):
        a=2*pi*i/12
        pts=[]
        for j in range(6):
            t=j/5; x=.236+.087*t
            pts.append((x+(.049-.020*t)*math.cos(a+.5*math.sin(j)),
                -.010+(.052-.021*t)*math.sin(a+.5*math.sin(j)),1.285-.36*t))
        curve('Arm_black_root',pts,.006+random.random()*.003,black)
        if i%3==0: curve('Arm_red_fissure',[(x,y-.003,z) for x,y,z in pts],.0024,glow)
    for z,x in [(1.16,.307),(1.07,.332)]:
        tube('Short_root_spur',[(x,.005,z),(x+.025,.017,z+.014),(x+.038,.02,z+.055)],[.014,.009,.0008],root)
    for i in range(3):
        x=.268+i*.015
        curve('Visible_forearm_fissure',[(x,-.050,1.17),(x+.012,-.067,1.125),
            (x+.004,-.071,1.085),(x+.025,-.064,1.035),(x+.027,-.058,.989)],.0026,glow)
    for i in range(3):
        curve('Palm_fissure',[(.309+i*.014,-.053,.967),(.314+i*.014,-.052,.938),
            (.310+i*.014,-.043,.909)],.0018,glow)
    for i in range(5):
        curve('Shoulder_infection',[(.22,.037,1.30),(.17-i*.015,.098,1.33),(.10-i*.012,.103,1.27-i*.012)],.005,root)
    # One diagonal utility strap, waist rope, sparse ritual accessories.
    ribbon('Diagonal_utility_strap',[(-.176,-.09,1.34),(-.06,-.126,1.20),(.15,-.118,1.00)],.028,cord)
    pts=[(.183*math.cos(a*2*pi/48),.126*math.sin(a*2*pi/48),.94+.008*math.sin(a*4*pi/48)) for a in range(49)]
    rope('Waist_shimenawa',pts,.018,linen,40)
    rope('Rear_knot',[(-.026,.122,.95),(.04,.155,.956),(.078,.14,.918),(0,.14,.927),(-.026,.122,.95)],.014,linen,8)
    for i,(x,y,z) in enumerate([(-.105,-.146,1.27),(.144,-.143,.91),(-.06,.142,.89)]):
        ribbon('Paper_talisman_'+str(i),[(x,y,z),(x+.004,y-.004,z-.072)],.027,bone)
        for j in range(3):
            curve('Talisman_ink',[(x-.005,y-.002,z-.014-j*.013),(x+.006,y-.003,z-.021-j*.013)],.0013,black)
    ico('Utility_pouch',(-.183,.065,.869),(.053,.047,.073),root)
    for x,z,flip in [(-.11,1.12,1),(.075,.83,-1),(-.055,.58,1)]:
        ribbon('Indigo_visible_repair',[(x,-.132,z),(x+.035*flip,-.134,z-.065)],.035,repair)
        for j in range(3):
            curve('Repair_stitch',[(x-.018,-.136,z-.012-j*.017),(x+.018,-.136,z-.012-j*.017)],.0012,linen)
    ribbon('Waist_linen_tail',[(-.097,-.136,.94),(-.113,-.154,.77),(-.145,-.137,.60)],.039,linen)
    curve('Red_waist_cord',[(.125,-.15,.951),(.18,-.147,.89),(.152,-.148,.76)],.005,red)
    # Katana as a separate editable object group, posed down and outward.
    start=Vector((-.323,-.055,.989)); end=Vector((-.85,-.105,.12))
    direction=(end-start).normalized(); normal=Vector((direction.z,0,-direction.x)).normalized()
    grip_start=start-direction*.14
    tube('Sakaidachi_grip',[grip_start,start],[.014,.014],black,10)
    for i in range(8):
        p=grip_start.lerp(start,i/8)
        tube('Grip_wrap',[p-normal*.014,p+normal*.014],.004,linen,5)
    tube('Plain_guard',[start-normal*.035,start+normal*.035],.008,root,8)
    vs=[]
    for i in range(16):
        t=i/15; c=start.lerp(end,t)+normal*(.027*t*t)
        w=.018*(1-.45*t) if i<15 else .0004
        vs.extend([c-normal*w,c+Vector((0,-.006,0)),c+normal*w,c+Vector((0,.004,0))])
    sword=mesh('Sakaidachi_blade',vs,[(i*4+j,i*4+(j+1)%4,(i+1)*4+(j+1)%4,(i+1)*4+j)
        for i in range(15) for j in range(4)],steel)
    sword.data.materials.append(edge)
    for p in sword.data.polygons:
        if p.index%4 in [0,3]: p.material_index=1
    tube('Sakaidachi_scabbard',[(.13,.09,.958),(.27,.12,.65),(.42,.16,.32)],[.021,.018,.014],black,10)
    objects=export('Shirotsura',list(bpy.context.scene.objects),ROOT/'Shirotsura')
    cam=stage(1,(0,0,.86),(2.7,-5,2.25),2.04)
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Shirotsura'/'Shirotsura.blend'))
    render(ROOT/'Previews'/'Shirotsura.png')
    render(ROOT/'Previews'/'Shirotsura_Back.png',cam,(2.7,5,2.15),(0,0,.88),2.02)
    return objects


def rock(name,loc,scale,mat,flat=False):
    ob=ico(name,loc,scale,mat,2,.18)
    if flat:
        for v in ob.data.vertices:
            if v.co.z>scale[2]*.30: v.co.z=scale[2]*.62+random.uniform(-.014,.014)
    return ob


def boar():
    setup()
    hide=material('01 • umber hide',(.057,.049,.040),noise=.025)
    fur=material('02 • charcoal bristles',(.032,.030,.025),noise=.016)
    stones=[material('03 • granite '+str(i),c,noise=.026) for i,c in enumerate([
        (.22,.215,.19),(.29,.28,.25),(.155,.16,.15),(.34,.33,.29)])]
    moss=material('04 • moss cushion',(.095,.13,.042),noise=.016)
    mosslight=material('05 • dry lichen',(.19,.21,.083),noise=.015)
    rope_mat=material('06 • weathered straw',(.40,.335,.23),noise=.006)
    rootmat=material('07 • blighted root',(.025,.018,.017),noise=.01)
    ember=material('08 • crimson fracture',(.43,.004,.007),emission=2.0)
    crystal=material('09 • redblack mineral',(.10,.006,.009),.38,.3)
    dark=material('10 • nostril shadow',(.008,.007,.006))
    ivory=material('11 • paper offerings',(.60,.54,.42))
    faded=material('12 • faded paper offerings',(.39,.35,.27),rough=.96,noise=.003)
    # Massive barrel-shaped boar with a lowered recognizable snout.
    ico('Boar_barrel',(0,.9,4.10),(2.52,4.42,2.44),hide,3,.035)
    ico('Shoulder_hump',(0,-1.7,4.60),(2.64,2.43,2.15),hide,3,.045)
    ico('Lowered_head',(0,-3.57,3.17),(1.85,1.95,1.76),hide,3,.035)
    ico('Tapered_muzzle',(0,-4.89,2.52),(1.09,1.48,.89),hide,3)
    nose=ico('Broad_nose',(0,-6.01,2.41),(.90,.40,.62),stones[2],3)
    for s in [-1,1]:
        ico('Deep_nostril',(s*.37,-6.362,2.46),(.205,.065,.155),dark,2)
        curve('Nose_ridge',[(s*.73,-6.24,2.66),(s*.40,-6.38,2.83),(s*.14,-6.35,2.73)],.052,hide)
        ico('Eye_socket',(s*1.41,-4.57,3.69),(.30,.29,.23),dark,2)
        ico('Small_ember_eye',(s*1.48,-4.78,3.69),(.090,.065,.065),ember,2)
        tube('Heavy_brow',[(s*1.09,-4.67,3.99),(s*1.5,-4.69,3.94),(s*1.68,-4.23,4.06)],[.18,.23,.14],hide,8)
        ear=mesh('Pointed_ear',[(s*1.27,-3.28,4.47),(s*2.18,-2.85,5.73),(s*2.15,-3.69,4.74),
             (s*1.75,-3.36,4.88),(s*1.81,-3.04,5.10)],[(0,1,3),(1,2,3),(2,0,3),(0,4,1),(1,4,2),(2,4,0)],hide)
        # Cloven hooves and pillar legs; belly deliberately free of rocks.
        for idx,y in enumerate([-1.97,3.28]):
            x=s*(1.69 if idx==0 else 1.59)
            tube('Leg_'+str(s)+'_'+str(idx),[(x,y,3.49),(x+s*.18,y-.18,1.83),(x+s*.27,y-.26,.58)],
                 [.82,.61,.40],hide,10)
            ico('Leg_muscle',(x,y,2.85),(.85,.85,1.26),hide,2,.04)
            for toe in [-1,1]:
                ico('Cloven_hoof',(x+s*.27+toe*.24,y-.41,.30),(.23,.58,.31),stones[2],2,.05)
            if idx==0:
                for j in range(3):
                    rock('Foreleg_grab_stone',(x+s*(.3+.08*j),y-.22,1.1+j*.66),(.48,.63,.24),stones[j],True)
        # Monumental hooked, faceted rock tusks, intentionally asymmetrical.
        points=[(s*1.03,-4.88,2.15),(s*1.83,-5.35,1.57),(s*2.61,-5.31,1.85),
            (s*3.05,-4.92,2.72),(s*(3.08 if s<0 else 2.98),-4.50,3.75),
            (s*(2.90 if s<0 else 2.78),-4.22,4.64 if s<0 else 4.35)]
        tube('Monument_tusk_'+str(s),points,[.47,.50,.42,.32,.20,.025 if s<0 else .12],stones[1],7)
        for j in range(2):
            a=Vector(points[j+1]); b=Vector(points[j+2])
            curve('Tusk_fault',[a+Vector((0,-.40,.08)),a.lerp(b,.5)+Vector((.08,-.34,.04)),b+Vector((0,-.23,0))],.022,stones[2])
    # Layered mane and cheek tufts, silhouette-only geometry.
    for s in [-1,1]:
        for i in range(16):
            y=-4.3+i*.53; z=2.6 if y< -2 else 3.1
            x=s*(1.46 if y< -2 else 2.30)
            tube('Coarse_fur_clump',[(x,y,z+.3),(x+s*.12,y+.12,z),(x+s*.05,y+.25,z-.43-random.random()*.3)],
                [.18,.15,.008],fur,5)
    rock('Forehead_prayer_plate',(0,-4.30,4.29),(.95,.68,.29),stones[1])
    for i,(x,y,z,sc) in enumerate([(0,-2.20,6.02,(1.25,1.13,.74)),
             (-1.42,-2.38,5.71,(.97,1.02,.65)),(1.44,-2.33,5.74,(1.06,1.07,.72))]):
        rock('Shoulder_crown_'+str(i),(x,y,z),sc,stones[i])
        rock('Shoulder_moss_'+str(i),(x,y,z+sc[2]*.77),(.62,.64,.10),moss,True)
        # Rooted transition stones visually key the hide into the continuous strata.
        for j in range(3):
            rock('Hide_to_stone_transition',(x+(j-1)*.31,y-.42,z-.36-j*.10),
                 (.42,.47,.24),stones[(i+j)%4])
            curve('Transition_root',[(x,y-.2,z),(x+(j-1)*.38,y-.5,z-.22),(x+(j-1)*.48,y-.62,z-.48)],.045,rootmat)
    for s in [-1,1]:
        curve('Worn_forehead_prayer',[(s*.32,-4.87,4.22),(s*.24,-4.82,4.45),(s*.20,-4.62,4.51)],.023,rope_mat)
    # Large primary stones. Flattened shelves have deliberate standing space.
    shelves=[(-2.13,-1.98,4.55,(1.18,1.38,.48)),(-1.78,-1.00,5.55,(1.28,1.40,.58)),
        (-.95,-.48,6.51,(1.29,1.37,.54)),(0,.55,7.23,(1.75,1.57,.73)),
        (.36,2.18,6.76,(1.64,1.28,.61)),(1.48,2.85,5.96,(1.15,1.26,.55)),
        (2.16,-1.47,4.92,(1.01,1.16,.48))]
    route=[]
    for i,(x,y,z,sc) in enumerate(shelves):
        ob=rock('CLIMB_Terrace_%02d'%i,(x,y,z),sc,stones[i%4],True)
        ob['role']='proposed standing shelf; gameplay not implemented'
        route.append({'name':ob.name,'center_m':[x,y,z+sc[2]*.62],'approx_width_m':sc[0]*1.6})
        for j in range(4):
            rock('Moss_on_terrace',(x+random.uniform(-.7,.7)*sc[0],y+random.uniform(-.65,.65)*sc[1],z+sc[2]*.62+.02),
                (.32+random.random()*.28,.27+random.random()*.26,.065),moss if j%2 else mosslight,True)
    # Structured mountain flank: medium supports, no scatter on open belly.
    for s in [-1,1]:
        for i in range(8):
            y=-2.5+i*.88
            x=s*(2.05+random.uniform(-.20,.24))
            z=4.8+.65*math.sin((i/7)*pi)
            rock('Mountain_flank',(x,y,z),(.79+random.random()*.28,.81,.69+random.random()*.35),stones[(i+1)%4])
            if i%2==0:
                rock('Moss_flank',(x,y,z+.47),(.64,.61,.14),moss,True)
    for loc,sc in [((-1.75,.92,7.13),(.73,.82,1.42)),((1.36,.52,6.56),(.81,1.30,1.03)),
                    ((-.84,3.4,6.03),(1.21,.87,.85)),((.19,4.4,5.13),(1.76,.88,.61))]:
        rock('Mountain_spine',loc,sc,random.choice(stones))
    # Three readable corruption cores away from the forehead.
    cores=[(2.18,-1.55,6.11),(0,.60,7.84),(-1.22,3.05,6.88)]
    for k,c in enumerate(cores):
        c=Vector(c)
        rock('MAGANE_%02d_Core'%(k+1),c,(.43,.40,.41),rootmat)
        # A root-bound nodule with only three broken mineral teeth.  The restrained
        # cracks remain legible while preserving the three gameplay core positions.
        for j in range(7):
            a=j*2*pi/7
            start=c+Vector((.28*math.cos(a),.27*math.sin(a),-.08))
            top=start+Vector((.18*math.cos(a),.18*math.sin(a),.38+random.random()*.38))
            if j in (0,2,5):
                tube('Redblack_crystal',[start,top],[.10,.018],crystal,5)
            curve('Core_emissive_crack',[start+Vector((0,-.115,.015)),start.lerp(top,.62)],.010,ember)
            pts=[c+Vector((0,0,.1)),c+Vector((.53*math.cos(a),.53*math.sin(a),-.06)),
                 c+Vector((1.01*math.cos(a+.18),.96*math.sin(a+.18),-.37)),
                 c+Vector((1.46*math.cos(a),1.28*math.sin(a),-.78))]
            curve('Spreading_black_root',pts,.063,rootmat)
            if j in (0,4): curve('Root_crimson_vein',[p+Vector((0,-.033,.046)) for p in pts[:3]],.009,ember)
    # Two sweeping shimenawa route markers and sparse tassels.
    paths=[[(-2.90,-2.2,4.96),(-2.64,-3.0,5.53),(-1.37,-3.28,6.16),
        (0,-3.23,6.39),(1.36,-3.28,6.20),(2.64,-3.0,5.57),(2.90,-2.2,4.96)],
        [(-2.83,2.65,5.35),(-2.30,4.14,5.67),(0,5.06,5.55),(2.29,4.14,5.60),(2.84,2.45,5.30)]]
    for i,path in enumerate(paths):
        rope('Sacred_rope_'+str(i),path,.132,rope_mat,52)
        for j,p in enumerate(path[::2]):
            x,y,z=p
            rope('Rope_drop',[(x,y,z),(x+.04,y-.04,z-.43)],.065,rope_mat,9)
            for t in range(7):
                a=2*pi*t/7
                tube('Straw_tassel',[(x+.05*math.cos(a),y+.05*math.sin(a),z-.39),
                     (x+.13*math.cos(a),y+.13*math.sin(a),z-.94)],[.025,.016],rope_mat,5)
            sag=.025*((i+j)%3); skew=(-1 if (i+j)%2 else 1)*.035
            ribbon('Paper_shide',[(x+.17,y,z-.13),(x+.2+skew,y-.05,z-.33-sag),
                (x+.11,y-.06,z-.47-sag),(x+.18-skew,y-.08,z-.62-sag)],.12,
                faded if (i+j)%2 else ivory)
    # Weathered trees on the perimeter, away from the standing route.
    for x,y,z in [(-1.78,1.03,7.89),(1.78,2.75,6.14)]:
        points=[(x,y,z),(x+.10,y+.05,z+.57),(x-.20,y+.24,z+1.13),(x-.49,y+.33,z+1.48)]
        tube('Dead_tree',points,[.18,.13,.073,.013],rootmat,7)
        for s in [-1,1]:
            tube('Dead_branch',[(x+.06,y+.07,z+.60),(x+s*.5,y+.23,z+.94),(x+s*.87,y+.48,z+1.05)], [.076,.04,.006],rootmat,6)
    curve('Small_tail',[(0,4.92,3.52),(.13,5.55,3.20),(.26,5.67,2.86)],.094,fur)
    objects=export('Ishibashiri',list(bpy.context.scene.objects),ROOT/'Ishibashiri')
    (ROOT/'Ishibashiri'/'climb-layout.json').write_text(json.dumps({'status':'visual layout only; Grab/Climbing/collision not implemented',
        'main_route_terraces':[0,1,2,3,4],'branch_terraces':[4,5], 'terraces':route,
        'corruption_cores_m':cores},indent=2),encoding='utf-8')
    cam=stage(7,(0,-.2,4.25),(16,-23,14),18.4)
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Ishibashiri'/'Ishibashiri.blend'))
    render(ROOT/'Previews'/'Ishibashiri.png')
    render(ROOT/'Previews'/'Ishibashiri_Back.png',cam,(-16,23,15),(0,0,4.25),18.0)
    render(ROOT/'Previews'/'Ishibashiri_Top.png',cam,(0,-.1,30),(0,0,0),17)


if __name__=='__main__':
    import sys
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    if not args or 'hero' in args: hero()
    if not args or 'boar' in args: boar()
    print('CHARACTER_MODELS_COMPLETE')
