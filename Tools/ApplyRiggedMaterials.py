"""Explicit UE materials for the shared baked atlases; independent of FBX shader translation.

Set CHARACTER_ASSET_FILTER to Shirotsura or Ishibashiri to update only that model.
"""
import unreal
import os
from pathlib import Path

asset_filter = os.environ.get('CHARACTER_ASSET_FILTER', '').strip()
if asset_filter and asset_filter not in ('Shirotsura', 'Ishibashiri'):
    raise ValueError('CHARACTER_ASSET_FILTER must be Shirotsura or Ishibashiri, or unset for both')
characters = (asset_filter,) if asset_filter else ('Shirotsura', 'Ishibashiri')

root=Path(unreal.Paths.project_dir()).resolve()
asset_tools=unreal.AssetToolsHelpers.get_asset_tools()
lib=unreal.MaterialEditingLibrary

def expression(mat,kind,x,y,prop,tag):
    """Reuse the property's node through the public UE 5.6 material API."""
    found=lib.get_material_property_input_node(mat,prop)
    if isinstance(found,kind):
        found.set_editor_property('desc',tag)
        return found
    for node in lib.get_all_material_expressions(mat):
        if isinstance(node,kind) and str(node.get_editor_property('desc')) == tag:
            return node
    node=lib.create_material_expression(mat,kind,x,y)
    node.set_editor_property('desc',tag)
    return node

def configure_shirotsura_corruption(mat, base_sample, mask_sample=None):
    """Add the one audited stage parameter without inventing atlas UV masks."""
    tag='SHIROTSURA_CORRUPTION_'
    early=expression(mat,unreal.MaterialExpressionConstant3Vector,-210,-80,
        unreal.MaterialProperty.MP_BASE_COLOR,tag+'EARLY_COLOR')
    early.set_editor_property('constant',unreal.LinearColor(.16,.105,.085,1))
    amount=expression(mat,unreal.MaterialExpressionScalarParameter,-210,20,
        unreal.MaterialProperty.MP_BASE_COLOR,tag+'AMOUNT')
    amount.set_editor_property('parameter_name','ShirotsuraCorruptionIntensity')
    amount.set_editor_property('default_value',1.0)
    blend=expression(mat,unreal.MaterialExpressionLinearInterpolate,40,0,
        unreal.MaterialProperty.MP_BASE_COLOR,tag+'BLEND')
    alpha=amount
    if mask_sample:
        # A zero mask must be an exact pass-through for the right hand and mask.
        lib.connect_material_expressions(base_sample,'RGB',blend,'A')
        lib.connect_material_expressions(early,'',blend,'B')
        alpha=expression(mat,unreal.MaterialExpressionMultiply,-80,100,
            unreal.MaterialProperty.MP_BASE_COLOR,tag+'MASKED_AMOUNT')
        lib.connect_material_expressions(mask_sample,'R',alpha,'A')
        lib.connect_material_expressions(amount,'',alpha,'B')
    else:
        lib.connect_material_expressions(early,'',blend,'A')
        lib.connect_material_expressions(base_sample,'RGB',blend,'B')
    lib.connect_material_expressions(alpha,'',blend,'Alpha')
    lib.connect_material_property(blend,'',unreal.MaterialProperty.MP_BASE_COLOR)

def surface_values(label):
    # Atlas color/normal is shared, but tactile response remains material-specific.
    if any(k in label for k in ['bronze','bell']): return .42,.78
    if any(k in label for k in ['iron','fittings']): return .56,.72
    if 'leather' in label: return .82,0.0
    if any(k in label for k in ['granite','stone','mineral','petrified']): return .91,0.0
    if any(k in label for k in ['cloth','linen','straw','paper','moss','lichen','repair']): return .96,0.0
    if any(k in label for k in ['hide','skin','hand','bristle','hair','root']): return .78,0.0
    if any(k in label for k in ['blade','sharpened']): return .28,.85
    return .86,0.0
