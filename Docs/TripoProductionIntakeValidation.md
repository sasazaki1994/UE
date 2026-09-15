# Tripo Intake Validation Record

This record is intentionally source-aware. It must not report a Tripo asset as imported or adopted when the source file is absent.

| Asset | Tripo source | Preflight | Blender cleanup/rig | UE import | Gameplay/visual | Decision |
|---|---|---|---|---|---|---|
| Shirotsura | Missing | `missing_source` | Blocked | Blocked | Not run | `BLOCKED — TRIPO SOURCE REQUIRED` |
| Ishibashiri | Missing | `missing_source` | Blocked | Blocked | Not run | `BLOCKED — TRIPO SOURCE REQUIRED` |

The existing `Art/Characters/<name>/Rigged` files, `Content/Characters/Rigged` assets, and `Art/CharacterModels.zip` were inspected as the authored fallback/baseline. None was reclassified as a Tripo output.

Verification completed on Windows UE 5.6 and Blender 3.6.23:

- `Tools/Prototype.ps1 -Action Build`: passed after retaining the existing source-contract fixes needed for UE 5.6 compilation.
- `Tools/Prototype.ps1 -Action Test -Climbing -SkipBuild -ProductionVisuals 1 -TestFPS 60`: passed. Both candidate paths were absent, so logs show `PRODUCTION_VISUAL_FALLBACK`; this validates fallback safety and the existing route, not a production visual.
- UE Editor Automation `RunTests IshibashiriPrototype`: 34 succeeded, 0 failed, 0 not run. This covers Sense, Kakon, Nushi state, Campaign lifecycle, and `ProductionVisuals.AssetOnlyContract`.
- `Tests/production_blender_smoke.py` under Blender 3.6.23: passed. It reads explicit test geometry in a temporary directory, exercises GLB/GLTF/FBX/BLEND loading, detects extreme scale and missing textures, transfers provisional baseline weights, verifies the baseline action deformation, bakes 32px fixture maps, and exports FBX. The fixture is not a Tripo or production asset.
- Python source tests: 58 passed with 10 subtests when run in UTF-8 mode. One unrelated existing test is locale-sensitive when invoked under the default cp932 locale; use `python -X utf8 -m pytest Tests -q`.

Not run by design: UE import of a real candidate, UE shader/pink-material review, human weight and silhouette review, input-driven production-candidate regression, 30/60/120 FPS production comparison, GPU/frame-time profiling, High Quality captures, and the ADOPT/CONDITIONAL/REJECT review. These require the actual Tripo source files and recorded review evidence.

## Required next evidence

After the source arrives, run the commands in [TripoProductionIntake.md](TripoProductionIntake.md), fill each manifest's provenance, complete `rig-review.json`, create paired Legacy/High Quality captures, record all listed regression cases and performance metrics, then run `ReviewProductionCharacter.py`. `--adopt` is the only command that records adoption, and it succeeds only after the evidence gates pass.
