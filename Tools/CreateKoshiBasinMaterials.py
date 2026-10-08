"""Build basin presentation materials using existing textures and simple graphs in UE 5.6.

Run with UnrealEditor-Cmd -run=pythonscript -script=Tools/CreateKoshiBasinMaterials.py.
No existing material, mesh, map or gameplay asset is modified.
"""
import unreal

DEST = '/Game/Environment/Ishibashiri'
LIB = unreal.MaterialEditingLibrary
ASSETS = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
MP = unreal.MaterialProperty


def node(mat, cls, x, y, **props):
    result = LIB.create_material_expression(mat, cls, x, y)
    for key, value in props.items():
        result.set_editor_property(key, value)
    return result


def link(a, b, pin, output=''):
    if not LIB.connect_material_expressions(a, output, b, pin):
        raise RuntimeError('Failed material expression connection: ' + pin)


def prop(a, p):
    if not LIB.connect_material_property(a, '', p):
        raise RuntimeError('Failed material property connection: ' + str(p))


def material(name, domain):
    path = DEST + '/' + name
    mat = unreal.load_asset(path) if ASSETS.does_asset_exist(path) else None
    if mat is None:
        mat = TOOLS.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())
    LIB.delete_all_material_expressions(mat)
    mat.set_editor_property('material_domain', domain)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    return mat


def ground_mark(name, color, roughness, opacity):
    mat = material(name, unreal.MaterialDomain.MD_DEFERRED_DECAL)
    uv = node(mat, unreal.MaterialExpressionTextureCoordinate, -1000, 200)
    centered = node(mat, unreal.MaterialExpressionSubtract, -800, 200, const_b=.5)
    link(uv, centered, 'A')
    radius = node(mat, unreal.MaterialExpressionDotProduct, -600, 200)
    link(centered, radius, 'A')
    link(centered, radius, 'B')
    scale = node(mat, unreal.MaterialExpressionMultiply, -420, 200, const_b=4.0)
    link(radius, scale, 'A')
    inverse = node(mat, unreal.MaterialExpressionOneMinus, -250, 200)
    link(scale, inverse, '')
    clamp = node(mat, unreal.MaterialExpressionSaturate, -80, 200)
    link(inverse, clamp, '')
    soft = node(mat, unreal.MaterialExpressionPower, 100, 200, const_exponent=2.0)
    link(clamp, soft, 'Base')
    alpha = node(mat, unreal.MaterialExpressionMultiply, 280, 200, const_b=opacity)
    link(soft, alpha, 'A')
    prop(alpha, MP.MP_OPACITY)
    tint = node(mat, unreal.MaterialExpressionConstant3Vector, -250, -100, constant=unreal.LinearColor(*color, 1))
    prop(tint, MP.MP_BASE_COLOR)
    rough = node(mat, unreal.MaterialExpressionConstant, 100, -100, r=roughness)
    prop(rough, MP.MP_ROUGHNESS)
    save(mat)


def save(mat):
    LIB.recompile_material(mat)
    if not ASSETS.save_loaded_asset(mat, only_if_is_dirty=False):
        raise RuntimeError('Material save failed: ' + mat.get_path_name())


def footprint():
    # Reuse the authored cloven-hoof mask. A matte, flat decal avoids glossy raised blobs.
    mat = material('D_Ishibashiri_BasinFootprint', unreal.MaterialDomain.MD_DEFERRED_DECAL)
    texture = unreal.load_asset(DEST + '/Decals/T_D_Ishibashiri_Footprint_A_BaseColor')
    if texture is None:
        raise RuntimeError('Import the existing footprint texture first')
    mask = node(mat, unreal.MaterialExpressionTextureSample, -600, 180, texture=texture)
    alpha = node(mat, unreal.MaterialExpressionMultiply, -350, 180, const_b=.58)
    link(mask, alpha, 'A', 'A')
    prop(alpha, MP.MP_OPACITY)
    color = node(mat, unreal.MaterialExpressionConstant3Vector, -350, -80, constant=unreal.LinearColor(.14, .115, .085, 1))
    prop(color, MP.MP_BASE_COLOR)
    rough = node(mat, unreal.MaterialExpressionConstant, -100, -80, r=.94)
    prop(rough, MP.MP_ROUGHNESS)
    specular = node(mat, unreal.MaterialExpressionConstant, -100, -200, r=.04)
    prop(specular, MP.MP_SPECULAR)
    save(mat)


def main():
    footprint()
    ground_mark('D_Ishibashiri_BasinWetSoil', (.10, .085, .065), .26, .30)
    ground_mark('D_Ishibashiri_BasinMoss', (.075, .115, .055), .91, .42)
    mat = material('M_Ishibashiri_BasinDust', unreal.MaterialDomain.MD_SURFACE)
    mat.set_editor_property('used_with_instanced_static_meshes', True)
    mat.set_editor_property('two_sided', False)
    tint = node(mat, unreal.MaterialExpressionConstant3Vector, -600, -150, constant=unreal.LinearColor(.19, .16, .12, 1))
    prop(tint, MP.MP_BASE_COLOR)
    fresnel = node(mat, unreal.MaterialExpressionFresnel, -900, 200, exponent=1.5, base_reflect_fraction=0.0)
    inverse = node(mat, unreal.MaterialExpressionOneMinus, -700, 200)
    link(fresnel, inverse, '')
    strength = node(mat, unreal.MaterialExpressionMultiply, -500, 200, const_b=.24)
    link(inverse, strength, 'A')
    fade = node(mat, unreal.MaterialExpressionDepthFade, -300, 200, fade_distance_default=90.0)
    link(strength, fade, 'Opacity')
    custom = node(mat, unreal.MaterialExpressionPerInstanceCustomData, -300, 450, data_index=0, const_default_value=0.0)
    alpha = node(mat, unreal.MaterialExpressionMultiply, -80, 200)
    link(fade, alpha, 'A')
    link(custom, alpha, 'B')
    prop(alpha, MP.MP_OPACITY)
    save(mat)
    unreal.log('KOSHI_BASIN_MATERIALS_PASS materials=4 new_textures=0')


if __name__ == '__main__':
    main()
