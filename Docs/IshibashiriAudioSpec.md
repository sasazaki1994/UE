# 石走り編 Audio Production Specification

Status: **DESIGN LOCK / AUDIO ASSETS `ASSET_REQUIRED` / runtime audition `NOT_RUN`**  
Baseline: local `main` fetch record and current HEAD `27a4e6f91db5a77f1a897865927a930af6b5147d` (2026-09-28). Network fetch was retried but HTTP 403; this SHA is therefore the newest main that can be evidenced in this checkout, not a claim about an unreachable remote.

This document is subordinate to `IshibashiriFinalGameplaySpec.md`, `IshibashiriHUDInformationSpec.md`, and `IshibashiriCameraSpec.md`. It fixes what audio communicates; it does **not** claim that a WAV, SoundCue, MetaSound, mix, or notify exists.

## 1. Audio philosophy and evidence boundary

The mix must let a player who briefly looks away recognize **Charge, Shake/Cling, low stamina, mount availability, purification, and Calm**, while every critical cue is also expressed by body motion, local light/VFX, haptics or optional UI. Silence is authored information: no constant combat bed, no repeated UI chirp, and no event is allowed to retrigger every Tick.

The source has gameplay hook points (`Telegraph`/`Charge`/`Recover`, Kneel, Grab, buck warning/buck, stamina, `HandleCorePurified`, Calm and both Sense holds), but contains no `USound`, playback call, audio component, WAV/OGG/MP3, SoundCue or MetaSound connection. `Audio/CommonSfx/manifest.json` lists six **pending** roles and contains no adopted files. Thus all sound below is a production contract, not implemented audio. Existing animation clips, primitive material pulses, dynamic lights and HUD/debug text are visual evidence only.

### Four classes and priority

| Class | Mix rule | Events |
|---|---|---|
| **A Gameplay Critical** | Never virtualize inside its gameplay radius; duck BGM 3–6 dB; one clear onset; visual redundancy mandatory. | Charge wind-up/start/near pass; Shake warning/Cling moment; stamina danger; Grab open; purification confirmation; 3/3 and Calm transition. |
| **B Gameplay Support** | May yield to A; rate-limit and avoid loops where state does not change. | Recover, counter/posture layers, rest/recovery, Boundary strength, Corruption-Sense category change, Kakon proximity. |
| **C World / Creature** | Sparse randomized beds with distance attenuation; silence between gestures. | forest/wind/birds, hoof, breath, hide/rock, rope/bell, tree movement, ground impact, climbing contacts. |
| **D Emotional / Narrative** | Used only at authored beats; never grants authority. | first silhouette, encounter threshold, short post-purify void, 3/3 release, Calm, ending. |

## 2. Event specification

### Approach — quiet forest to enormous presence

No continuous BGM. A quiet 3D forest bed (wind, leaves, sparse birds) leaves at least 2–4 s of low-detail space. The first boundary stone slightly filters nearby insects (**B/C**, once); it does not chime like a collectible. Distant ground/hoof events (**C**) begin below the visible threshold at irregular 7–14 s intervals, with no exact directional ping until the silhouette is relevant. One tree-groan/branch response follows a distant step, not every step. At the first silhouette, birds stop locally and one breath/stone-settle gesture occupies the cleared space (**D**). At the encounter boundary, the closest hoof weight and low breath become dry and directional; music remains absent. This creates “quiet forest → displaced wildlife/ground → a huge living body,” not “music says boss.”

### Charge, Recover and posture

| Beat | Class | Sound contract |
|---|---|---|
| Chase/tracking | C | Slow, position-locked hoof and breath cadence; no warning ostinato. |
| 1.00 s Telegraph | **A** | A unique three-part bodily phrase: hoof scrapes/loads the soil (onset), inhalation and low growl rise (middle), granite/root tension catches (end). It must be identifiable in mono and start at least 0.8 s before launch. |
| Charge start | **A** | Weight release plus first heavy hoof; distinct from impact, no synthetic alarm. Directional 3D source follows the chest/forelegs. |
| Approach/near | **A** | Increasing hoof/transient density and low-frequency ground layer; conservative loudness and near-field limiter, with haptics/edge cue as redundancy. |
| Pass | A/C | Short air/soil sweep pans past; never reused as “player hit.” |
| Wall/ground impact | A/C | Dry soil impact + stone/body layer; strongest transient in the charge family, then space. |
| Recover | **B** | Exhalation, hoof slide and rock settling; descending contour means “counter now,” paired with lowered body/local neutral light. |
| Counter success | B | Weapon/body contact plus one muted stone fracture. Each success adds one higher rock-tension overtone, not a UI stack tone. |
| Final posture break | A/B | The third fracture resolves downward into foreleg weight loss; it is longer/heavier than counters 1–2. |
| Kneel | A/C | Both knees/forequarters meet soil, rope/stone settle, breath opens. No death impact. |
| Grab open | **A** | One restrained rope/fitting tick over the kneel tail; never loops during the five-second window. Gold/kinari hold light and contextual input duplicate it. |

