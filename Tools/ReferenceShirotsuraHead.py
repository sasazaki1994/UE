"""Reference rebuild: carved ritual mask, swept hair, tied bun and draped scarf.

Meter-scale rest pose; callable build(m, mats) deliberately has no exports or
scene/file setup. Every authored object declares its existing skeleton region.
"""
import math
import random

import bpy
from mathutils import Vector


PI = math.pi


def _finish(ob, region='head', smooth=True):
    ob['rig_region'] = region
    if smooth and ob.type == 'MESH':
        for face in ob.data.polygons:
            face.use_smooth = True
    return ob


def _shell(ob, thickness, region='head'):
    mod = ob.modifiers.new('Sculpted shell thickness', 'SOLIDIFY')
    mod.thickness = thickness
    mod.offset = -1
    return _finish(ob, region)


def _stroke(m, name, points, radius, mat, region='head'):
    return _finish(m.curve(name, points, radius, mat), region)


def _sample(controls, count=19):
    a, b, c, d = [Vector(p) for p in controls]
    return [a*(1-t)**3 + b*3*(1-t)**2*t + c*3*(1-t)*t*t + d*t**3
            for t in [i/(count-1) for i in range(count)]]


def _mask_surface(x, z):
    u = max(0, min(1, (z-1.468)/.214))
    width = .079*(.35+.65*math.sin(PI*u))
    t = x/max(.001, width)
    y = -.077-.029*(1-t*t)
    # Broad brow planes, cut eye sockets, a narrow bridge and angular nose tip.
    y -= .013*math.exp(-(x/.012)**4-((u-.46)/.20)**4)
    y -= .015*math.exp(-(x/.014)**4-((u-.33)/.055)**4)
    y -= .005*math.exp(-((abs(x)-.047)/.023)**2-((u-.34)/.15)**2)
    y -= .007*math.exp(-((abs(x)-.040)/.027)**2-((u-.65)/.056)**2)
    y += .007*math.exp(-((abs(x)-.040)/.021)**2-((u-.566)/.038)**2)
    y -= .004*math.exp(-(x/.028)**2-((u-.19)/.065)**2)
    return y


def _eye(x):
    a = abs(x)
    t = max(0, min(1, (a-.019)/.046))
    center = 1.589+.24*(a-.04)
    half = .0024+.0032*math.sin(PI*t)**.65
    return center-half, center+half


def _paint(m, name, coordinates, mat, radius=.00065):
    # Dense sampling keeps paint attached to the sculpt even across the nose.
    points = []
    for a, b in zip(coordinates, coordinates[1:]):
        for i in range(7):
            t = i/7
            x, z = a[0]*(1-t)+b[0]*t, a[1]*(1-t)+b[1]*t
            points.append((x, _mask_surface(x, z)-.0010, z))
    x, z = coordinates[-1]
    points.append((x, _mask_surface(x, z)-.0010, z))
    return _stroke(m, name, points, radius, mat)


