"""Import deforming meshes, baked surface maps and the matching animation library.

Set CHARACTER_ASSET_FILTER to Shirotsura or Ishibashiri to import only that model.
"""
from pathlib import Path
import json
import os
import unreal

asset_filter = os.environ.get('CHARACTER_ASSET_FILTER', '').strip()
if asset_filter and asset_filter not in ('Shirotsura', 'Ishibashiri'):
    raise ValueError('CHARACTER_ASSET_FILTER must be Shirotsura or Ishibashiri, or unset for both')
characters = (asset_filter,) if asset_filter else ('Shirotsura', 'Ishibashiri')

root=Path(unreal.Paths.project_dir()).resolve()
asset_tools=unreal.AssetToolsHelpers.get_asset_tools()
report=[]

def run(source,dest,name,options):
    task=unreal.AssetImportTask()
    task.filename=str(source);task.destination_path=dest;task.destination_name=name
    task.automated=True;task.replace_existing=True;task.save=True;task.options=options
    asset_tools.import_asset_tasks([task])
    return [unreal.load_asset(p) for p in task.imported_object_paths]

for name in characters:
    source=root/'Art/Characters'/name/'Rigged'
    dest='/Game/Characters/Rigged/'+name
    opts=unreal.FbxImportUI()
    opts.automated_import_should_detect_type=False
    opts.mesh_type_to_import=unreal.FBXImportType.FBXIT_SKELETAL_MESH
    opts.import_as_skeletal=True;opts.import_mesh=True;opts.import_animations=False
    opts.import_materials=True;opts.import_textures=True;opts.create_physics_asset=False
    loaded=run(source/('SK_'+name+'.fbx'),dest,'SK_'+name,opts)
    meshes=[a for a in loaded if isinstance(a,unreal.SkeletalMesh)]
    if len(meshes)!=1:raise RuntimeError('Missing skeletal mesh: '+name)
    sk=meshes[0];skeleton=sk.get_editor_property('skeleton')
    expected=dest+'/SK_'+name
    if sk.get_path_name().split('.')[0]!=expected:
        if not unreal.EditorAssetLibrary.rename_asset(sk.get_path_name(),expected):raise RuntimeError('Cannot normalize mesh name')
        sk=unreal.load_asset(expected)
    clips=json.loads((source/'rig-info.json').read_text())['animations']
    for clip,meta in clips.items():
        opts=unreal.FbxImportUI()
        opts.automated_import_should_detect_type=False
        opts.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION
        opts.import_as_skeletal=True;opts.import_mesh=False;opts.import_animations=True
        opts.import_materials=False;opts.import_textures=False;opts.skeleton=skeleton
        opts.anim_sequence_import_data.set_editor_property('animation_length',unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
        asset_name='AN_'+name+'_'+clip
        loaded=run(source/(asset_name+'.fbx'),dest,asset_name,opts)
        anims=[a for a in loaded if isinstance(a,unreal.AnimSequence)]
        if len(anims)!=1:raise RuntimeError('Missing animation: '+asset_name)
        animation=anims[0];expected=dest+'/'+asset_name
        if animation.get_path_name().split('.')[0]!=expected:
            if not unreal.EditorAssetLibrary.rename_asset(animation.get_path_name(),expected):raise RuntimeError('Cannot normalize animation name')
        length=animation.get_play_length()
        if abs(length-meta['seconds'])>.06:raise RuntimeError('Clip length mismatch: '+asset_name+' '+str(length))
        if animation.get_editor_property('skeleton')!=skeleton:raise RuntimeError('Wrong skeleton: '+asset_name)
        report.append({'animation':expected,'seconds':length,'skeleton':skeleton.get_path_name()})
    unreal.EditorAssetLibrary.save_directory(dest,only_if_is_dirty=False,recursive=True)
(root/'Art/Characters/rigged-ue-validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('RIGGED_CHARACTERS_IMPORT_PASS')
