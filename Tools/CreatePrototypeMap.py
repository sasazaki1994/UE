"""Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>."""
from pathlib import Path
import unreal


map_asset = "/Game/Maps/L_Prototype_01"
map_file = Path(unreal.Paths.project_content_dir()) / "Maps" / "L_Prototype_01.umap"
if map_file.exists():
    unreal.log("Ishibashiri: existing map preserved: " + str(map_file))
else:
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    if not world:
        raise RuntimeError("Failed to create the blank prototype world")
    game_mode = unreal.load_class(None, "/Script/IshibashiriPrototype.PrototypeGameMode")
    if not game_mode:
        raise RuntimeError("Build IshibashiriPrototypeEditor before generating the map")
    world.get_world_settings().set_editor_property("default_game_mode", game_mode)
    map_file.parent.mkdir(parents=True, exist_ok=True)
    if not unreal.EditorLoadingAndSavingUtils.save_map(world, map_asset):
        raise RuntimeError("Failed to save " + map_asset)
    if not map_file.exists():
        raise RuntimeError("Save returned success but the map file is missing")
    unreal.log("Ishibashiri: generated " + map_asset)
