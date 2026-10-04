"""Source contracts for the CC0 microsurface layer on the rigged character atlases."""
import ast
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MATERIALS = ROOT / "Tools/ApplyRiggedMaterials.py"
PREPARE = ROOT / "Tools/PrepareCharacterDetailTextures.py"
FETCH = ROOT / "Tools/FetchCharacterDetailTextures.py"
SOURCES = ROOT / "Art/Characters/Detail/sources.json"


def function_body(path, name):
    source = path.read_text(encoding="utf-8")
    node = next(n for n in ast.walk(ast.parse(source)) if isinstance(n, ast.FunctionDef) and n.name == name)
    return ast.get_source_segment(source, node)


def detail_table():
    tree = ast.parse(MATERIALS.read_text(encoding="utf-8"))
    assign = next(n for n in tree.body if isinstance(n, ast.Assign) and n.targets[0].id == "SURFACE_DETAIL")
    return ast.literal_eval(assign.value)


def test_detail_never_touches_corruption_skin_cord_or_blade_slots():
    keywords = [keyword for rows in detail_table().values() for keyword, _ in rows]
    for protected in ("corruption", "fissure", "crimson", "hand", "blade", "sharpened", "cinnabar", "mineral"):
        assert not any(protected in keyword for keyword in keywords), protected


def test_every_detail_set_is_prepared_and_has_cc0_provenance():
    record = json.loads(SOURCES.read_text(encoding="utf-8"))
    assert record["provider"] == "Poly Haven" and record["license"] == "CC0 1.0"
    fetched = {source["asset_id"] for source in record["sources"]}
    for source in record["sources"]:
        assert {item["map"] for item in source["files"]} == {"diff", "nor_gl", "arm"}
        assert all(re.fullmatch(r"[0-9a-f]{64}", item["sha256"]) for item in source["files"])
    used = {spec[0] for rows in detail_table().values() for _, spec in rows}
    # The bristle set is generated procedurally, so it has no external source.
    assert used - fetched == {"bristle"}
    assert fetched <= used
    assert 'save_png("T_CD_bristle_Detail"' in PREPARE.read_text(encoding="utf-8")


def test_detail_follows_pre_skinned_space_through_vertex_interpolators():
    body = function_body(MATERIALS, "triplanar_detail_function")
    assert "MaterialExpressionPreSkinnedPosition" in body and "MaterialExpressionPreSkinnedNormal" in body
    assert body.count("MaterialExpressionVertexInterpolator") == 2
    assert "WorldPosition" not in body


def test_detail_is_optional_and_applied_after_the_baked_and_corruption_graph():
    body = function_body(MATERIALS, "apply_character_materials")
    assert "if DETAIL_SOURCE.is_dir() else None" in body
    assert body.index("remember_graph(mat)") < body.index("samples[kind]=sample")
    assert body.index("configure_shirotsura_corruption(mat,sample,mask)") < body.index("configure_surface_detail(")
    assert body.index("configure_surface_detail(") < body.index("lib.recompile_material(mat)")


def test_detail_textures_force_linear_compression():
    body = function_body(MATERIALS, "detail_textures")
    assert "set_editor_property('srgb',False)" in body
    assert "TC_NORMALMAP" in body and "TC_DEFAULT" in body and "flip_green_channel" in body


def test_fetch_reuses_the_environment_poly_haven_helpers():
    source = FETCH.read_text(encoding="utf-8")
    assert "import FetchPolyHavenEnvironment as polyhaven" in source
    assert "polyhaven.fetch_texture(" in source and "polyhaven.info(" in source
