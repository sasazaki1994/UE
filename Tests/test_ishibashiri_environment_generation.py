import ast
import importlib.util
import json
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "Art/Environment/Ishibashiri/manifest.json"
GENERATOR = ROOT / "Tools/CreateIshibashiriEnvironmentKit.py"
VALIDATOR = ROOT / "Tools/VerifyIshibashiriEnvironmentKit.py"
ROUNDTRIP = ROOT / "Tools/ValidateIshibashiriEnvironmentArtifacts.py"
UE_IMPORTER = ROOT / "Tools/ImportIshibashiriEnvironmentKit.py"
WORKFLOW = ROOT / ".github/workflows/ishibashiri-environment-generation.yml"


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


def test_first_batch_reports_pair_one_render_mesh_with_its_ucx_collision():
    spec = importlib.util.spec_from_file_location("kit_validator", VALIDATOR)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    specs = {item["asset_id"]: item for item in manifest()["assets"]}
    first_batch = manifest()["first_adoption_batch"]
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        for asset in first_batch:
            directory = root / asset.removeprefix("SM_Ishibashiri_")
            directory.mkdir()
            for suffix in (".fbx", ".glb", "_preview.png"):
                (directory / (asset + suffix)).write_bytes(b"fixture")
            ranges = module.dimension_ranges(specs[asset])
            lod = specs[asset]["lods"][0]
            report = {
                "asset_id": asset,
                "mesh_object_count": 1,
                "visual_object_names": [asset],
                "collision": {"name": "UCX_" + asset + "_00"},
                "blender_version": "fixture",
                "seed": 830194,
                "dimensions_cm": {axis: (limits[0] + limits[1]) / 2 for axis, limits in ranges.items()},
                "triangle_count": (lod["triangle_target_min"] + lod["triangle_target_max"]) // 2,
                "material_count": 1,
                "pivot": "ground",
                "collision_status": "GENERATED",
                "lod_status": {},
                "generation_status": "BLENDER PRODUCTION CANDIDATE",
                "ue_import_status": "NOT_RUN",
                "ue_visual_review_status": "NOT_RUN",
            }
            (directory / (asset + "_report.json")).write_text(json.dumps(report), encoding="utf-8")
        assert module.validate(root)[0] == "PASS"
        cedar = first_batch[0]
        report_path = root / cedar.removeprefix("SM_Ishibashiri_") / (cedar + "_report.json")
        report = json.loads(report_path.read_text(encoding="utf-8"))
        report["visual_object_names"] = ["Cedar_Trunk", "Cedar_Foliage"]
        report_path.write_text(json.dumps(report), encoding="utf-8")
        status, messages = module.validate(root)
        assert status == "FAIL"
        assert any("UCX collision" in message for message in messages)


def test_generator_reports_ue_checks_as_not_run():
    source = GENERATOR.read_text(encoding="utf-8")
    assert '"ue_import_status": "NOT_RUN"' in source
    assert '"ue_visual_review_status": "NOT_RUN"' in source
    assert "FINAL PRODUCTION ASSET" not in source


def test_workflow_runs_official_blender_and_real_generator():
    source = WORKFLOW.read_text(encoding="utf-8")
    assert "runs-on: ubuntu-24.04" in source
    assert "BLENDER_VERSION: 3.6.23" in source
    assert "https://download.blender.org/release/Blender3.6" in source
    assert "sha256sum --check archive.sha256" in source
    assert "--background --factory-startup --python-exit-code 1" in source
    assert "--python Tools/CreateIshibashiriEnvironmentKit.py" in source
    assert "python Tools/VerifyIshibashiriEnvironmentKit.py" in source
    assert "--python Tools/ValidateIshibashiriEnvironmentArtifacts.py" in source


def test_workflow_is_first_batch_artifact_only_and_does_not_fake_ue():
    source = WORKFLOW.read_text(encoding="utf-8")
    for asset in manifest()["first_adoption_batch"]:
        assert asset in source
    for forbidden in ("FallenCedar_A", "OldCedar_B", "Rock_B", "RitualPost_A", "OldRope_A"):
        assert forbidden not in source
    assert "actions/upload-artifact@v4" in source
    assert "retention-days: 14" in source
    assert "git commit" not in source and "git push" not in source
    assert '.ue_import == "NOT_RUN"' in source
    assert '.visual_approval == "NOT_RUN"' in source


