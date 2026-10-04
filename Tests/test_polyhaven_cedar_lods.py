import ast
import json
from pathlib import Path


ROOT = Path(__file__).parents[1]
PREPARE = ROOT / "Tools/PreparePolyHavenEnvironment.py"
REPORT = ROOT / "Art/Environment/Ishibashiri/PolyHaven/ue-import-report.json"
CEDARS = ("SM_Ishibashiri_OldCedar_A", "SM_Ishibashiri_OldCedar_B")


def specs() -> dict:
    tree = ast.parse(PREPARE.read_text(encoding="utf-8"))
    node = next(item for item in tree.body if isinstance(item, ast.Assign) and item.targets[0].id == "SPECS")
    return {spec["asset"]: spec for spec in ast.literal_eval(node.value)}


def prepare_body() -> str:
    return PREPARE.read_text(encoding="utf-8").split("def prepare(spec):", 1)[1].split("\ndef ", 1)[0]


def test_cedar_lods_keep_the_lod0_crown_area_with_fewer_cards():
    for asset in CEDARS:
        spec = specs()[asset]
        budgets = [budget for budget, _, _ in spec["lods"]]
        assert budgets == sorted(budgets, reverse=True)
        assert budgets[0] < spec["card_budget"]
        # Every cedar in the Campaign arena is 60 m or more from the fight, so LOD2/LOD3 carry the silhouette.
        assert all(coverage == 1.0 for _, coverage, _ in spec["lods"])


def test_lod_card_scale_is_derived_from_the_dropped_card_area():
    body = prepare_body()
    assert 'cards = slot_triangles(obj).get(spec.get("foliage"), 0)' in body
    loop = body.split("for index, (budget, coverage, ratio)", 1)[1]
    assert "scale = (coverage * cards / budget) ** .5" in loop
    assert loop.index("scale = (coverage") < loop.index("thin_cards(lod,") < loop.index("decimate_solids(lod,")
    assert "budget, scale)" in loop
    assert '"card_scale": round(scale, 3)' in loop


def test_imported_cedar_lods_stay_inside_the_authored_budgets():
    meshes = {Path(entry["asset"]).stem.split(".")[0]: entry for entry in json.loads(REPORT.read_text(encoding="utf-8"))["static_meshes"]}
    for asset in CEDARS:
        triangles = meshes[asset]["lod_triangles"]
        assert len(triangles) == 4
        assert triangles == sorted(triangles, reverse=True)
        assert triangles[3] < 45000