def _face(m, mats):
    skin, white, dark, red = (mats[k] for k in ('skin', 'mask', 'hair', 'red'))
    _finish(m.ico('Head_under_mask', (0, .015, 1.574), (.082, .076, .108), skin, 3))
    _finish(m.loft('Neck_anatomy', [(0,.017,1.356,.053,.048),
                                    (0,.017,1.414,.051,.047),
                                    (0,.009,1.475,.057,.051),
                                    (0,.007,1.507,.064,.055)], skin, 24), 'head')
    for side in (-1, 1):
        _finish(m.ico('Head_ear_pinna', (side*.081,.010,1.568), (.022,.018,.035),skin,3))
        _finish(m.ico('Head_ear_concha', (side*.094,-.006,1.569), (.010,.004,.022),mats['leather'],2))
        pts = [(side*(.085+.014*math.cos(a)), -.009-.004*math.sin(a),
                1.568+.026*math.sin(a)) for a in [2*PI*i/24 for i in range(25)]]
        _stroke(m,'Head_ear_helix',pts,.0033,skin)
        _stroke(m,'Head_ear_inner_cartilage',[(side*.093,-.013,1.582),
                                             (side*.099,-.014,1.572),
                                             (side*.093,-.013,1.557)],.0026,skin)
    rows, cols = 89, 65
    verts = []
    for i in range(rows):
        u=i/(rows-1)
        width=.079*(.35+.65*math.sin(PI*u))
        for j in range(cols):
            t=-1+2*j/(cols-1)
            x=width*t
            z=1.468+.214*u+.011*t*t*(1-u)**6-.017*t*t*u**8
            verts.append((x,_mask_surface(x,z),z))
    faces=[]
    for i in range(rows-1):
        for j in range(cols-1):
            indices=(i*cols+j,i*cols+j+1,(i+1)*cols+j+1,(i+1)*cols+j)
            x=sum(verts[k][0] for k in indices)/4
            z=sum(verts[k][2] for k in indices)/4
            low,high=_eye(x)
            if .019<abs(x)<.065 and low<z<high:
                continue
            faces.append(indices)
    mask=m.mesh('Reference_mask_carved_porcelain',verts,faces,white)
    _shell(mask,.0045)
    # Inner black lining follows the curved shell and cannot protrude at temples.
    for side in (-1,1):
        vertices=[]
        for row in range(2):
            for j in range(25):
                x=side*(.015+.055*j/24)
                low,high=_eye(x)
                z=low-.004 if row==0 else high+.004
                vertices.append((x,_mask_surface(x,z)+.007,z))
        faces=[(j,j+1,j+26,j+25) for j in range(24)]
        if side<0:faces=[tuple(reversed(face)) for face in faces]
        _finish(m.mesh('Reference_mask_dark_eye_recess',vertices,faces,dark))
        # Thin sculpted lid crests obscure the sampling staircase at the opening.
        for upper in (False,True):
            pts=[]
            for j in range(23):
                x=side*(.019+.046*j/22)
                z=_eye(x)[1 if upper else 0]
                pts.append((x,_mask_surface(x,z)-.0004,z))
            _stroke(m,'Reference_mask_sharp_eyelid',pts,.0008,white)
        _paint(m,'Reference_mask_red_tear',[(side*.043,1.581),
              (side*.042,1.548),(side*.039,1.515),(side*.028,1.484)],red,.00105)
        _paint(m,'Reference_mask_temple_red',[(side*.066,1.628),
              (side*.069,1.607),(side*.062,1.585)],red,.0008)
    # The characteristic tapered red forehead triangle is an actual surface patch.
    vertices=[]
    for i in range(25):
        t=i/24;z=1.678-.062*t;w=.0105*(1-t)+.00012
        for x in (-w,0,w): vertices.append((x,_mask_surface(x,z)-.0010,z))
    _finish(m.mesh('Reference_mask_red_forehead_triangle',vertices,
                   [(i*3+j,i*3+j+1,(i+1)*3+j+1,(i+1)*3+j)
                    for i in range(24) for j in range(2)],red))
    _paint(m,'Reference_mask_quiet_mouth',[(-.022,1.510),(-.015,1.513),
            (0,1.512),(.016,1.513),(.022,1.510)],mats['leather'],.0008)
    for side in (-1,1):
        _paint(m,'Reference_mask_mouth_red_edge',[(side*.024,1.506),
                (side*.023,1.497),(side*.018,1.491)],red,.00065)
        _paint(m,'Reference_mask_nostril_cut',[(side*.006,1.535),
                (side*.010,1.534),(side*.013,1.537)],mats['leather'],.0004)
    cracks=[[( -.060,1.650),(-.045,1.640),(-.041,1.623),(-.030,1.617)],
            [(-.041,1.623),(-.053,1.617),(-.061,1.619)],
            [(.040,1.653),(.037,1.641),(.048,1.636),(.049,1.622)],
            [(.037,1.641),(.028,1.638)],
            [(-.065,1.560),(-.054,1.551),(-.058,1.539),(-.047,1.529)],
            [(.061,1.551),(.054,1.539),(.060,1.526)],
            [(-.028,1.488),(-.019,1.483),(-.017,1.474)],
            [(.019,1.479),(.012,1.482),(.004,1.476)],
            [(-.023,1.669),(-.028,1.658),(-.020,1.647)]]
    for coordinates in cracks:
        _paint(m,'Reference_mask_hairline_ceramic_crack',coordinates,mats['cloth_edge'],.00022)


