"""Read-only UE 5.6 API checks. No Tripo source, import, or candidate is fabricated."""
from pathlib import Path
import sys
import unreal
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Tools'))
import ProductionCharacter as P
from ImportRiggedCharacters import run
from ApplyRiggedMaterials import apply_character_materials

results = []
for name in P.NAMES:
    mesh = unreal.load_asset('/Game/Characters/Rigged/' + name + '/SK_' + name)
    assert isinstance(mesh, unreal.SkeletalMesh)
    component = unreal.SkeletalMeshComponent()
    component.set_skeletal_mesh_asset(mesh)
    bones = [str(component.get_bone_name(i)) for i in range(component.get_num_bones())]
    for bone in bones:
        component.get_parent_bone(bone)
        assert component.does_socket_exist(bone)
    bounds = mesh.get_bounds()
    assert bounds.box_extent.z > 0
    mesh.get_editor_property('physics_asset')
    options = unreal.FbxImportUI()
    options.skeletal_mesh_import_data.set_editor_property('update_skeleton_reference_pose', False)
    options.skeletal_mesh_import_data.set_editor_property('use_t0_as_ref_pose', False)
    for slot in mesh.get_editor_property('materials'):
        assert slot.material_interface
        if isinstance(slot.material_interface, unreal.Material):
            unreal.MaterialEditingLibrary.get_used_textures(slot.material_interface)
    results.append({'asset': name, 'bones': len(bones), 'bounds': str(bounds), 'status': 'pass'})
P.write(P.ROOT / 'Saved/ProductionIntake/ue-api-smoke.json', {
    'status': 'pass', 'scope': 'read-only baseline API checks; no candidate import', 'results': results})
unreal.log('PRODUCTION_UE_API_SMOKE_PASS')
