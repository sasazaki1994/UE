# SM_Ishibashiri_RitualPost_A

## Tripo production prompt (paste as-is)

```text
Plain old wooden ritual stake for a remote Japanese mountain boundary, 1.5 to 2.1 meters tall relative to a 1.72 meter human, narrow hand-hewn post with slightly uneven square-to-round section, blunt weathered top and simple buried base. Dark rain-soaked aged timber, longitudinal cracks, softened edges, minor rot and restrained moss near ground. One wood material region with readable grain; no attached rope or paper so the modular OldRope asset can be combined separately. Humble utilitarian object, natural asymmetry, reusable in Approach and Basin. Game-ready isolated static asset, efficient LOD-ready topology, full object visible, neutral even lighting, no background, no unnecessary pedestal, no fantasy exaggeration.
```

## Negative prompt / reject conditions

```text
anime, cute, ornate shrine, torii, lantern, totem, carved face, writing, runes, paper charms, attached rope, metal ornament, polished lacquer, bright red paint, symmetrical perfect surface, neon, magic glow, sci-fi, pedestal, terrain, watermark, floating pieces, cropped base
```

## Asset-specific Blender cleanup

Remove disconnected splinters/hidden internals where safe and repair practical non-manifold areas. Apply transforms, verify dimensions, set base at Z=0 and origin at base center. Inspect UV grain direction, reduce to one wood slot, validate normals and paths. Retain top and crack silhouette through manual LODs. Prepare simple capsule or box collision and validate export; avoid blind auto-Decimate.

## Common Blender/export gate

- Remove disconnected geometry; remove hidden internal geometry where safe; fix non-manifold geometry where practical.
- Apply transform; set the specified origin; verify centimeter-equivalent scale against `manifest.json`.
- Inspect UVs; clean material slots; validate normals/tangents and texture paths.
- Prepare LODs and collision proxy as specified; validate UE-oriented export and exact asset naming.
- **Do not apply automatic Decimate unconditionally.** Preserve the long-range silhouette and compare every LOD. Nanite remains **UE RUNTIME DECISION**.
- Generated source, Blender cleanup, UE import, and visual review are currently **NOT_RUN**.
