"""Explicit UE materials for the shared baked atlases; independent of FBX shader translation."""
import unreal
import json
from pathlib import Path

root=Path(unreal.Paths.project_dir()).resolve()
asset_tools=unreal.AssetToolsHelpers.get_asset_tools()
lib=unreal.MaterialEditingLibrary
for name in ['Shirotsura','Ishibashiri']:
    dest='/Game/Characters/Rigged/'+name
    folder=root/'Art/Characters'/name/'Rigged'
    textures={}
    for kind in ['BaseColor','Normal']:
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
        for kind,prop in [('BaseColor',unreal.MaterialProperty.MP_BASE_COLOR),('Normal',unreal.MaterialProperty.MP_NORMAL)]:
            sample=lib.create_material_expression(mat,unreal.MaterialExpressionTextureSample,-450,0 if kind=='BaseColor' else 220)
            sample.set_editor_property('texture',textures[kind])
            sample.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if kind=='BaseColor' else unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
            lib.connect_material_property(sample,'RGB',prop)
        rough=lib.create_material_expression(mat,unreal.MaterialExpressionConstant,-220,420)
        rough.set_editor_property('r',.84)
        lib.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
        if 'crimson' in label or 'ember' in label:
            emissive=lib.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-220,550)
            emissive.set_editor_property('constant',unreal.LinearColor(.75,.008,.004,1))
            lib.connect_material_property(emissive,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        if 'blade' in label or 'sharpened' in label:
            rough.set_editor_property('r',.28)
            metal=lib.create_material_expression(mat,unreal.MaterialExpressionConstant,-220,620)
            metal.set_editor_property('r',.85)
            lib.connect_material_property(metal,'',unreal.MaterialProperty.MP_METALLIC)
        lib.recompile_material(mat)
        slot.set_editor_property('material_interface',mat)
        slots[i]=slot
    mesh.set_editor_property('materials',slots)
    assert all('M_Baked_' in s.material_interface.get_name() for s in mesh.get_editor_property('materials'))
    unreal.EditorAssetLibrary.save_directory(dest,only_if_is_dirty=False,recursive=True)
unreal.log('BAKED_MATERIALS_APPLIED')
