"""Host-side contract checks that do not pretend to replace Blender/UE validation."""
import ast
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
create = (ROOT / 'Tools/CreateCharacterModels.py').read_text(encoding='utf-8')
rig = (ROOT / 'Tools/RigCharacterModels.py').read_text(encoding='utf-8')
materials = (ROOT / 'Tools/ApplyRiggedMaterials.py').read_text(encoding='utf-8')

for path in ['Tools/CreateCharacterModels.py', 'Tools/RefineCharacterModels.py',
             'Tools/RigCharacterModels.py', 'Tools/ImportRiggedCharacters.py',
             'Tools/ApplyRiggedMaterials.py', 'Tools/VerifyRiggedCharacters.py',
             'Tools/RebuildReferenceShirotsura.py', 'Tools/ReferenceShirotsuraHead.py',
             'Tools/ReferenceShirotsuraCostume.py',
             'Tools/PolishCharacterSurfaces.py', 'Tools/VerifyCharacterQuality.py',
             'Tools/VerifyCharacterQualityUE.py']:
    ast.parse((ROOT / path).read_text(encoding='utf-8'), filename=path)

required_geometry = [
    'Mask_shallow_adze_mark', 'Mask_edge_chip', 'Short_straw_mantle',
    'Shoulder_straw_tuft', 'Indigo_visible_repair', 'Hide_to_stone_transition',
    'Transition_root', "cores=[(2.18,-1.55,6.11),(0,.60,7.84),(-1.22,3.05,6.88)]",
]
assert all(token in create for token in required_geometry)
assert "'Climb'" in rig and "'Hang'" in rig and "'Grip'" in rig
assert "return .91,0.0" in materials and "return .96,0.0" in materials
# The UE 5.6 implementation reuses the node connected to the material property.
# Runtime repeat-application checks live in VerifyCharacterQualityUE.py.
assert 'get_material_property_input_node(mat,prop)' in materials, 'material expressions must be reused'

baseline = {}
for name in ['Shirotsura', 'Ishibashiri']:
    baseline[name] = json.loads(
        (ROOT / f'Art/Characters/{name}/model-info.json').read_text(encoding='utf-8'))
    assert baseline[name]['unit'] == 'meter' and baseline[name]['forward'] == '-Y'
assert len(json.loads((ROOT / 'Art/Characters/Ishibashiri/climb-layout.json').read_text())['corruption_cores_m']) == 3
print(json.dumps({'status': 'SOURCE_CONTRACT_PASS', 'checked_baseline': baseline}, indent=2))