def apply_character_materials(name, dest, folder, replace_existing=True):
    textures={}
    for kind in ['BaseColor','Normal','Roughness']:
        t=unreal.AssetImportTask();t.filename=str(folder/('T_'+name+'_'+kind+'.png'))
        t.destination_path=dest;t.destination_name='T_'+name+'_'+kind
        t.automated=True;t.replace_existing=replace_existing;t.save=True
        asset_tools.import_asset_tasks([t])
        tex=unreal.load_asset(dest+'/T_'+name+'_'+kind)
        if not tex:raise RuntimeError('Missing atlas '+kind)
        tex.set_editor_property('srgb',kind=='BaseColor')
        if kind=='Normal':
            tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property('flip_green_channel',True)
        elif kind=='Roughness':
            tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_MASKS)
        textures[kind]=tex
    mask_path=folder/('T_'+name+'_FaceNeckMask.png')
    if name=='Shirotsura' and mask_path.exists():
        t=unreal.AssetImportTask();t.filename=str(mask_path)
        t.destination_path=dest;t.destination_name='T_'+name+'_FaceNeckMask'
        t.automated=True;t.replace_existing=replace_existing;t.save=True
        asset_tools.import_asset_tasks([t])
        textures['FaceNeckMask']=unreal.load_asset(dest+'/T_'+name+'_FaceNeckMask')
        textures['FaceNeckMask'].set_editor_property('srgb',False)
        textures['FaceNeckMask'].set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_MASKS)
    mesh=unreal.load_asset(dest+'/SK_'+name)
    slots=mesh.get_editor_property('materials')
    for i,slot in enumerate(slots):
        label=str(slot.material_slot_name).lower()
        mat_path=dest+'/M_Baked_'+str(i)
        mat=unreal.load_asset(mat_path) or asset_tools.create_asset('M_Baked_'+str(i),dest,unreal.Material,unreal.MaterialFactoryNew())
        mat.set_editor_property('used_with_skeletal_mesh',True)
        # These materials can already be rooted by the character CDO. Deleting
        # rooted expressions asserts in UE 5.6; reconnect new expressions safely.
        for kind,prop in [('BaseColor',unreal.MaterialProperty.MP_BASE_COLOR),('Normal',unreal.MaterialProperty.MP_NORMAL),('Roughness',unreal.MaterialProperty.MP_ROUGHNESS)]:
            sample=expression(mat,unreal.MaterialExpressionTextureSample,-450,{'BaseColor':0,'Normal':220,'Roughness':440}[kind],
                prop,'BAKED_'+kind.upper())
            sample.set_editor_property('texture',textures[kind])
            sample.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if kind=='BaseColor' else (unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if kind=='Normal' else unreal.MaterialSamplerType.SAMPLERTYPE_MASKS))
            lib.connect_material_property(sample,'R' if kind=='Roughness' else 'RGB',prop)
            if name=='Shirotsura' and label=='09 • petrified corruption' and kind=='BaseColor':
                configure_shirotsura_corruption(mat,sample)
            if name=='Shirotsura' and label=='05 • exposed right hand' and kind=='BaseColor' and 'FaceNeckMask' in textures:
                mask=expression(mat,unreal.MaterialExpressionTextureSample,-450,120,
                    unreal.MaterialProperty.MP_BASE_COLOR,'SHIROTSURA_FACE_NECK_MASK')
                mask.set_editor_property('texture',textures['FaceNeckMask'])
                mask.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
                configure_shirotsura_corruption(mat,sample,mask)
        roughness,metallic=surface_values(label)
        # Numeric material names survive a mesh reimport even when slot labels
        # change. Assign both properties for every slot to clear old responses.
        emissive=expression(mat,unreal.MaterialExpressionConstant3Vector,-220,550,unreal.MaterialProperty.MP_EMISSIVE_COLOR,'BAKED_EMISSIVE')
        glow=unreal.LinearColor(.30,.003,.002,1) if 'crimson' in label or 'ember' in label else unreal.LinearColor(0,0,0,1)
        emissive.set_editor_property('constant',glow)
        lib.connect_material_property(emissive,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        metal=expression(mat,unreal.MaterialExpressionConstant,-220,620,unreal.MaterialProperty.MP_METALLIC,'BAKED_METALLIC')
        metal.set_editor_property('r',metallic)
        lib.connect_material_property(metal,'',unreal.MaterialProperty.MP_METALLIC)
        lib.recompile_material(mat)
        slot.set_editor_property('material_interface',mat)
        slots[i]=slot
    mesh.set_editor_property('materials',slots)
    assert all('M_Baked_' in s.material_interface.get_name() for s in mesh.get_editor_property('materials'))
    unreal.EditorAssetLibrary.save_directory(dest,only_if_is_dirty=False,recursive=True)
def main():
    for name in characters:
        apply_character_materials(name, '/Game/Characters/Rigged/'+name, root/'Art/Characters'/name/'Rigged')
    unreal.log('BAKED_MATERIALS_APPLIED')


if __name__ == '__main__':
    main()
