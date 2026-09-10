"""Explicit UE materials for the shared baked atlases; independent of FBX shader translation."""
import unreal
import json
from pathlib import Path

root=Path(unreal.Paths.project_dir()).resolve()
asset_tools=unreal.AssetToolsHelpers.get_asset_tools()
lib=unreal.MaterialEditingLibrary

def expression(mat,kind,x,y,ordinal=0):
    """Reuse the first expression of a type so reruns cannot grow the graph."""
    expressions=mat.get_editor_property('expressions')
    found=[e for e in expressions if isinstance(e,kind)]
    return found[ordinal] if len(found)>ordinal else lib.create_material_expression(mat,kind,x,y)

def surface_values(label):
    # Atlas color/normal is shared, but tactile response remains material-specific.
    if any(k in label for k in ['granite','stone','mineral','petrified']): return .91,0.0
    if any(k in label for k in ['cloth','linen','straw','paper','moss','lichen','repair']): return .96,0.0
    if any(k in label for k in ['hide','skin','hand','bristle','hair','root']): return .78,0.0
    if any(k in label for k in ['blade','sharpened']): return .28,.85
    return .86,0.0
for name in ['Shirotsura','Ishibashiri']:
    dest='/Game/Characters/Rigged/'+name
    folder=root/'Art/Characters'/name/'Rigged'
    textures={}
    for kind in ['BaseColor','Normal','Roughness']:
        t=unreal.AssetImportTask();t.filename=str(folder/('T_'+name+'_'+kind+'.png'))
        t.destination_path=dest;t.destination_name='T_'+name+'_'+kind
        t.automated=True;t.replace_existing=True;t.save=True
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
                {'BaseColor':0,'Normal':1,'Roughness':2}[kind])
            sample.set_editor_property('texture',textures[kind])
            sample.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if kind=='BaseColor' else (unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if kind=='Normal' else unreal.MaterialSamplerType.SAMPLERTYPE_MASKS))
            lib.connect_material_property(sample,'R' if kind=='Roughness' else 'RGB',prop)
        roughness,metallic=surface_values(label)
        if 'crimson' in label or 'ember' in label:
            emissive=expression(mat,unreal.MaterialExpressionConstant3Vector,-220,550)
            emissive.set_editor_property('constant',unreal.LinearColor(.30,.003,.002,1))
            lib.connect_material_property(emissive,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        if metallic:
            metal=expression(mat,unreal.MaterialExpressionConstant,-220,620,1)
            metal.set_editor_property('r',metallic)
            lib.connect_material_property(metal,'',unreal.MaterialProperty.MP_METALLIC)
        lib.recompile_material(mat)
        slot.set_editor_property('material_interface',mat)
        slots[i]=slot
    mesh.set_editor_property('materials',slots)
    assert all('M_Baked_' in s.material_interface.get_name() for s in mesh.get_editor_property('materials'))
    unreal.EditorAssetLibrary.save_directory(dest,only_if_is_dirty=False,recursive=True)
unreal.log('BAKED_MATERIALS_APPLIED')
