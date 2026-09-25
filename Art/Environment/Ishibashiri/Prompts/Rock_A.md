# SM_Ishibashiri_Rock_A

## Tripo production prompt (paste as-is)

```text
Natural Japanese mountain granite rock mass, approximately 2.8 to 4.2 meters long, 2.0 to 3.4 meters wide, and 1.8 to 3.0 meters high relative to a 1.72 meter human. Low broad grounded silhouette suitable for path shoulders, cliff grouping, and a basin perimeter. Irregular fractured planes, dark charcoal to deep gray mineral, rain-wet rough surface with subtle roughness variation, limited moss in creases and ground-facing areas, chipped edges and long weathering. One clearly unified rock material region, no embedded architecture. Natural asymmetry and plausible weight; useful under rotation and scale variation. Game-ready isolated static asset, efficient LOD-ready topology, full object visible including underside contact, neutral even lighting, no background, no unnecessary pedestal, no fantasy exaggeration.
```

## Negative prompt / reject conditions

```text
anime, stylized cute, crystal, gemstone, monolith, giant decorative hero rock, statue, carved rune, shrine symbol, face, symmetrical, polished stone, marble, neon, emissive, magic glow, sci-fi, lava, snow, excessive moss, pedestal, terrain island, text, watermark, floating chunks, cropped object
```

## Asset-specific Blender cleanup

Remove floating chips and hidden internal shells safely; repair practical non-manifold seams. Apply transforms, verify dimensions, place the broad stable contact on Z=0, and set origin at ground-contact center. Inspect UV scale/seams, reduce to one shared rock slot, validate normals/tangents and paths. Author silhouette-aware LODs; do not flatten fracture planes with blind Decimate. Prepare one or a few simple convex hulls, never complex-as-simple, then validate export.

## Common Blender/export gate

- Remove disconnected geometry; remove hidden internal geometry where safe; fix non-manifold geometry where practical.
- Apply transform; set the specified origin; verify centimeter-equivalent scale against `manifest.json`.
- Inspect UVs; clean material slots; validate normals/tangents and texture paths.
- Prepare LODs and collision proxy as specified; validate UE-oriented export and exact asset naming.
- **Do not apply automatic Decimate unconditionally.** Preserve the long-range silhouette and compare every LOD. Nanite remains **UE RUNTIME DECISION**.
- Generated source, Blender cleanup, UE import, and visual review are currently **NOT_RUN**.
