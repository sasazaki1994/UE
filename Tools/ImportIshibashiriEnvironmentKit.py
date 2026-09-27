"""Import and collision-check the Ishibashiri first batch in Unreal Editor.

Run with UnrealEditor-Cmd.exe <uproject> -run=pythonscript
    -script=Tools/ImportIshibashiriEnvironmentKit.py
The Blender generator must have produced the ignored Generated/ artifacts first.
"""
from pathlib import Path

import unreal


ROOT = Path(__file__).resolve().parents[1]
GENERATED = ROOT / "Art/Environment/Ishibashiri/Generated"
DESTINATION = "/Game/Environment/Ishibashiri"
ASSETS = (
    "SM_Ishibashiri_OldCedar_A",
    "SM_Ishibashiri_Rock_A",
    "SM_Ishibashiri_BoundaryStone_A",
)


def source_path(asset):
    return GENERATED / asset.removeprefix("SM_Ishibashiri_") / (asset + ".fbx")


def import_asset(asset):
    source = source_path(asset)
    if not source.is_file() or source.stat().st_size == 0:
        raise RuntimeError(f"missing generated FBX: {source}")
    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.static_mesh_import_data.combine_meshes = True
    options.static_mesh_import_data.auto_generate_collision = False
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = DESTINATION
    task.destination_name = asset
    task.automated = True
    task.replace_existing = True
    task.save = False
    task.options = options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    package = f"{DESTINATION}/{asset}"
    mesh = unreal.load_asset(package)
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError(f"UE import did not create StaticMesh {package}")
    collision_count = unreal.get_editor_subsystem(
        unreal.StaticMeshEditorSubsystem).get_simple_collision_count(mesh)
    if collision_count < 1:
        raise RuntimeError(
            f"{asset}: no simple collision; expected UCX_{asset}_00 in the FBX")
    unreal.EditorAssetLibrary.save_asset(package, only_if_is_dirty=False)
    unreal.log(f"ISHIBASHIRI_ENV_IMPORT_PASS asset={asset} collision={collision_count}")


for asset_id in ASSETS:
    import_asset(asset_id)
unreal.log("ISHIBASHIRI_ENV_FIRST_BATCH_PASS assets=3")
