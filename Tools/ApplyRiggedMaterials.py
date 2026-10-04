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
GRAPH_PROPERTIES=(unreal.MaterialProperty.MP_BASE_COLOR,unreal.MaterialProperty.MP_NORMAL,
    unreal.MaterialProperty.MP_ROUGHNESS,unreal.MaterialProperty.MP_METALLIC,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
known_nodes={}

def remember_graph(mat):
    """Snapshot reachable nodes before reconnecting; UE 5.6 Python cannot list every expression."""
    pending=[lib.get_material_property_input_node(mat,prop) for prop in GRAPH_PROPERTIES]
    nodes=[]
    while pending:
        node=pending.pop()
        if node is None or any(node==seen for seen in nodes):
            continue
        nodes.append(node)
        pending.extend(lib.get_inputs_for_material_expression(mat,node))
    known_nodes[mat.get_path_name()]=nodes

def find_tagged(mat,kind,tag):
    return next((node for node in known_nodes.get(mat.get_path_name(),())
                 if isinstance(node,kind) and str(node.get_editor_property('desc')) == tag),None)

def create_tagged(mat,kind,x,y,tag):
    node=lib.create_material_expression(mat,kind,x,y)
    node.set_editor_property('desc',tag)
    known_nodes.setdefault(mat.get_path_name(),[]).append(node)
    return node

def expression(mat,kind,x,y,prop,tag):
    """Reuse the property's node through the public UE 5.6 material API."""
    found=lib.get_material_property_input_node(mat,prop)
    if isinstance(found,kind):
        found.set_editor_property('desc',tag)
        return found
    return find_tagged(mat,kind,tag) or create_tagged(mat,kind,x,y,tag)

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

DETAIL_SOURCE=root/'Art/Characters/Detail/Generated'
DETAIL_DEST='/Game/Characters/Detail'
DETAIL_FUNCTION='MF_CharacterTriplanarDetail'
# label keyword -> (detail set, tile size in pre-skinned cm, colour strength, normal strength).
# Corruption, skin, cord and blade slots keep their authored atlas response untouched.
SURFACE_DETAIL={
    'Ishibashiri':(
        ('umber hide',('bristle',140,.6,.9)),
        ('charcoal bristles',('bristle',90,.7,1.0)),
        ('granite',('rock_face',260,.55,.8)),
        ('moss cushion',('mossy_rock',160,.45,.7)),
        ('dry lichen',('lichen_rock',160,.55,.7)),
        ('weathered straw',('thatch_roof_angled',120,.5,.8)),
        ('blighted root',('japanese_cedar_bark',150,.5,.8)),
        ('paper offerings',('rough_linen',60,.4,.5)),
    ),
    'Shirotsura':(
        ('indigo',('denim_fabric',14,.35,.6)),
        ('worn fold edges',('denim_fabric',14,.35,.6)),
        ('charcoal',('denim_fabric',14,.3,.5)),
        ('unbleached linen',('rough_linen',18,.4,.6)),
        ('rice straw',('thatch_roof_angled',30,.45,.7)),
        ('aged whitewood mask',('fine_grained_wood',30,.35,.45)),
        ('black hair',('bristle',10,.4,.6)),
    ),
}

def tagged(mat,kind,x,y,tag):
    """Find detail nodes by tag only; property lookups would return the final multiply."""
    return find_tagged(mat,kind,tag) or create_tagged(mat,kind,x,y,tag)

def mask(function,source,channels,x,y):
    node=lib.create_material_expression_in_function(function,unreal.MaterialExpressionComponentMask,x,y)
    for channel in 'rgba':
        node.set_editor_property(channel,channel in channels)
    lib.connect_material_expressions(source,'',node,'')
    return node

def triplanar_detail_function():
    """Project detail on pre-skinned local space so it stays on the body while bones move."""
    path=DETAIL_DEST+'/'+DETAIL_FUNCTION
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    function=asset_tools.create_asset(DETAIL_FUNCTION,DETAIL_DEST,unreal.MaterialFunction,unreal.MaterialFunctionFactoryNew())
    new=lambda kind,x,y: lib.create_material_expression_in_function(function,kind,x,y)
    inputs={}
    for order,(name,kind) in enumerate([('DetailTexture',unreal.FunctionInputType.FUNCTION_INPUT_TEXTURE2D),
            ('NormalTexture',unreal.FunctionInputType.FUNCTION_INPUT_TEXTURE2D),
            ('TileCm',unreal.FunctionInputType.FUNCTION_INPUT_SCALAR)]):
        node=new(unreal.MaterialExpressionFunctionInput,-1400,order*160)
        node.set_editor_property('input_name',name); node.set_editor_property('input_type',kind)
        node.set_editor_property('sort_priority',order); inputs[name]=node
    # Pre-skinned attributes only exist in the vertex shader.
    position=new(unreal.MaterialExpressionVertexInterpolator,-1200,500)
    lib.connect_material_expressions(new(unreal.MaterialExpressionPreSkinnedPosition,-1400,500),'',position,'')
    normal=new(unreal.MaterialExpressionVertexInterpolator,-1200,700)
    lib.connect_material_expressions(new(unreal.MaterialExpressionPreSkinnedNormal,-1400,700),'',normal,'')
    scaled=new(unreal.MaterialExpressionDivide,-1000,500)
    lib.connect_material_expressions(position,'',scaled,'A'); lib.connect_material_expressions(inputs['TileCm'],'',scaled,'B')
    # V follows local Y on the side and top planes: the boar's body length and the mask's grain.
    side=new(unreal.MaterialExpressionAppendVector,-650,350)
    lib.connect_material_expressions(mask(function,scaled,'b',-850,320),'',side,'A')
    lib.connect_material_expressions(mask(function,scaled,'g',-850,400),'',side,'B')
    planes=[side,mask(function,scaled,'rb',-650,480),mask(function,scaled,'rg',-650,560)]
    absolute=new(unreal.MaterialExpressionAbs,-1000,700)
    lib.connect_material_expressions(normal,'',absolute,'')
    sharpened=new(unreal.MaterialExpressionPower,-850,700)
    sharpened.set_editor_property('const_exponent',4.0)
    lib.connect_material_expressions(absolute,'',sharpened,'Base')
    ones=new(unreal.MaterialExpressionConstant3Vector,-850,800)
    ones.set_editor_property('constant',unreal.LinearColor(1,1,1,1))
    total=new(unreal.MaterialExpressionDotProduct,-700,780)
    lib.connect_material_expressions(sharpened,'',total,'A'); lib.connect_material_expressions(ones,'',total,'B')
    weights=new(unreal.MaterialExpressionDivide,-550,720)
    lib.connect_material_expressions(sharpened,'',weights,'A'); lib.connect_material_expressions(total,'',weights,'B')
    weight=[mask(function,weights,c,-400,680+i*80) for i,c in enumerate('rgb')]
    for row,(name,sampler,output) in enumerate([('DetailTexture',unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR,'Detail'),
            ('NormalTexture',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,'Normal')]):
        blended=None
        for i,plane in enumerate(planes):
            sample=new(unreal.MaterialExpressionTextureSample,-250,row*500+i*150)
            sample.set_editor_property('sampler_type',sampler)
            lib.connect_material_expressions(plane,'',sample,'UVs')
            lib.connect_material_expressions(inputs[name],'',sample,'Tex')
            term=new(unreal.MaterialExpressionMultiply,0,row*500+i*150)
            lib.connect_material_expressions(sample,'RGB',term,'A'); lib.connect_material_expressions(weight[i],'',term,'B')
            if blended:
                summed=new(unreal.MaterialExpressionAdd,200,row*500+i*150)
                lib.connect_material_expressions(blended,'',summed,'A'); lib.connect_material_expressions(term,'',summed,'B')
                term=summed
            blended=term
        result=new(unreal.MaterialExpressionFunctionOutput,450,row*500)
        result.set_editor_property('output_name',output)
        lib.connect_material_expressions(blended,'',result,'')
    lib.update_material_function(function)
    unreal.EditorAssetLibrary.save_loaded_asset(function)
    return function

def detail_textures(key,cache):
    if key in cache:
        return cache[key]
    pair=[]
    for kind in ('Detail','Normal'):
        t=unreal.AssetImportTask();t.filename=str(DETAIL_SOURCE/('T_CD_'+key+'_'+kind+'.png'))
        t.destination_path=DETAIL_DEST;t.destination_name='T_CD_'+key+'_'+kind
        t.automated=True;t.replace_existing=True;t.save=True
        asset_tools.import_asset_tasks([t])
        tex=unreal.load_asset(DETAIL_DEST+'/T_CD_'+key+'_'+kind)
        if not tex:raise RuntimeError('Missing detail texture '+key+' '+kind)
        tex.set_editor_property('srgb',False)
        if kind=='Normal':
            tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property('flip_green_channel',True)
        else:
            # Import-time normal-map detection can misfire on pale neutral maps.
            tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_DEFAULT)
        unreal.EditorAssetLibrary.save_loaded_asset(tex)
        pair.append(tex)
    cache[key]=tuple(pair)
    return cache[key]

