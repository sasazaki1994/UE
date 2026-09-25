# SM_Ishibashiri_Rock_B

## Tripo production prompt (paste as-is)

```text
Natural fractured Japanese mountain granite rock, approximately 1.8 to 3.2 meters long, 1.6 to 2.8 meters wide, and 2.4 to 4.2 meters high relative to a 1.72 meter human. Upright offset silhouette distinct from Rock A, with one weathered split and uneven stepped planes, usable in mountain path, cliff clusters, and basin perimeter without resembling a monument. Dark charcoal and deep gray wet rough stone, restrained moss only in cracks and lower shade, chipped and rain-aged. Single unified shared rock material region, grounded natural asymmetry, rotation and scale reusable. Game-ready isolated static environment asset, efficient LOD-ready topology, full object visible, neutral even lighting, no background, no unnecessary pedestal, no fantasy exaggeration.
```

## Negative prompt / reject conditions

```text
anime, cute, crystal, obelisk, statue, menhir, carved writing, shrine motif, symmetrical, polished or clean stone, neon, glow, sci-fi, lava, excessive moss, pedestal, terrain chunk, text, watermark, floating shards, cropped base
```

## Asset-specific Blender cleanup

Remove disconnected shards/internal shells, repair practical non-manifold geometry, apply transforms, verify dimensions and stable Z=0 contact, origin at contact center. Inspect UV/texel density and reuse Rock A material where possible. Validate normals and texture paths. Make manual silhouette-aware LODs, retain the defining split, prepare simple convex collision, and validate export; no unconditional auto-Decimate.

## Common Blender/export gate

- Remove disconnected geometry; remove hidden internal geometry where safe; fix non-manifold geometry where practical.
- Apply transform; set the specified origin; verify centimeter-equivalent scale against `manifest.json`.
- Inspect UVs; clean material slots; validate normals/tangents and texture paths.
- Prepare LODs and collision proxy as specified; validate UE-oriented export and exact asset naming.
- **Do not apply automatic Decimate unconditionally.** Preserve the long-range silhouette and compare every LOD. Nanite remains **UE RUNTIME DECISION**.
- Generated source, Blender cleanup, UE import, and visual review are currently **NOT_RUN**.
