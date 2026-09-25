import json
from pathlib import Path


ROOT = Path(__file__).parents[1]
PACKAGE = ROOT / "Art" / "Environment" / "Ishibashiri"
MANIFEST = json.loads((PACKAGE / "manifest.json").read_text(encoding="utf-8"))

REQUIRED_ASSETS = {
    "SM_Ishibashiri_OldCedar_A",
    "SM_Ishibashiri_OldCedar_B",
    "SM_Ishibashiri_FallenCedar_A",
    "SM_Ishibashiri_Rock_A",
    "SM_Ishibashiri_Rock_B",
    "SM_Ishibashiri_BoundaryStone_A",
    "SM_Ishibashiri_RitualPost_A",
    "SM_Ishibashiri_OldRope_A",
}
FIRST_BATCH = [
    "SM_Ishibashiri_OldCedar_A",
    "SM_Ishibashiri_Rock_A",
    "SM_Ishibashiri_BoundaryStone_A",
]


def test_manifest_has_exact_minimal_kit_and_first_adoption_batch():
    assets = MANIFEST["assets"]
    assert {asset["asset_id"] for asset in assets} == REQUIRED_ASSETS
    assert MANIFEST["first_adoption_batch"] == FIRST_BATCH
    assert MANIFEST["production_order"][:3] == FIRST_BATCH
    assert MANIFEST["production_order"][-1] == "Decals"


def test_every_asset_is_an_honest_spec_only_record():
    for asset in MANIFEST["assets"]:
        assert asset["production_status"] == "spec_only"
        assert asset["generator"] == "Tripo AI"
        assert asset["target_engine"] == "UE5.6"
        assert asset["source_file"] is None
        assert asset["textures"] == []
        assert asset["ue_import_status"] == "NOT_RUN"
        assert asset["visual_review_status"] == "NOT_RUN"
        assert asset["tripo_generation_status"] == "NOT_RUN"
        assert asset["blender_cleanup_status"] == "NOT_RUN"
        assert all(lod["source_file"] is None for lod in asset["lods"])


def test_prompts_lods_collision_and_gameplay_constraints_are_complete():
    required_prompt_terms = (
        "game-ready",
        "full object visible",
        "no background",
        "no unnecessary pedestal",
        "no fantasy exaggeration",
    )
    for asset in MANIFEST["assets"]:
        prompt = PACKAGE / asset["prompt_file"]
        assert prompt.is_file(), asset["asset_id"]
        text = prompt.read_text(encoding="utf-8").lower()
        assert all(term in text for term in required_prompt_terms), asset["asset_id"]
        assert "negative prompt" in text
        assert "do not apply automatic decimate unconditionally" in text
        assert [lod["name"] for lod in asset["lods"]] == ["LOD0", "LOD1", "LOD2"]
        assert asset["collision_policy"]
        assert asset["pivot_policy"]
        constraints = " ".join(asset["gameplay_constraints"])
        for term in ("navigation corridor", "Grab", "Climbing", "dodge space"):
            assert term in constraints


def test_later_boss_assets_are_not_in_the_package():
    serialized = json.dumps(MANIFEST, ensure_ascii=False)
    for forbidden in ("Fuchimatoi", "Minedaki", "Magatsune"):
        assert forbidden not in serialized


def test_documentation_keeps_capture_and_generated_files_not_run():
    guide = (ROOT / "Docs" / "Art" / "IshibashiriEnvironmentKit.md").read_text(encoding="utf-8")
    decals = (PACKAGE / "Decals" / "README.md").read_text(encoding="utf-8")
    plan = (ROOT / "Docs" / "IshibashiriDemoFirstPlan.md").read_text(encoding="utf-8")
    assert "NOT_RUN — WINDOWS UE 5.6.1 REQUIRED" in guide
    assert all(name in guide for name in ("Approach entrance", "Cedar enclosure", "Boundary clearing", "Damaged grove", "Ishibashiri reveal gap", "Basin entrance", "Basin wide"))
    assert all(name in decals for name in ("D_Ishibashiri_Footprint_A", "D_Ishibashiri_CorruptionCrack_A", "D_Ishibashiri_Gouge_A"))
    assert "Art/IshibashiriEnvironmentKit.md" in plan