def _lock(m, name, controls, width, depth, mat, center, strands=2):
    points=_sample(controls,21)
    verts=[];frames=[]
    for i,p in enumerate(points):
        t=i/(len(points)-1)
        tangent=(points[min(i+1,len(points)-1)]-points[max(0,i-1)]).normalized()
        normal=(p-Vector(center)).normalized()
        across=tangent.cross(normal).normalized()
        if across.length<.1:across=Vector((1,0,0))
        normal=across.cross(tangent).normalized()
        if normal.dot(p-Vector(center))<0:normal=-normal
        w=width*(.18+.80*math.sin(PI*t)**.55)*(1-t**3)+.00015
        d=depth*(.20+.80*math.sin(PI*t)**.55)*(1-t**4)+.00012
        frames.append((across,normal,w,d))
        for j in range(8):
            a=2*PI*j/8
            verts.append(p+across*w*math.cos(a)+normal*d*math.sin(a))
    faces=[tuple(reversed(range(8)))]
    faces.extend((i*8+j,i*8+(j+1)%8,(i+1)*8+(j+1)%8,(i+1)*8+j)
                 for i in range(len(points)-1) for j in range(8))
    faces.append(tuple((len(points)-1)*8+j for j in range(8)))
    _finish(m.mesh(name,verts,faces,mat))
    for strand in range(strands):
        s=(strand-(strands-1)/2)*.50
        out=[p+frame[0]*(frame[2]*s)+frame[1]*(frame[3]*math.sqrt(1-s*s)+.00015)
             for p,frame in zip(points[:-2],frames[:-2])]
        _stroke(m,'Hair_fine_directional_strand',out,.00025,mat)


def _hair(m, mats):
    hair,red=mats['hair'],mats['red'];rng=random.Random(9206)
    center=(0,.023,1.584)
    _finish(m.ico('Hair_swept_scalp_volume',center,(.086,.082,.105),hair,3))
    # Broad, overlapping swept clumps have lens sections and tapered ends.
    # These sit on the scalp rather than hanging as a ring of cylindrical locks.
    for i in range(17):
        x=-.076+i*.0095
        controls=[(x,-.035,1.680-.044*(abs(x)/.076)**1.5),
                  (x*1.04,-.052,1.708-.025*(abs(x)/.076)),
                  (x*.52,.018,1.726),(x*.15+.007,.049,1.741)]
        _lock(m,'Hair_swept_front_clump',controls,.0097,.0039,hair,center,3)
    for side in (-1,1):
        for i in range(7):
            controls=[(side*(.075-.004*i),.020+i*.007,1.647),
                      (side*(.088+.002*i),.026+i*.007,1.614),
                      (side*(.091+.002*i),.041+i*.006,1.556),
                      (side*(.080+.005*i),.061+i*.005,1.491+.010*(i%3))]
            _lock(m,'Hair_temple_swept_wisp',controls,.008,.003,hair,center,2)
    bun=(.007,.057,1.746)
    _finish(m.ico('Hair_high_bun_volume',(.007,.057,1.740),(.034,.032,.031),hair,3))
    for i in range(19):
        a=2*PI*i/19
        controls=[(.007+.038*math.cos(a),.057+.036*math.sin(a),1.718),
                  (.007+.058*math.cos(a+.45),.057+.054*math.sin(a+.45),1.752),
                  (.007+.039*math.cos(a+1.05),.057+.039*math.sin(a+1.05),1.785),
                  (.007+.012*math.cos(a+1.8),.057+.022*math.sin(a+1.8),1.779)]
        _lock(m,'Hair_bun_twisted_clump',controls,.0082,.0033,hair,bun,2)
    for i in range(12):
        a=rng.uniform(0,2*PI)
        points=_sample([(.007+.037*math.cos(a),.057+.035*math.sin(a),1.759),
                (.007+.050*math.cos(a),.057+.049*math.sin(a),1.792),
                (.007+.065*math.cos(a+.25),.057+.058*math.sin(a+.25),1.802-rng.random()*.014),
                (.007+.072*math.cos(a+.45),.057+.060*math.sin(a+.45),1.786-rng.random()*.009)],15)
        _finish(m.tube('Hair_bun_flyaway',points,
                       [.0011*(1-i/15)**1.5+.00005 for i in range(15)],hair,5))
    loop=[(.007+.046*math.cos(a),.057+.040*math.sin(a),1.722+.002*math.sin(2*a))
          for a in [2*PI*i/48 for i in range(49)]]
    _stroke(m,'Hair_tie_cinnabar_bun_binding',loop,.0035,red)
    _stroke(m,'Hair_tie_cinnabar_second_binding',[(x,y,z+.006) for x,y,z in loop],.0022,red)
    # Two asymmetrical hanging red cords and silk tassels are prominent in front.
    for end in [(-.154,-.012,1.574),(-.190,-.012,1.617)]:
        ex,ey,ez=end
        pts=_sample([(-.015,.015,1.728),(-.084,-.012,1.713),
                     (ex+.011,ey,ez+.068),end],22)
        _stroke(m,'Hair_tie_hanging_red_cord',pts,.0030,red)
        _finish(m.ico('Hair_tie_wrapped_tassel_knot',end,(.006,.005,.008),red,2))
        for i in range(17):
            a=2*PI*i/17;r=.0045
            points=[(ex+r*math.cos(a),ey+r*math.sin(a),ez-.006),
                    (ex+.008*math.cos(a),ey+.007*math.sin(a),ez-.023),
                    (ex+.012*math.cos(a),ey+.009*math.sin(a),ez-.051+rng.uniform(-.003,.003))]
            _finish(m.tube('Hair_tie_silk_tassel_fiber',points,[.0010,.00085,.0005],red,5))


