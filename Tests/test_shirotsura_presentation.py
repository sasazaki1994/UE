import ast
import re
from pathlib import Path


ROOT = Path(__file__).parents[1]
PRIVATE = ROOT / "Source/IshibashiriPrototype/Private"
PUBLIC = ROOT / "Source/IshibashiriPrototype/Public"


def test_story_wrap_keeps_layout_width_separate_from_measurement():
    hud = (PRIVATE / "CampaignHUD.cpp").read_text(encoding="utf-8")
    layout = (PRIVATE / "CampaignTextLayout.h").read_text(encoding="utf-8")
    assert "StrLen(Font, TEXT(\"白面\"), SampleWidth, LineHeight)" in hud
    assert "const float DisplayWidth" in hud
    assert "DisplayWidth) / FMath::Max(Scale" in layout
    assert "ContinueTop - BodyTop" in hud


def test_all_encounter_players_use_shared_visual_and_reset_it():
    for player in ("Prototype", "Fuchimatoi", "Minedaki", "Magatsune"):
        source = (PRIVATE / f"{player}Player.cpp").read_text(encoding="utf-8")
        header = (PUBLIC / f"{player}Player.h").read_text(encoding="utf-8")
        assert "UShirotsuraVisualComponent" in header
        assert "ShirotsuraVisual->Configure(GetMesh()" in source
        assert "ShirotsuraVisual->ResetPresentation()" in source


def test_shared_visual_has_mutually_exclusive_fallback_and_state_guard():
    source = (PRIVATE / "ShirotsuraVisualComponent.cpp").read_text(encoding="utf-8")
    assert "Mesh->SetVisibility(bUsingRig" in source
    assert "Fallback->SetVisibility(!bUsingRig" in source
    assert "Next == CurrentState" in source
    assert "ProductionVisuals::ApplyAtBeginPlay" in source


def test_stage_is_applied_after_production_selection_and_is_not_tick_driven():
    source = (PRIVATE / "ShirotsuraVisualComponent.cpp").read_text(encoding="utf-8")
    begin = source.index("ProductionVisuals::ApplyAtBeginPlay")
    apply = source.index("ApplyCorruptionAppearance();", begin)
    tick = source.index("void UShirotsuraVisualComponent::TickComponent")
    assert begin < apply < tick
    assert "ApplyCorruptionAppearance();" not in source[tick:]
    assert "GetCorruptionStageForEncounter(StandaloneEncounter" in source
    assert "HasResolvedCorruptionStage" in (PUBLIC / "ShirotsuraVisualComponent.h").read_text(encoding="utf-8")
    assert "HasAppliedCorruptionAppearance" in (PUBLIC / "ShirotsuraVisualComponent.h").read_text(encoding="utf-8")


def test_standalone_encounters_pass_their_stage_authority():
    expected = {
        "Prototype": None,
        "Fuchimatoi": "Fuchimatoi",
        "Minedaki": "Minedaki",
        "Magatsune": "Magatsune",
    }
    for player, encounter in expected.items():
        source = (PRIVATE / f"{player}Player.cpp").read_text(encoding="utf-8")
        if encounter:
            assert f"ECampaignState::{encounter}" in source
        else:
            assert "ShirotsuraVisual->Configure(GetMesh(), Body, Sword);" in source


def test_material_setup_is_shirotsura_only_and_idempotent_by_tag():
    source = (ROOT / "Tools/ApplyRiggedMaterials.py").read_text(encoding="utf-8")
    assert "name=='Shirotsura'" in source
    assert "slot_role(label)=='petrified corruption'" in source
    assert "ShirotsuraCorruptionIntensity" in source
    # UE 5.6 Python has no expression listing; tagged nodes come from a pre-reconnect graph snapshot.
    assert "get_all_material_expressions" not in source
    assert "remember_graph(mat)" in source
    assert "tag+'BLEND'" in source
    assert "delete_material_expression" not in source


def test_face_neck_mask_uses_source_object_provenance_and_is_optional_until_regenerated():
    rig = (ROOT / "Tools/RigCharacterModels.py").read_text(encoding="utf-8")
    materials = (ROOT / "Tools/ApplyRiggedMaterials.py").read_text(encoding="utf-8")
    runtime = (PRIVATE / "ShirotsuraVisualComponent.cpp").read_text(encoding="utf-8")
    assert "FACE_NECK_MASK_ATTRIBUTE='ShirotsuraFaceNeckMask'" in rig
    assert "ob.name.startswith(('Head_under_mask','Neck_anatomy'))" in rig
    assert "T_Shirotsura_FaceNeckMask.png" in rig
    assert "mask_path.exists()" in materials
    assert "slot_role(label)=='exposed right hand'" in materials
    assert "SHIROTSURA_FACE_NECK_MASK" in materials
    assert "A zero mask must be an exact pass-through" in materials
    assert 'TEXT("exposed right hand")' in runtime


def imported_slot_names(asset):
    # Non-ASCII FNames are stored as UTF-16 at either byte alignment.
    data = (ROOT / asset).read_bytes()
    text = data.decode("utf-16-le", errors="ignore") + data[1:].decode("utf-16-le", errors="ignore")
    return set(re.findall(r"\d\d_•_[a-z_]+?(?=DiffSpecFunc|[^a-z_]|$)", text))


def tool_slot_role():
    tree = ast.parse((ROOT / "Tools/ApplyRiggedMaterials.py").read_text(encoding="utf-8"))
    function = next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == "slot_role")
    namespace = {}
    exec(compile(ast.Module(body=[function], type_ignores=[]), "slot_role", "exec"), namespace)
    return namespace["slot_role"]


def test_corruption_roles_match_imported_shirotsura_slots():
    slots = imported_slot_names("Content/Characters/Rigged/Shirotsura/SK_Shirotsura.uasset")
    slot_role = tool_slot_role()
    roles = {slot_role(slot) for slot in slots}
    assert "petrified corruption" in roles
    assert "exposed right hand" in roles
    assert slot_role("09 • petrified corruption") == "petrified corruption"
    runtime = (PRIVATE / "ShirotsuraVisualComponent.cpp").read_text(encoding="utf-8")
    assert 'Replace(TEXT("_"), TEXT(" "))' in runtime
    assert 'Role.Find(TEXT("•"))' in runtime
    assert 'TEXT("petrified corruption")' in runtime
    assert "Mesh->GetMaterialSlotNames()" in runtime
    # Section numbers changed between model revisions; matching must not depend on them.
    assert not re.search(r'TEXT\("\d\d', runtime)


def test_no_gameplay_or_extra_appearance_state_was_added():
    header = (PUBLIC / "CampaignGameInstance.h").read_text(encoding="utf-8")
    enum = header.split("enum class EShirotsuraCorruptionStage", 1)[1].split("};", 1)[0]
    assert enum.count("Early") == 1
    assert enum.count("Advanced") == 1
    visual = (PRIVATE / "ShirotsuraVisualComponent.cpp").read_text(encoding="utf-8")
    for forbidden in ("MaxWalkSpeed", "Health", "Stamina", "InputComponent", "PlayerSense"):
        assert forbidden not in visual