def surface_detail(name,label):
    words=label.replace('_',' ')
    return next((spec for keyword,spec in SURFACE_DETAIL[name] if keyword in words),None)

def configure_surface_detail(mat,spec,base_sample,normal_sample,function,cache):
    """Multiply a neutral (0.5 mean) detail into the atlas and whiteout-blend its normal."""
    key,tile,colour_strength,normal_strength=spec
    colour_texture,normal_texture=detail_textures(key,cache)
    call=tagged(mat,unreal.MaterialExpressionMaterialFunctionCall,-900,900,'DETAIL_TRIPLANAR')
    call.set_editor_property('material_function',function)
    for pin,texture,y in (('DetailTexture',colour_texture,820),('NormalTexture',normal_texture,900)):
        node=tagged(mat,unreal.MaterialExpressionTextureObject,-1150,y,'DETAIL_'+pin.upper())
        node.set_editor_property('texture',texture)
        if pin=='NormalTexture':
            node.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        lib.connect_material_expressions(node,'',call,pin)
    size=tagged(mat,unreal.MaterialExpressionConstant,-1150,980,'DETAIL_TILE_CM')
    size.set_editor_property('r',float(tile))
    lib.connect_material_expressions(size,'',call,'TileCm')
    doubled=tagged(mat,unreal.MaterialExpressionMultiply,-650,820,'DETAIL_DOUBLED')
    doubled.set_editor_property('const_b',2.0)
    lib.connect_material_expressions(call,'Detail',doubled,'A')
    amount=tagged(mat,unreal.MaterialExpressionLinearInterpolate,-450,820,'DETAIL_COLOUR_AMOUNT')
    amount.set_editor_property('const_a',1.0); amount.set_editor_property('const_alpha',float(colour_strength))
    lib.connect_material_expressions(doubled,'',amount,'B')
    colour=tagged(mat,unreal.MaterialExpressionMultiply,-200,0,'DETAIL_BASECOLOR')
    lib.connect_material_expressions(base_sample,'RGB',colour,'A'); lib.connect_material_expressions(amount,'',colour,'B')
    lib.connect_material_property(colour,'',unreal.MaterialProperty.MP_BASE_COLOR)
    flat=tagged(mat,unreal.MaterialExpressionConstant3Vector,-650,980,'DETAIL_FLAT_NORMAL')
    flat.set_editor_property('constant',unreal.LinearColor(0,0,1,1))
    detail=tagged(mat,unreal.MaterialExpressionLinearInterpolate,-450,980,'DETAIL_NORMAL_AMOUNT')
    detail.set_editor_property('const_alpha',float(normal_strength))
    lib.connect_material_expressions(flat,'',detail,'A'); lib.connect_material_expressions(call,'Normal',detail,'B')
    # Whiteout blend: add the slopes, multiply the heights, renormalize.
    nodes={}
    for tag,source,channels,x,y in (('BASE_XY',normal_sample,'rg',-300,220),('BASE_Z',normal_sample,'b',-300,300),
            ('DETAIL_XY',detail,'rg',-300,980),('DETAIL_Z',detail,'b',-300,1060)):
        node=tagged(mat,unreal.MaterialExpressionComponentMask,x,y,'DETAIL_MASK_'+tag)
        for channel in 'rgba':
            node.set_editor_property(channel,channel in channels)
        lib.connect_material_expressions(source,'RGB' if source is normal_sample else '',node,'')
        nodes[tag]=node
    slopes=tagged(mat,unreal.MaterialExpressionAdd,-150,400,'DETAIL_NORMAL_XY')
    lib.connect_material_expressions(nodes['BASE_XY'],'',slopes,'A'); lib.connect_material_expressions(nodes['DETAIL_XY'],'',slopes,'B')
    heights=tagged(mat,unreal.MaterialExpressionMultiply,-150,480,'DETAIL_NORMAL_Z')
    lib.connect_material_expressions(nodes['BASE_Z'],'',heights,'A'); lib.connect_material_expressions(nodes['DETAIL_Z'],'',heights,'B')
    joined=tagged(mat,unreal.MaterialExpressionAppendVector,0,440,'DETAIL_NORMAL_JOIN')
    lib.connect_material_expressions(slopes,'',joined,'A'); lib.connect_material_expressions(heights,'',joined,'B')
    blended=tagged(mat,unreal.MaterialExpressionNormalize,120,440,'DETAIL_NORMAL')
    lib.connect_material_expressions(joined,'',blended,'')
    lib.connect_material_property(blended,'',unreal.MaterialProperty.MP_NORMAL)

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
    # Generated detail is optional until PrepareCharacterDetailTextures.py has run.
    detail_function=triplanar_detail_function() if DETAIL_SOURCE.is_dir() else None
    detail_cache={}
    for i,slot in enumerate(slots):
        label=str(slot.material_slot_name).lower()
        mat_path=dest+'/M_Baked_'+str(i)
        mat=unreal.load_asset(mat_path) or asset_tools.create_asset('M_Baked_'+str(i),dest,unreal.Material,unreal.MaterialFactoryNew())
        mat.set_editor_property('used_with_skeletal_mesh',True)
        remember_graph(mat)
        # These materials can already be rooted by the character CDO. Deleting
        # rooted expressions asserts in UE 5.6; reconnect new expressions safely.
        samples={}
        for kind,prop in [('BaseColor',unreal.MaterialProperty.MP_BASE_COLOR),('Normal',unreal.MaterialProperty.MP_NORMAL),('Roughness',unreal.MaterialProperty.MP_ROUGHNESS)]:
            sample=expression(mat,unreal.MaterialExpressionTextureSample,-450,{'BaseColor':0,'Normal':220,'Roughness':440}[kind],
                prop,'BAKED_'+kind.upper())
            sample.set_editor_property('texture',textures[kind])
            sample.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if kind=='BaseColor' else (unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if kind=='Normal' else unreal.MaterialSamplerType.SAMPLERTYPE_MASKS))
            lib.connect_material_property(sample,'R' if kind=='Roughness' else 'RGB',prop)
            samples[kind]=sample
            if name=='Shirotsura' and label=='09 • petrified corruption' and kind=='BaseColor':
                configure_shirotsura_corruption(mat,sample)
            if name=='Shirotsura' and label=='05 • exposed right hand' and kind=='BaseColor' and 'FaceNeckMask' in textures:
                mask=expression(mat,unreal.MaterialExpressionTextureSample,-450,120,
                    unreal.MaterialProperty.MP_BASE_COLOR,'SHIROTSURA_FACE_NECK_MASK')
                mask.set_editor_property('texture',textures['FaceNeckMask'])
                mask.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
                configure_shirotsura_corruption(mat,sample,mask)
        spec=surface_detail(name,label)
        if detail_function and spec:
            configure_surface_detail(mat,spec,samples['BaseColor'],samples['Normal'],detail_function,detail_cache)
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
