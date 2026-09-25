# SM_Ishibashiri_OldRope_A

## Tripo production prompt (paste as-is)

```text
Modular old Japanese shimenawa-style rope segment, 2.5 to 4.0 meters between attachment ends, 5 to 9 centimeters thick, with a restrained natural sag of 20 to 55 centimeters. Simple heavy twisted plant-fiber rope, dry-gray and rain-dark tan variation, worn fuzzy surface, a few controlled short frays, aged and strained but continuous. Both attachment ends cleanly readable and suitable for connecting boundary stone, ritual posts, or cedar; shape should also serve as a reference for a future Unreal spline mesh. One rope fiber material region. Not a hero asset and no attached bells, paper, ornaments, hooks, posts, or stones. Game-ready isolated static asset, efficient LOD-ready topology, full object visible end to end, neutral even lighting, no background, no unnecessary pedestal, no fantasy exaggeration.
```

## Negative prompt / reject conditions

```text
anime, cute, ornate sacred decoration, paper shide, bells, gold fittings, chain, wire, vines, glowing rope, neon, magic, sci-fi, perfectly straight, perfect clean fiber, excessive frays, broken floating strands, knot sculpture, noose, pedestal, background, text, watermark, cropped ends
```

## Asset-specific Blender cleanup

Remove floating fibers and hidden internal geometry while retaining only silhouette-relevant frays; repair practical non-manifold areas. Apply transforms, verify dimensions, align local X from attachment A to B, and set origin at attachment A. Preserve/document both attachment points for sockets or spline conversion. Inspect UV direction/seams, use one material, validate normals/paths. Author LODs that keep thickness and sag; prepare NoCollision only and validate export. Never blind-Decimate thin strands.

## Common Blender/export gate

- Remove disconnected geometry; remove hidden internal geometry where safe; fix non-manifold geometry where practical.
- Apply transform; set the specified origin; verify centimeter-equivalent scale against `manifest.json`.
- Inspect UVs; clean material slots; validate normals/tangents and texture paths.
- Prepare LODs and collision proxy as specified; validate UE-oriented export and exact asset naming.
- **Do not apply automatic Decimate unconditionally.** Preserve the long-range silhouette and compare every LOD. Nanite remains **UE RUNTIME DECISION**.
- Generated source, Blender cleanup, UE import, and visual review are currently **NOT_RUN**.
