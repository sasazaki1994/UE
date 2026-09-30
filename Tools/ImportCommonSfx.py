"""Import six validated PCM cues. Run using UE's PythonScript commandlet.

The preview is deliberately excluded. Import success does not mean listening passed.
"""
import json
from pathlib import Path
import sys

import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0, str(ROOT / "Tools"))
import ValidateCommonSfx

report = ValidateCommonSfx.validate()
if "provenance: OK" not in report:
    raise RuntimeError("Common SFX sources are not complete: " + "; ".join(report))
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
imported = []
for name in ValidateCommonSfx.NAMES:
    task = unreal.AssetImportTask()
    task.filename = str(ROOT / "Audio/CommonSfx" / f"{name}.wav")
    task.destination_path = "/Game/Audio/CommonSfx"
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    task.factory = unreal.SoundFactory()
    asset_tools.import_asset_tasks([task])
    path = f"/Game/Audio/CommonSfx/{name}"
    asset = unreal.load_asset(path)
    if not isinstance(asset, unreal.SoundWave) or not task.imported_object_paths:
        raise RuntimeError("Expected a SoundWave: " + path)
    asset.set_editor_property("looping", False)
    asset.set_editor_property("volume", 1.0)
    asset.set_editor_property("pitch", 1.0)
    # Avoid waiting on streamed chunks for short, timing-critical cues.
    asset.set_editor_property("loading_behavior", unreal.SoundWaveLoadingBehavior.FORCE_INLINE)
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError("Could not save " + path)
    imported.append(path)
manifest_path = ROOT / "Audio/CommonSfx/manifest.json"
manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
manifest["ue_import_review"] = "passed"
manifest["ue_assets"] = imported
manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
result = ROOT / "Saved/Automation/CommonSfxImport.json"
result.parent.mkdir(parents=True, exist_ok=True)
result.write_text(json.dumps(dict(status="PASS", imported=imported, human_listening="NOT_RUN"), indent=2) + "\n", encoding="utf-8")
unreal.log("COMMON_SFX_IMPORT PASS: six PCM SoundWaves saved; preview excluded; listening not performed")