def _scarf(m, mats):
    cloth=mats['cloth'];edge=mats['cloth_edge']
    rows,cols=21,97
    verts=[]
    for i in range(rows):
        v=i/(rows-1)
        for j in range(cols):
            a=2*PI*j/(cols-1)
            roll=.006*math.sin(v*PI*5+.65*math.sin(2*a))
            rx=.073+.063*math.sin(v*PI/2)+roll
            ry=.066+.054*math.sin(v*PI/2)+roll
            z=1.467-.114*v-.021*max(0,-math.sin(a))+.012*math.sin(a+v*3)
            z+=.003*math.sin(5*a+8*v)*math.sin(PI*v)
            verts.append((rx*math.cos(a),ry*math.sin(a)+.009,z))
    faces=[(i*cols+j,i*cols+j+1,(i+1)*cols+j+1,(i+1)*cols+j)
           for i in range(rows-1) for j in range(cols-1)]
    faces=[tuple(reversed(f)) for f in faces]
    _shell(m.mesh('Scarf_soft_draped_cowl',verts,faces,cloth),.0025,'chest')
    # A broad falling fold crosses the front; the hem is a folded textile edge.
    vertices=[]
    for i in range(41):
        t=i/40;x=-.107+.214*t
        for j in range(7):
            v=j/6
            width=.030*math.sin(PI*t)**.45+.003
            z=1.407-.020*t-.024*math.sin(PI*t)+(v-.5)*width
            y=-.119*math.sqrt(max(.03,1-(x/.138)**2))-.005*math.sin(PI*t)-.003*math.sin(PI*v)
            vertices.append((x,y,z))
    _shell(m.mesh('Scarf_crossing_front_fold',vertices,
            [(i*7+j,i*7+j+1,(i+1)*7+j+1,(i+1)*7+j)
             for i in range(40) for j in range(6)],cloth),.002,'chest')
    _stroke(m,'Scarf_folded_selvedge',[vertices[i*7] for i in range(41)],.0009,edge,'chest')
    # The back overlap lays on the shoulder, with restrained worn edge fibers.
    tail=[(-.062,.100,1.421),(-.032,.143,1.381),(-.007,.128,1.282)]
    _finish(m.ribbon('Scarf_short_back_overlap',tail,.058,cloth),'chest')
    for i in range(9):
        x=-.032+i*.006
        _finish(m.tube('Scarf_short_back_fray',[(x,.128,1.284),
                     (x+.003,.13,1.274-(i%3)*.003)], [.0008,.00015],edge,5),'chest')


def build(m, mats):
    """Build only the new reference head and scarf, retaining the existing rig."""
    _face(m,mats)
    _hair(m,mats)
    _scarf(m,mats)
    print('REFERENCE_SHIROTSURA_HEAD_PASS')
