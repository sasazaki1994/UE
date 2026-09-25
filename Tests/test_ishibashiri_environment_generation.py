import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "Art/Environment/Ishibashiri/manifest.json"
GENERATOR = ROOT / "Tools/CreateIshibashiriEnvironmentKit.py"
VALIDATOR = ROOT / "Tools/VerifyIshibashiriEnvironmentKit.py"


def manifest():
    return json.loads(MANIFEST.read_text(encoding="utf-8"))


def test_generator_is_deterministic_offline_and_first_batch_only():
    source = GENERATOR.read_text(encoding="utf-8")
    data = manifest()
    assert "SEED = 830194" in source
    assert tuple(data["first_adoption_batch"]) == (
        "SM_Ishibashiri_OldCedar_A", "SM_Ishibashiri_Rock_A", "SM_Ishibashiri_BoundaryStone_A")
    assert "ASSET_IDS = (" in source
    for forbidden in ("requests", "urllib", "http://", "https://", "Tripo API", "marketplace"):
        assert forbidden not in source


def test_manifest_remains_authority_and_tripo_not_falsified():
    data = manifest()
    specs = {item["asset_id"]: item for item in data["assets"]}
    expected = {
        "SM_Ishibashiri_OldCedar_A": (1800, 2400, 28000, 45000),
        "SM_Ishibashiri_Rock_A": (180, 300, 12000, 22000),
        "SM_Ishibashiri_BoundaryStone_A": (170, 220, 10000, 18000),
    }
    for asset, (zmin, zmax, tmin, tmax) in expected.items():
        spec = specs[asset]
        zr = spec["dimensions"].get("height_cm", spec["dimensions"].get("size_cm", {}).get("z"))
        assert zr == [zmin, zmax]
        assert (spec["lods"][0]["triangle_target_min"], spec["lods"][0]["triangle_target_max"]) == (tmin, tmax)
        assert spec["generator"] == "Tripo AI"
        assert spec["tripo_generation_status"] == "NOT_RUN"
        assert spec["fallback_generation_status"] == "NOT_RUN"
        assert spec["gameplay_constraints"]


def test_validator_distinguishes_not_generated(tmp_path):
    spec = importlib.util.spec_from_file_location("kit_validator", VALIDATOR)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    status, messages = module.validate(tmp_path)
    assert status == "NOT_GENERATED"
    assert messages


def test_generator_reports_ue_checks_as_not_run():
    source = GENERATOR.read_text(encoding="utf-8")
    assert '"ue_import_status": "NOT_RUN"' in source
    assert '"ue_visual_review_status": "NOT_RUN"' in source
    assert "FINAL PRODUCTION ASSET" not in source
