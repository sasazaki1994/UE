# Free character models

These assets are included under CC0 1.0. No purchases or online account are required to build or run this project.

| Asset | Creator | Original download page | License |
| --- | --- | --- | --- |
| Warrior, sword, textures and animations from LowPoly RPG Characters | Quaternius | https://opengameart.org/content/lowpoly-rpg-characters | CC0 1.0 |
| Boar, texture and animations | Teh_Bucket | https://opengameart.org/content/boar | CC0 1.0 |

License: https://creativecommons.org/publicdomain/zero/1.0/

Sources downloaded on 2026-09-07. The original Warrior license is preserved in `Sources/Warrior/License.txt`. The Boar author's distribution page specifies CC0.

`Sources/` contains the selected original Blender files and Warrior textures. `Export/` contains the FBX and PNG files consumed by the Unreal importer. Only the Warrior from the six-character pack is included.

Conversion changes: join the Warrior meshes, convert rigid bone-attached sword and shoulder pads to skin weights, bake each selected source animation separately, export without extra leaf bones, extract the Boar's packed texture. The original source files are preserved. UE materials use the original albedo multiplied by a gameplay state tint.

## Rebuild assets

Normal builds use the checked-in assets in `Content/Characters/FreeModels/` and do not need Blender or downloads.

To regenerate from the sources, run Blender 3.6.23 from the repository root:

```powershell
& '<Blender path>/blender.exe' -b --disable-autoexec --python Tools/ExportFreeModels.py
powershell.exe -NoProfile -ExecutionPolicy Bypass -File Tools/Prototype.ps1 -Action ImportModels
```

`ImportModels` replaces only the imported character assets under `/Game/Characters/FreeModels/`. It recreates their materials. Keep manual artwork changes in the source files or conversion script before reimporting.

The prototype uses source animations without root motion to preserve existing C++ movement and hit timing. The Warrior roll and slash are sped up to fit the existing dodge/attack durations. The Boar's walk cycle is accelerated during a charge; the source asset does not include a dedicated gallop or collapse animation.
