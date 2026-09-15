# STEP 3D — Ishibashiri Approach / Exploration Vertical Slice

## Source commit and objective

- Starting `main`: `1cc93bd9b8e4381c8554b3cdf783f005128cb88f` (local PR #47 merge). The remote check returned HTTP 403, so remote freshness is **UNVERIFIED**.
- Objective: make the quiet three-to-five minutes before meeting Ishibashiri playable without an open world, combat, NPCs, loot, or a new exploration framework.
- Asset policy: only Engine Basic Shapes are used. `Art/References/Tripo/` remains reference-only; no absent production asset is represented as imported.

## Campaign position

`Title → Prologue → IshibashiriApproach → Ishibashiri → Interlude1` is now the opening sequence. The thin chapter has its own GameMode and arena. Reaching its gate performs one guarded `CompleteApproach()` transition; encounter completion remains owned by the existing Ishibashiri manager.

## Path structure and duration

The deterministic path is approximately **1,311 m** (runtime logs the computed polyline length). At the player's 6 m/s walk speed it takes about 3.6 minutes without stops, and 3–5 minutes while observing landmarks.

Its 13 points form occluded beats rather than a visible corridor: entrance → rock bend → cedar enclosure → boundary clearing → cliff turn → damaged grove → distant reveal gap → second forest → basin gate. Broad overlapping dirt slabs keep navigation forgiving and maze-free.

## Environment placement

`AIshibashiriApproachArena` generates wet earth, ink-dark rock shoulders, moss, old cedars, a fallen cedar, one tilted boundary stone, faded ritual posts, decayed shimenawa, four giant footprints, a gouged rock and five small black corruption cracks. Corruption is non-interactive and has no bright red emissive.

## Rumble stages and reveal

1. At the boundary clearing, a distant-rumble hook fires and the stone shows only `ここから先へ入るな` for four seconds.
2. At the damaged grove, footprints, gouged rock and felled tree form the trail.
3. At the valley gap, an inert Ishibashiri-shaped primitive silhouette crosses once for exactly **3.0 seconds**. It has no AI, collision, combat, damage, Grab or Kakon state.
4. Before the gate, a close-rumble/camera-reaction hook fires. Camera ownership is not changed.

No audio files were invented. Audio locations log `NOT_IMPLEMENTED — AUDIO ASSET REQUIRED`.

## Sense support

The existing `UPlayerSenseComponent` is reused. Boundary Sense receives one available, non-gameplay target at the basin gate, so `Q / LT` strengthens while approaching. Held Corruption Sense moves from `QUIET` to `TRANSITION` after the trail and `DANGER` near the basin. Chapter travel retains the existing `ResetSense()` path.

## Transition and retry

The gate opens the existing `PrototypeGameMode`; campaign entry first changes state to `Ishibashiri`. Approach never spawns a boss, encounter manager, Kakon or retry timer. Standalone Ishibashiri and `-Basin` retain their startup paths.

## Tests

- Gherkin acceptance coverage specifies ordered traversal, one-shot reveal, non-combat behavior, gate transition and cleanup.
- Python contracts verify primitive construction, landmarks, one three-second reveal, absence of boss reuse, Sense wiring and missing-audio markers.
- UE campaign lifecycle automation now requires `CompleteApproach()` before Ishibashiri.
- `Tools/Prototype.ps1 -Action Test -Approach -TestFPS 60` (and 30) uses a controller-input path driver and expects `APPROACH_TEST_PASS`.
- `-CampaignE2E` uses the same input driver; it does not mutate position, Kakon, Phase, Stamina, Progress, Completed, or Campaign State.

## Screenshots

No Windows UE 5.6.1 runtime is installed in this container, so screenshots were not fabricated. Future capture set: entrance, boundary stone, footprints, first distant sight, path after reveal, basin entrance, encounter start.

## Known issues and runtime validation status

- **Runtime status: UNVERIFIED.** C++ compilation, 60/30 FPS traversal, Campaign E2E, visuals, collisions, gate travel, and screenshots require Windows UE 5.6.1.
- Audio and subtle tree/camera reactions are hook-only.
- The distant figure is intentionally a blockout proxy, not an imported Tripo model.
- Production replacement priorities: terrain surface; cliff/rock and cedar/fallen-tree kits; boundary stone, rope and ritual posts; footprint/corruption decals; fog; Ishibashiri LOD silhouette; rumble/tree audio; brief camera impulse.

Final source-only verdict: **SOURCE IMPLEMENTED — UE RUNTIME UNVERIFIED**.
