# SM_Ishibashiri_OldCedar_A

## Tripo production prompt (paste as-is)

```text
Old Japanese mountain cedar tree, 18 to 24 meters tall relative to a 1.72 meter human, tall narrow readable silhouette, old trunk with a subtle natural bend, sparse lower branches and a restrained irregular crown. Aged dark brown to gray fissured bark, rain-darkened surface, subtle patchy moss only near sheltered bark and root flare, broken minor limbs and weathering without a hollow fantasy face. Separate trunk/bark from foliage clearly for no more than two game material slots. Use a production-realistic hybrid construction: geometry for trunk and silhouette-critical primary branches, restrained masked cards for needle clusters and fine twigs. Grounded realistic Japanese humid mountain forest proportions, natural asymmetry, reusable under rotation and 0.85 to 1.15 scale variation. Game-ready isolated static environment asset, coherent manifold surfaces where practical, efficient topology suitable for authored LODs, complete roots and crown, full object visible from base to top, neutral even lighting, no background, no unnecessary pedestal, no fantasy exaggeration.
```

## Negative prompt / reject conditions

```text
anime, stylized cute, bonsai, broadleaf tree, tropical tree, lush low branches, excessive branches, dense spherical crown, perfectly straight or symmetrical trunk, giant magical roots, carved face, shrine ornaments, paper charms, neon, emissive, magic glow, sci-fi, polished bark, perfect clean surface, snow, flowers, fruit, pedestal, terrain chunk, background, text, letters, watermark, cropped base, cropped crown, floating or disconnected parts
```

## Asset-specific Blender cleanup

Preserve the narrow crown and trunk bend while removing floating needles/disconnected geometry. Delete hidden internal trunk/branch geometry where safe; repair non-manifold bark where practical. Apply transforms; set Z-up scale to the manifest range and origin at centered trunk base on Z=0. Inspect UV islands and foliage-card padding, consolidate to bark plus foliage slots, validate outward normals/tangents and texture paths. Build LODs manually around the silhouette; do not blindly auto-Decimate. Prepare trunk-only capsule collision proxies and validate export naming.

## Common Blender/export gate

- Remove disconnected geometry; remove hidden internal geometry where safe; fix non-manifold geometry where practical.
- Apply transform; set the specified origin; verify centimeter-equivalent scale against `manifest.json`.
- Inspect UVs; clean material slots; validate normals/tangents and texture paths.
- Prepare LODs and collision proxy as specified; validate UE-oriented export and exact asset naming.
- **Do not apply automatic Decimate unconditionally.** Preserve the long-range silhouette and compare every LOD. Nanite remains **UE RUNTIME DECISION**.
- Generated source, Blender cleanup, UE import, and visual review are currently **NOT_RUN**.
