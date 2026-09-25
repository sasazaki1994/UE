# SM_Ishibashiri_FallenCedar_A

## Tripo production prompt (paste as-is)

```text
Fallen old Japanese mountain cedar, 11 to 15 meters long relative to a 1.72 meter human, heavy tapered trunk resting low with a broken root end and snapped crown end, only a few short branch stubs so a gameplay route can remain open. Rain-dark aged brown-gray bark, fresh and old split wood at damage, mud contact, restrained moss on the upper sheltered side, credible crushing and scrape marks suggesting a giant animal passed nearby. Separate bark/wood from any sparse retained foliage with no more than two material slots. Grounded natural asymmetry, not a barricade or hero sculpture. Game-ready isolated static environment asset, efficient LOD-ready topology, full object visible end to end, neutral even lighting, no background, no unnecessary pedestal, no fantasy exaggeration.
```

## Negative prompt / reject conditions

```text
anime, cute, magical log, hollow tunnel, giant root wall, dense branch cage, route-blocking branch spread, ornate carving, shrine ornament, ropes or charms, neon, emissive, sci-fi, polished wood, clean saw cut, lumberyard log, symmetrical, pedestal, terrain chunk, text, watermark, cropped ends, floating parts
```

## Asset-specific Blender cleanup

Remove disconnected splinters unless silhouette-critical and safe; delete hidden interiors while retaining visible broken-end depth; repair practical non-manifold areas. Apply transforms and align local X to trunk length. Verify dimensions, set origin at the logical root-side ground contact, inspect UVs, consolidate shared cedar materials, validate normals and paths. Author LODs without destroying broken-end silhouette. Prepare a simple capsule/box chain or low-poly proxy; also document a NoCollision placement option. Validate export.

## Common Blender/export gate

- Remove disconnected geometry; remove hidden internal geometry where safe; fix non-manifold geometry where practical.
- Apply transform; set the specified origin; verify centimeter-equivalent scale against `manifest.json`.
- Inspect UVs; clean material slots; validate normals/tangents and texture paths.
- Prepare LODs and collision proxy as specified; validate UE-oriented export and exact asset naming.
- **Do not apply automatic Decimate unconditionally.** Preserve the long-range silhouette and compare every LOD. Nanite remains **UE RUNTIME DECISION**.
- Generated source, Blender cleanup, UE import, and visual review are currently **NOT_RUN**.