### Grab and climbing

Grab success is tactile, not celebratory UI: hand catches bristle/rope/rock, cloth strains, then the player's breath stabilizes (**B/C**, 3D body contact). A miss uses only hand/cloth brushing and a short uncommitted exhale (**B**); it must not resemble success. The first upward weight transfer marks climbing start with cloth/hold strain. Normal climbing selects parameter variants of one contact family—granite is dry/high, bristle soft/damped, shimenawa fibrous with a restrained fitting/bell tick—rather than separate systems.

Rest is **B**: contact noise stops, breath decelerates and wind returns; no “safe” voice or fanfare. Stamina drain has no per-tick sound. Below the HUD low threshold, breath becomes shorter and cloth strain appears (**B**). At the danger threshold, a clearly patterned two-breath cycle plus rope/hand slip texture enters (**A**), at most once per 1.5 s and without electronic beeps. Recovery reverses breath cadence; full recovery gets no confirmation chime. High wind is a C loop parameterized by route height and ducked under Shake warnings.

### Shake / Cling timing contract

| Phase | Class | Required distinction |
|---|---|---|
| 1. warning starts (2.0 s before Shake) | **A** | Breath abruptly holds/changes, body rock/root tension rises and the rope/fitting gives one irregular tick. This onset is the learnable “prepare” cue. |
| 2. Cling moment (last 0.45 s before Shake) | **A** | Tension reaches a short low-frequency catch plus one dry bell/fitting contact. This is the precise cue on which E/RB can be pressed; it is not a beep. |
| 3. Shake active (2.0 s) | A/C | Buck body/rock/rope motion, broad but lower in transient priority than the warning onset. |
| 4. Cling success | A/B | Hands/cloth cinch and player controlled exhale; do not play victory music. |
| 5. Cling failure | A | Grip slip/rope scrape, breath break; distinct from successful cinch. |
| 6. fall | A/C | Wind rises then body/landing response; checkpoint recovery sound is grounded and non-punitive. |

The warning and precise moment must survive music-off, stereo-to-mono, and a -12 dB SFX accessibility test. If the event cannot be heard, the body anticipation, local cue, controller pulse and optional `しがみつく` prompt still carry it.

### Kakon and Sense

Kakon sounds like an embedded foreign body: intermittent stone pressure, sticky root/fiber tension and a barely pitched dark resonance, never sparkle. Far is normally silent; while Boundary Sense is held, **far/medium/near** use the same restrained texture with increasing clarity and shorter spacing (not louder-only, no exact sonar distance). Very near adds a localized stressed seam and is still **B**. Corruption Sense has no constant loop: only category changes produce a low arm/fabric pulse—Danger faster/rougher, Transition flowing, Safe receding, Quiet silent—rate-limited to one event per transition.

Purification is authoritative only when gameplay accepts the attack: **weapon cut → fibrous/stone corruption tears → 0.20–0.35 s deliberate near-silence → Ishibashiri breath and local nature return**. 1/3 restores a small dry breath/nature aperture; 2/3 adds a wider exhale and slightly longer environmental return; 3/3 uses the same identity with its greatest dynamic release and hands directly to Calm. No explosion and no separate “weak point complete” jingle. Duplicate input and rejected purification are silent except for ordinary weapon movement.

### Calm — alive, not defeated

On 3/3, all combat music layers (if present) stop within 0.25 s; after the purification void, breathing slows into two uneven but living exhalations. Rock strain and aggressive hoof transients cease, while nostril breath, soil weight shift, wind through cedar, then sparse birds return over 3–6 s. The body remains a 3D source and may make a quiet nose/ground gesture. There is no corpse thud, death sting, loot tone, triumph cadence, silence extending like death, or audio that gates `Calm → Completed`. Retry restores encounter ambience immediately and cancels Calm tails.

## 3. BGM policy

| Section | Policy |
|---|---|
| Approach | **No BGM**; forest and distant body establish scale. |
| Encounter/Chase | Optional low, non-melodic two-layer bed may enter only after encounter commit; ship without it if it masks body cues. |
| Charge | Do not add notes per charge. Duck/freeze the bed beneath Telegraph, then release after impact/Recover. |
| Climbing | One sparse sustained layer at most; remove percussion. Height wind/body rhythm lead. |
| Kakon purification | Bed thins before the accepted cut; 1/3 and 2/3 do not restart a cue. |
| 3/3 | Stop bed; purification sequence owns the transition. |
| Calm | No “victory theme.” Natural sound and living breath are foreground. A single non-looping tonal release is optional only after survival reads clearly. |
| Ending cards | One short restrained motif may begin after Calm is established; it must not retroactively make Calm a kill victory. |

