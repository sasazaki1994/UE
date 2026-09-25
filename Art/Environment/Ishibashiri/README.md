# Ishibashiri minimal environment production package

This directory is a **specification-only handoff**, not an asset drop. It defines the eight reusable meshes needed to replace the most visible Approach/Basin primitives. No FBX, GLB, texture, UE import, Tripo output, or rendered capture is claimed.

## Production order

1. `SM_Ishibashiri_OldCedar_A`
2. `SM_Ishibashiri_Rock_A`
3. `SM_Ishibashiri_BoundaryStone_A`
4. `SM_Ishibashiri_FallenCedar_A`
5. `SM_Ishibashiri_OldCedar_B`
6. `SM_Ishibashiri_Rock_B`
7. `SM_Ishibashiri_RitualPost_A`
8. `SM_Ishibashiri_OldRope_A`
9. Decals (specification only)

The first three entries are the **First Adoption Batch**. Run Tripo → Blender cleanup → UE import → temporary Approach placement → matched-camera comparison for only those three before producing the remaining five. They cover the largest share of the Approach image and test organic foliage, natural rock, and a restrained human-made landmark.

`manifest.json` is the machine-readable authority. `Prompts/` contains paste-ready Tripo prompts and asset-specific cleanup checks. See `Docs/Art/IshibashiriEnvironmentKit.md` for workflow, shared material rules, capture contract, and acceptance criteria.

Status: **SPEC_ONLY — UE / Blender / Tripo NOT_RUN**.
