# Ishibashiri environment — Poly Haven CC0 replacement

The Approach and Basin arenas load these assets by name from `/Game/Environment/Ishibashiri/` through `IshibashiriEnvironment::Load`. `-PrimitiveEnvironment` still forces the primitive fallback. All visual meshes are NoCollision; the hidden primitives keep gameplay collision.

The normal Campaign Ishibashiri encounter opens `PrototypeGameMode` without `?BasinPrototype`, so it fights in the legacy 80 m square arena. `APrototypeArenaDressing` dresses that arena with the same kit: the ground material on the floor, a two-row rock rim outside the hidden walls, a cedar circle, ferns, fallen cedars, sky atmosphere and distant haze. The floor and wall blocks keep their positions and `BlockAll` collision.

| UE asset | Poly Haven source | License |
| --- | --- | --- |
| `SM_Ishibashiri_OldCedar_A` | `fir_tree_01` (object `fir_tree_01_a_LOD0`) | CC0 1.0 |
| `SM_Ishibashiri_OldCedar_B` | `fir_tree_01` (`fir_tree_01_b_LOD0`) | CC0 1.0 |
| `SM_Ishibashiri_Rock_A` | `rock_moss_set_01` (`rock_moss_set_01_rock01`) | CC0 1.0 |
| `SM_Ishibashiri_Rock_B` | `rock_moss_set_01` (`rock_moss_set_01_rock04`) | CC0 1.0 |
| `SM_Ishibashiri_FallenCedar_A` | `dead_tree_trunk` | CC0 1.0 |
| `SM_Ishibashiri_Fern_A` | `fern_02` (`fern_02_b`) | CC0 1.0 |
| `M_Ishibashiri_Ground` | texture `forest_ground_04` (2k) | CC0 1.0 |
| `M_Ishibashiri_BoundaryStone`, `M_Ishibashiri_SteppingStone` | texture `mossy_rock` (1k) | CC0 1.0 |
| `M_Ishibashiri_RitualWood` | texture `rough_wood` (1k), faded vermilion tint | CC0 1.0 |
| `M_Ishibashiri_RopeStraw` / `M_Ishibashiri_RopePaper` | textures `thatch_roof_angled` / `rough_linen` (1k) | CC0 1.0 |

Procedural meshes and decals from `Tools/CreateIshibashiriPropKit.py` (Blender, seed 830194, no external input):

| UE asset | Use | Size |
| --- | --- | --- |
| `SM_Ishibashiri_RitualPost_A` | Approach ritual posts, Basin stakes | 25 × 25 × 322 cm, 8.5k tris |
| `SM_Ishibashiri_OldRope_A` | shimenawa with straw tassels and shide; pivot at end A, local X to end B | 1603 cm span, 20.8k tris |
| `SM_Ishibashiri_SteppingStone_A` | Basin procession stones | 316 × 124 × 14 cm |
| `D_Ishibashiri_Footprint_A` | Approach giant cloven footprints (deferred decal) | 1024² BaseColor+alpha, Normal |
| `D_Ishibashiri_CorruptionCrack_A` | Approach black fissures, no emissive (deferred decal) | 1024² BaseColor+alpha, Normal |

The Approach reveal silhouette reuses the existing rigged `SK_Ishibashiri` with its Walk clip; it stays inert (no AI, collision, Grab or Kakon). Ritual post and rope sizes follow the existing blockout placements (3.1–4.8 m posts, 14–18 m spans) rather than the older manifest ranges.

`sources.json` records URL, size, SHA256 and authors for every downloaded file. `ue-import-report.json` records the imported slots, dimensions and per-LOD triangle counts.

## Regenerate

1. `Tools/FetchPolyHavenEnvironment.py` (any Python 3) downloads into `Saved/ModelDownloads/PolyHaven`.
2. `blender --background --factory-startup --python Tools/PreparePolyHavenEnvironment.py` writes FBX, LOD FBX and reports to `Art/Environment/Ishibashiri/Generated/PolyHaven` (gitignored). `POLYHAVEN_ASSETS=<asset>[,<asset>]` limits the run. `blender --background --factory-startup --python Tools/CreateIshibashiriPropKit.py` writes the procedural props and decal maps to `Generated/Props`.
3. `UnrealEditor-Cmd.exe IshibashiriPrototype.uproject -ExecutePythonScript=Tools/ImportPolyHavenEnvironment.py -unattended -nullrhi` imports into `Content/Environment/Ishibashiri`.

## Cedar processing

- Needle geometry (4.07M source triangles) is thinned to 400k at LOD0; kept needle islands are enlarged to hold the crown silhouette. Islands on the opaque stem strip of the twig atlas are not enlarged.
- Only the trunk is decimated. Branch tubes are kept intact because collapsing them produces flat dark fins.
- LOD1–LOD3 are authored in Blender and imported with `import_lod`; engine LOD reduction turns alpha needles into opaque spikes.
- Each LOD keeps fewer needle cards and grows them by `sqrt(LOD0 cards / kept cards)`, so the crown keeps the LOD0 card area. With the earlier fixed 1.6× scale, LOD3 kept about 5% of that area and the untouched branch tubes read as bare dead trees in the Campaign arena, where every cedar is 60 m or more away. The prepare report records the card scale per LOD.
- Dead-branch cards are dropped because the 1k glTF ships no cutout map for them.

## Verification (2026-10-04)

- PASS: Blender prepare, UE 5.6.1 import (cedar LOD triangles 434068 / 135846 / 55213 / 32582), `-Action Test -EnvironmentReview -Approach|-Basin -Capture` exit 0, `python -m pytest Tests` 212 passed.
- Captures inspected for the Approach and Basin review cameras: no black spikes on cedars and ground tiling is uniform.
- Props, variants and decals (same day): UE import PASS (cedar B LOD 321395 / 98715 / 37126 / 19533), Build PASS, both captures exit 0 with every `ENVIRONMENT_KIT` flag = 1, `pytest` 212 passed, gameplay contract digests match. Captures show posts with rope, footprint and crack decals, cedar/rock variants, stepping stones and the rigged reveal silhouette.
- NOT_RUN: frame-rate measurement, interactive play in the game window, hardware input, human visual approval.
- Campaign arena dressing (same day): Build PASS; smoke test body PASS with every `ENVIRONMENT_KIT arena` flag = 1 (exit 1 is the existing `04-Victory.png` lookup); `-Climbing` and `-Grab` PASS; `-PrimitiveEnvironment` smoke PASS with all flags = 0 and the original blockout. The distant cedars read sparse at 60–90 m; frame rate, Campaign E2E and human visual approval are NOT_RUN.
- Distant cedar coverage (same day): Blender prepare and UE import PASS (cedar A LOD 434068 / 135846 / 65213 / 40582, cedar B 321395 / 98715 / 43124 / 24534). In the smoke `01-Dodge` capture, non-sky pixels in the top tenth of the frame (cedar crowns above the rock rim) rose from 5372 to 9984. `-EnvironmentReview -Basin|-Approach -Capture` PASS; mid-distance crowns are fuller and LOD0 cedars are unchanged. Alpha-coverage mip scaling on the twig mask was also tried and lowered the same count (9984 to 6969), so it is not used. Frame rate and human visual approval are NOT_RUN.
