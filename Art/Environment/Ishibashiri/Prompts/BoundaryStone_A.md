# SM_Ishibashiri_BoundaryStone_A

## Tripo production prompt (paste as-is)

```text
Small old Japanese mountain boundary stone pillar, 1.7 to 2.2 meters tall relative to a 1.72 meter human, narrow plain rectangular form with a subtle authored 4 to 9 degree lean, stable buried base, irregular chipped top and softened edges. Humble weathered dark gray local stone, decades of rain staining, restrained moss and lichen, shallow surface erosion, no luxury finish. Keep the front reading area visually quiet and blank so separate Unreal gameplay information reading “ここから先へ入るな” remains legible; do not create letters, inscriptions, symbols, or a text recess. One stone material region, grounded human-made but modest and naturally aged. Game-ready isolated static asset, efficient LOD-ready topology, full object visible including base, neutral even lighting, no background, no unnecessary pedestal, no fantasy exaggeration.
```

## Negative prompt / reject conditions

```text
text, Japanese characters, letters, runes, engraved symbols, shrine crest, statue, torii, ornate shrine, grave marker, polished stone, marble, perfect symmetry, perfect clean surface, neon, glow, magic, sci-fi, gold, pedestal, terrain chunk, background, watermark, floating parts, cropped base
```

## Asset-specific Blender cleanup

First reject any generated writing or symbol rather than attempting to preserve it. Remove disconnected chips and hidden internals safely; repair practical non-manifold areas. Apply transforms while retaining the authored visual lean; verify manifest dimensions and stable ground contact, origin at ground center. Inspect UVs and keep the front quiet, consolidate to one stone slot, validate normals/paths. Author LODs that retain lean and chipped crown. Prepare simple box/convex collision and validate export. Do not auto-Decimate blindly.

## Common Blender/export gate

- Remove disconnected geometry; remove hidden internal geometry where safe; fix non-manifold geometry where practical.
- Apply transform; set the specified origin; verify centimeter-equivalent scale against `manifest.json`.
- Inspect UVs; clean material slots; validate normals/tangents and texture paths.
- Prepare LODs and collision proxy as specified; validate UE-oriented export and exact asset naming.
- **Do not apply automatic Decimate unconditionally.** Preserve the long-range silhouette and compare every LOD. Nanite remains **UE RUNTIME DECISION**.
- Generated source, Blender cleanup, UE import, and visual review are currently **NOT_RUN**.