## 4. Minimum SFX asset set (15 maximum)

Events are lightweight SoundCue/parameter variants; this table requests **15 source families**, not dozens of files or MetaSound graphs.

| # | Asset/event family | Uses / reuse | Priority | Playback | Space | Need |
|---:|---|---|---|---|---|---|
| 1 | `SFX_Forest_Bed` | Approach, Rest, Calm return; intensity/bird variants | C/D | Loop | 2D bed + 3D spots | **Required** |
| 2 | `SFX_Wind_Height` | canopy, climb height, fall | C | Loop | 2D/3D zone | Required |
| 3 | `SFX_Ishi_Breath` | silhouette, Chase, Telegraph inhale, stamina mix, Calm exhale | A/C/D | Loop + one-shot regions | 3D | **Required, highly reused** |
| 4 | `SFX_Ishi_Hoof_Ground` | distant step, Chase, charge cadence, Kneel/Calm weight | A/C | OneShot variants | 3D | **Required, reused** |
| 5 | `SFX_Ishi_Rock_Creak` | Telegraph, posture, Shake, embedded Kakon layer | A/B/C | OneShot | 3D | **Required, reused** |
| 6 | `SFX_Rope_Fitting` | Grab open, climb rope, Shake warning/moment | A/C | OneShot variants | 3D | **Required, reused** |
| 7 | `SFX_Charge_Release` | charge start | A | OneShot | 3D | **Required** |
| 8 | `SFX_Charge_Impact_Pass` | near pass and impact via layers/pitch | A/C | OneShot variants | 3D | **Required, reused** |
| 9 | `SFX_Weapon_Body_Counter` | counters 1–3, posture break layer | B | OneShot parameterized | 3D | Required |
| 10 | `SFX_Grab_Climb_Contact` | success, miss, granite/bristle/cloth contacts, cling/slip | A/B/C | OneShot variants | 3D | **Required, highly reused** |
| 11 | `SFX_Player_Exertion` | climb, stamina low/danger/recovery, cling/fall | A/B | OneShot sequence | 3D/2D close | **Required** |
| 12 | `SFX_Shake_Body` | active Shake body/rope/rock bed | A/C | Loop (2 s) | 3D | **Required** |
| 13 | `SFX_Kakon_Embedded` | proximity and both Sense responses by filters/rate | B | Loop/OneShot | 3D | **Required, reused** |
| 14 | `SFX_Purify_Cut_Rupture` | accepted cut, rupture, 1/3–3/3 intensity | A/D | OneShot layered | 3D | **Required** |
| 15 | `SFX_Ending_Motif` | post-Calm ending only | D | OneShot | 2D | Optional |

The six common manifest roles (`BoundaryReading`, `ArmWarning`, `MountOpen`, `ClingWarning`, `KakonPurified`, `EncounterCalmed`) map onto families 3, 5, 6, 10, 13 and 14 rather than demanding six unrelated aesthetics. All remain `pending`; filenames, licensing, listening, loudness, UE import and spatialization are `ASSET_REQUIRED`/`NOT_RUN`.

## 5. Implementation matrix and acceptance status

| Capability | Current evidence | Status |
|---|---|---|
| Gameplay state/timing hooks | Charge/Kneel/Grab/Shake/Kakon/Calm/Sense exist in C++ | Implemented hook only |
| Creature animation/body motion | Idle/Walk/Charge/Buck/Calmed assets and source selection exist | Implemented; runtime review `NOT_RUN` |
| Actual audio playback and mix | No audio API or source media found | **`ASSET_REQUIRED`** |
| Common SFX manifest | Six named roles, all `pending`, no adopted file | Contract only |
| Spatialization, mono, masking, loudness, controller-speaker test | Requires UE/audio assets/hardware | **`NOT_RUN`** |
| “Understand without looking” human test | Requires implemented sounds and blind/eyes-away protocol | **`NOT_RUN`**, never infer PASS from hooks |

## 6. Production constraints

Use one creature component, one ambience component, simple concurrency/ducking and parameterized cue variants. Do not add dialogue, orchestral stems, dozens of one-off sounds, state-per-MetaSound graphs, or audio-driven gameplay callbacks. Gameplay emits a state edge; presentation observes it; progress remains owned by Kakon/Nushi state.