def test_roundtrip_contract_checks_geometry_collision_and_previews():
    source = ROUNDTRIP.read_text(encoding="utf-8")
    assert "bpy.ops.wm.read_factory_settings(use_empty=True)" in source
    assert "bpy.ops.import_scene.fbx" in source
    assert "bpy.ops.import_scene.gltf" in source
    assert 'collision_name = "UCX_" + asset + "_00"' in source
    for check in ("NaN or Inf", "abnormal scale", "empty geometry", "not_single_colour"):
        assert check in source
    for check in ("visual_object_names", "vertex_count", "triangle_count", "local_aabb", "world_aabb", "PREVIEW_"):
        assert check in source
    assert "TOLERANCE = 0.03" in source
    assert '"ue_import": "NOT_RUN"' in source
    assert '"visual_approval": "NOT_RUN"' in source


def test_preview_visibility_and_export_selection_are_separate_contracts():
    source = GENERATOR.read_text(encoding="utf-8")
    assert "def set_preview_visibility(visual):" in source
    assert "obj.name.startswith(COLLISION_PREFIX)" in source
    assert "obj.hide_render = True" in source
    assert "obj.hide_render = False" in source
    assert "selected = visual + [collision]" in source
    assert "preview-only object entered production export selection" in source
    assert source.index("bpy.ops.export_scene.gltf") < source.index("preview(directory/")


def test_contact_sheet_has_three_separate_labelled_panels_and_exact_validation():
    generator = GENERATOR.read_text(encoding="utf-8")
    validator = ROUNDTRIP.read_text(encoding="utf-8")
    assert "scene.render.resolution_x, scene.render.resolution_y = 1800, 720" in generator
    assert "x = (index - 1) * 6.0" in generator
    for label in ("Dimensions:", "Triangles:"):
        assert label in generator
    assert '(contact["width"], contact["height"]) != (1800, 720)' in validator


def test_ue_importer_requires_ucx_collision_before_saving_first_batch():
    source = UE_IMPORTER.read_text(encoding="utf-8")
    tree = ast.parse(source)
    constants = {
        statement.targets[0].id: ast.literal_eval(statement.value)
        for statement in tree.body
        if isinstance(statement, ast.Assign) and isinstance(statement.targets[0], ast.Name)
        and statement.targets[0].id in ("DESTINATION", "ASSETS")
    }
    assert constants["DESTINATION"] == "/Game/Environment/Ishibashiri"
    assert tuple("SM_Ishibashiri_" + name for name in constants["ASSETS"]) == tuple(manifest()["first_adoption_batch"])
    importer = next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == "import_mesh")
    settings = next(node for node in importer.body if isinstance(node, ast.Assign)
                    and ast.unparse(node.value) == "options.static_mesh_import_data")
    alias = settings.targets[0].id
    assignments = {
        ast.unparse(target): node.value
        for node in ast.walk(importer) if isinstance(node, ast.Assign)
        for target in node.targets
    }
    assert ast.literal_eval(assignments[alias + ".combine_meshes"]) is True
    assert ast.literal_eval(assignments[alias + ".auto_generate_collision"]) is False
    assert ast.literal_eval(assignments[alias + ".one_convex_hull_per_ucx"]) is True
    assert ast.unparse(assignments["name"]) == "'SM_Ishibashiri_' + short_name"
    assert ast.unparse(assignments["collision_count"]) == "EDITOR.get_simple_collision_count(mesh)"
    # A stricter exactly-one hull gate replaced the old nonzero check. Keep the
    # failure branch and ordering explicit; changing its message cannot hide it.
    guard = next(node for node in importer.body if isinstance(node, ast.If)
                 and ast.unparse(node.test) == "collision_count != 1")
    assert any(isinstance(node, ast.Raise) for node in guard.body)
    final_saves = [node for node in ast.walk(importer) if isinstance(node, ast.Call)
                   and ast.unparse(node.func) == "unreal.EditorAssetLibrary.save_loaded_asset"
                   and node.args and ast.unparse(node.args[0]) == "mesh"]
    assert final_saves and all(node.lineno > guard.end_lineno for node in final_saves)
