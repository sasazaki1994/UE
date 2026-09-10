# CC0 PBR source cache

This is the offline cache consumed by the character baker. `manifest.json` is
deliberately unresolved: both official providers returned `403 Forbidden` from
the 2026-09-10 environment. No asset ID, URL, license assertion, or hash has been
invented.

On a connected workstation, resolve one official CC0 asset per role, store its
2K maps under `files/`, and run `python Tools/ManageCharacterPBR.py verify`.
Each map record contains `url`, repository-relative `path`, and `sha256`.
Normals must be OpenGL (`normal_gl`); UE performs the one required Y flip.
