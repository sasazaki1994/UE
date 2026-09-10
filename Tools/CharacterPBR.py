"""Blender helpers that layer verified microsurface maps before atlas baking."""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];MANIFEST=ROOT/'Art/Materials/CC0PBR/manifest.json'
KEYWORDS={'weathered_stone':('granite',),'aged_whitewood':('whitewood',),'coarse_indigo_cloth':('indigo_work_cloth',)}
TINTS={'weathered_stone':(.42,.43,.39,1),'aged_whitewood':(.82,.72,.55,1),'coarse_indigo_cloth':(.035,.075,.16,1)}
def role_for_material(name):
    label=name.lower();return next((r for r,words in KEYWORDS.items() if any(w in label for w in words)),None)
def apply_external_pbr(materials,bpy):
    data=json.loads(MANIFEST.read_text(encoding='utf-8'))
    if not data.get('license',{}).get('verified'):
        print('PBR_CACHE_UNAVAILABLE: retaining procedural materials');return
    assets={a['role']:a for a in data['assets']}
    for mat in materials:
        role=role_for_material(mat.name)
        if role not in assets:continue
        nodes,links=mat.node_tree.nodes,mat.node_tree.links;p=nodes.get('Principled BSDF')
        for node in list(nodes):
            if node.name.startswith('ExternalPBR_'):nodes.remove(node)
        uv=nodes.new('ShaderNodeUVMap');uv.name='ExternalPBR_Coordinates';uv.uv_map='PBRDetailUV'
        mapping=nodes.new('ShaderNodeMapping');mapping.name='ExternalPBR_Mapping';scale={'weathered_stone':4,'aged_whitewood':7,'coarse_indigo_cloth':18}[role]
        mapping.inputs['Scale'].default_value=(scale,scale,scale);links.new(uv.outputs['UV'],mapping.inputs['Vector']);loaded={}
        for kind in ('base_color','normal_gl','roughness'):
            image=bpy.data.images.load(str(ROOT/assets[role]['maps'][kind]['path']),check_existing=True)
            if kind!='base_color':image.colorspace_settings.name='Non-Color'
            node=nodes.new('ShaderNodeTexImage');node.name='ExternalPBR_'+kind;node.image=image;links.new(mapping.outputs['Vector'],node.inputs['Vector']);loaded[kind]=node
        tint=nodes.new('ShaderNodeMixRGB');tint.name='ExternalPBR_DesignTint';tint.blend_type='MULTIPLY';tint.inputs[0].default_value=.72;tint.inputs[2].default_value=TINTS[role]
        links.new(loaded['base_color'].outputs['Color'],tint.inputs[1]);links.new(tint.outputs['Color'],p.inputs['Base Color'])
        normal=nodes.new('ShaderNodeNormalMap');normal.name='ExternalPBR_NormalGL';normal.inputs['Strength'].default_value=.32
        links.new(loaded['normal_gl'].outputs['Color'],normal.inputs['Color']);links.new(normal.outputs['Normal'],p.inputs['Normal']);links.new(loaded['roughness'].outputs['Color'],p.inputs['Roughness'])
