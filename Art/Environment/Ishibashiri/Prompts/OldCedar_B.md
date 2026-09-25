# SM_Ishibashiri_OldCedar_B

## Tripo production prompt (paste as-is)

```text
Old Japanese mountain cedar tree, 16 to 22 meters tall relative to a 1.72 meter human, secondary silhouette clearly different from Cedar A: mild lower-trunk sweep, one uneven crown bias, slightly broader sparse canopy, few lower branches. Ancient dark brown-gray rain-wet fissured bark, restrained moss on shaded root flare, small healed breaks and storm aging. Clear separation of trunk/bark and foliage for at most two material slots. Production-realistic hybrid of geometry trunk and major branches with restrained masked needle cards. Natural humid mountain proportions, asymmetric but stable, reusable through rotation and 0.85 to 1.15 scaling. Game-ready isolated static asset, efficient LOD-ready topology, full object visible including root contact and crown, neutral even light, no background, no unnecessary pedestal, no fantasy exaggeration.
```

## Negative prompt / reject conditions

```text
anime, cute, bonsai, clone of a perfectly straight cedar, broadleaf or tropical foliage, huge umbrella crown, excessive low branches, symmetry, ornate shrine decoration, carved symbols, magic glow, neon, sci-fi, polished bark, clean young tree, pedestal, terrain base, text, watermark, crop, floating parts
```

## Asset-specific Blender cleanup

Keep the silhouette distinct from A; remove disconnected foliage and hidden internals safely, repair practical non-manifold areas, apply transforms, verify manifest scale, and put origin at trunk base Z=0. Inspect UVs/card padding; reduce to shared Cedar A bark/foliage slots where credible. Validate normals, texture paths, manual silhouette-aware LODs, trunk-only capsule collision, and export. Never auto-Decimate without visual comparison.

## Common Blender/export gate

- Remove disconnected geometry; remove hidden internal geometry where safe; fix non-manifold geometry where practical.
- Apply transform; set the specified origin; verify centimeter-equivalent scale against `manifest.json`.
- Inspect UVs; clean material slots; validate normals/tangents and texture paths.
- Prepare LODs and collision proxy as specified; validate UE-oriented export and exact asset naming.
- **Do not apply automatic Decimate unconditionally.** Preserve the long-range silhouette and compare every LOD. Nanite remains **UE RUNTIME DECISION**.
- Generated source, Blender cleanup, UE import, and visual review are currently **NOT_RUN**.
