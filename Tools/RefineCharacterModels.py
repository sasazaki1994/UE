"""Weathered organic refinement of the original studies; no external mesh assets."""
import bpy
import math
import random
import importlib.util
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('maker',ROOT/'Tools/CreateCharacterModels.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
random.seed(3221)

def apply(ob,mod):
    bpy.context.view_layer.objects.active=ob
    bpy.ops.object.modifier_apply(modifier=mod.name)

def finish(name):
    path=ROOT/'Art/Characters'/name/(name+'.blend')
    bpy.ops.wm.open_mainfile(filepath=str(path))
    bpy.context.preferences.filepaths.save_version=0
    hero=name=='Shirotsura'
    obs=[o for o in bpy.context.scene.objects if o.type=='MESH' and not o.name.startswith('STUDIO')]
    # Bindable, rounded organic masses with small irregularities, angular stone retained.
    for ob in obs:
        label=ob.name.lower()
        if hero:
            smooth=any(k in label for k in ['trouser','jacket','sleeve','hair_mass','hair_knot','boot','corrupted','forearm','right_hand'])
        else:
            smooth=any(k in label for k in ['barrel','hump','lowered_head','muzzle','nose','leg_muscle'])
        if smooth:
            sub=ob.modifiers.new('Organic surface','SUBSURF');sub.levels=2 if hero else 1
            apply(ob,sub)
            for p in ob.data.polygons:p.use_smooth=True
        if not hero and any(k in label for k in ['mountain','shoulder_crown','terrace','monument_tusk','forehead_prayer']):
            sub=ob.modifiers.new('Eroded strata','SUBSURF');sub.subdivision_type='SIMPLE';sub.levels=2
            apply(ob,sub)
            tex=bpy.data.textures.new('Stone relief','CLOUDS');tex.noise_scale=.23;tex.noise_depth=2
            dis=ob.modifiers.new('Broken granite','DISPLACE');dis.texture=tex;dis.strength=.09;dis.texture_coords='GLOBAL'
            apply(ob,dis)
            bevel=ob.modifiers.new('Worn ridges','BEVEL');bevel.width=.045;bevel.segments=2;bevel.angle_limit=.55
            apply(ob,bevel)
            for p in ob.data.polygons:p.use_smooth=True
            ob.data.use_auto_smooth=True;ob.data.auto_smooth_angle=math.radians(42)
    mats={o.data.materials[0].name:o.data.materials[0] for o in obs if o.data.materials}
    # Position-based layers bake consistently into UV maps for UE.
    for mat in mats.values():
        if not mat.use_nodes:continue
        nt=mat.node_tree;p=nt.nodes.get('Principled BSDF')
        if not p:continue
        label=mat.name.lower();base=tuple(p.inputs['Base Color'].default_value)[:3]
        if any(k in label for k in ['crimson','emissive','sharpened','boundary']):continue
        if 'granite' in label:base=tuple(c*.55 for c in base)
        if 'moss' in label or 'lichen' in label:base=tuple(c*.62 for c in base)
        coord=nt.nodes.new('ShaderNodeTexCoord')
        noise=nt.nodes.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=18 if hero else 2.8
        noise.inputs['Detail'].default_value=5;noise.inputs['Roughness'].default_value=.78
        nt.links.new(coord.outputs['Object'],noise.inputs['Vector'])
        ramp=nt.nodes.new('ShaderNodeValToRGB')
        ramp.color_ramp.elements[0].position=.20;ramp.color_ramp.elements[1].position=.80
        ramp.color_ramp.elements[0].color=(*[c*.40 for c in base],1)
        ramp.color_ramp.elements[1].color=(*[min(1,c*1.40) for c in base],1)
        nt.links.new(noise.outputs['Fac'],ramp.inputs[0]);nt.links.new(ramp.outputs[0],p.inputs['Base Color'])
        grain=nt.nodes.new('ShaderNodeTexNoise');grain.inputs['Scale'].default_value=450 if hero else 36
        grain.inputs['Detail'].default_value=3
        nt.links.new(coord.outputs['Object'],grain.inputs['Vector'])
        bump=nt.nodes.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.48
        bump.inputs['Distance'].default_value=.0018 if hero else .038
        nt.links.new(grain.outputs['Fac'],bump.inputs['Height']);nt.links.new(bump.outputs[0],p.inputs['Normal'])
        p.inputs['Roughness'].default_value=.84
    if hero:
        # Fine, irregular cloth creases and a more anatomical human hand.
        cloth=next(v for k,v in mats.items() if 'worn fold' in k)
        skin=next(v for k,v in mats.items() if 'exposed' in k)
        for i in range(16):
            x=random.uniform(-.16,.16);z=random.uniform(1.0,1.27)
            m.curve('Cloth_fine_crease',[(x,-.113,z),(x+.017,-.115,z-.045),(x-.003,-.12,z-.1)],.0013,cloth)
        for i in range(4):
            x=-.346+i*.013
            m.tube('Right_hand_finger',[(x,-.043,.952),(x,-.065,.927),(x+.005,-.077,.942)],[.009,.007,.006],skin,8)
    else:
        fur=next(v for k,v in mats.items() if 'bristles' in k)
        # Coarse hair tufts around the neck and belly, tapered and laid backward.
        for i in range(220):
            y=random.uniform(-3.4,3.9);a=random.choice([-1,1])*random.uniform(.65,1.35)
            x=2.42*math.sin(a);z=4.1-1.65*math.cos(a)
            if y< -2:x*=.8;z-=.25
            length=random.uniform(.18,.5)
            m.tube('Weathered_fur',[(x,y,z+.10),(x*1.03,y+.13,z-.08),(x*1.03,y+.26,z-length)],
                [random.uniform(.025,.055),.023,.001],fur,5)
    # Convert new curves before handing the asset to the skeleton pipeline.
    objects=[o for o in bpy.context.scene.objects if o.type in ['MESH','CURVE'] and not o.name.startswith('STUDIO')]
    m.export(name,objects,ROOT/'Art/Characters'/name)
    scene=bpy.context.scene;scene.cycles.samples=48
    scene.view_settings.look='Medium High Contrast'
    bpy.ops.wm.save_as_mainfile(filepath=str(path))
    m.render(ROOT/'Art/Characters/Previews'/(name+'.png'))
    print('REFINEMENT_PASS',name)

finish('Shirotsura')
finish('Ishibashiri')
