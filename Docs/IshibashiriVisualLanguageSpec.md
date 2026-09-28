# 石走り編 Visual Language Production Specification

Status: **DESIGN LOCK / final Niagara and material assets `ASSET_REQUIRED` / UE visual validation `NOT_RUN`**  
Baseline: evidenced checkout SHA `27a4e6f91db5a77f1a897865927a930af6b5147d`.

This specification follows the locked gameplay, HUD and camera specifications. Color is consistent but is never the sole carrier: shape, location, rhythm, body motion, sound and optional accessibility symbols duplicate every critical state.

## 1. Color language

| Meaning | Palette / value rule | Never means |
|---|---|---|
| **Corruption / 禍** | charcoal-black and ink, with dark oxblood seams; danger reveals narrow deep-crimson cracks from inside darkness. Saturated red is momentary Level 2/3 only. | route, ordinary damage flash, Grab, safe rest |
| **Purification** | warm off-white core with pale blue-green/water-blue edge, low saturation and short duration. | healing spell, permanent objective beacon |
| **Interact / Grab** | restrained aged gold/kinari, rope fiber and moss highlight; stable rather than pulsing red. | loot/rare item, purification |
| **Rest / safe foothold** | low warm amber on moss/wood, broad and still; nearby corruption emissive recedes. | progress complete |
| **Boundary Sense** | desaturated pale cyan/white-green limited to blade, reachable Kakon seam and a short route response. | whole-world vision mode or exact GPS line |
| **Corruption Sense** | left-arm ink/crimson response plus a thin peripheral vignette: Quiet charcoal, Safe muted blue-gray, Transition oxblood slow flow, Danger deep-crimson broken pulse. | full-screen red damage state |
| **Calm / living nature** | corruption red extinguishes; neutral wet stone, moss, cedar warmth and pale sky recover gradually. | death-white fade, victory gold burst |

Contrast and rhythm are canonical, not exact monitor RGB. Production materials must expose shared `CorruptionAmount`, `CueStrength`, `SenseStrength` and `PurifyAmount` parameters where practical; do not create a full material per boss state.

## 2. State cue table

Times refer to presentation only and never gate gameplay.

| State | Color / luminance | Rhythm | Origin | Duration | Gameplay meaning and redundant channel |
|---|---|---|---|---|---|
| Charge warning | black soil displacement + deep-red cracks, +15–25% local contrast | one accelerating 1.00 s gather | forehooves/chest and contact soil | Telegraph only | “lateral dodge soon”; weight shift, bodily sound, camera framing, optional edge icon |
| Recover | neutral stone/moss, corruption contracts; slightly brighter counter limb | one descending settle, no flash | foreleg/impact side | 1.65 s | “counter now”; lowered body and recover exhale |
| Kneel | neutral dust/soil, no red victory burst | heavy downward settle | forequarters/knees | state duration | posture broken; body silhouette/audio |
| Grab possible | aged gold/kinari rim, medium value | slow 1.2 Hz breathe, stable enough to aim | valid foreleg and horn/head volumes | up to 5 s | “E/RB can mount”; contextual icon/audio once |
| Grab success | kinari compresses to contact then disappears | single 0.25–0.48 s contraction | hand/contact hold | approach only | committed mount; tactile sound/body warp |
| Shake warning | corruption-dark body tension with two localized deep-red pulses; rope highlight catches near action moment | 0.7 Hz first second, 1.8 Hz final second | shoulder/back under rider + rope/fitting | 2.0 s | prepare, then Cling; pose/audio/controller pulse/optional icon |
| Cling | off-white/kinari tight contact outline, not green | held/stable during input; 0.2 s cinch on success | hands/current hold | Shake window | held = braced; grip pose and contact sound |
| Stamina danger | desaturated off-white bar shape with dark-red notch; never screen wash | two uneven pulses per 1.5 s max | player hands + compact HUD edge | while below danger threshold | rest/cling soon; exertion/audio and bar shape |
| Rest | warm amber/moss, low output; corruption dims locally | almost still, <=0.35 Hz | authored safe ledge/contact patch | while resting, fade 0.5 s | safe recovery; breath, stopped drain and HUD recovery |
| Kakon distant | no normal-play beacon; Sense-only charcoal seam with faint cyan blade response | <=0.5 Hz | reachable Kakon/short route segment | Sense hold only | coarse direction, not distance arrow |
| Kakon near | black mass with narrow oxblood/crimson internal seam; pale Sense edge if held | irregular 1.0–1.6 Hz | embedded core only | while near/current | corrupted target is close; 3D embedded sound/body geometry |
| purification | deep red collapses inward, then off-white/pale-blue-green slit | cut, 0.20–0.35 s visual stillness, short outward release | struck Kakon only | <=0.35 s tail | accepted purification; cut/rupture/silence/audio |
| purified | emission off; natural host material remains, no glowing empty socket | none | former Kakon | persistent | this Kakon is clean; transient `n/3` and next Sense target |
| 1/3 | Level 2 local pale release | single small pulse | current core/body patch | <=0.35 s + HUD 1.5 s | progress only |
| 2/3 | same identity, 1.25x area not saturation | single medium pulse | current patch + nearest seam | <=0.45 s + HUD 1.5 s | progress only |
| 3/3 | Level 3 pale release, corruption recedes across body; no explosion | one broad 0.8–1.2 s release then stillness | final core → body | <=1.2 s | all purified; living Calm motion/audio |
| Calm | no corruption emission; soft neutral key and environment return | 3–6 s gradual settle, no flashing | whole creature and Basin atmosphere | encounter tail | “alive and settled”; Calmed pose, slow breathing, nature audio, no Victory word |
| Sense Boundary | pale desaturated cyan/white-green | strength changes amplitude, slow 0.8 Hz max | blade + reachable seam + next usable route only | hold + <=0.15 s fade | direction/strength; blade orientation and optional pattern |
| Sense Corruption | ink/oxblood/deep crimson according to fixed categories | Quiet none, Safe slow recede, Transition 0.6 Hz flow, Danger broken 2 Hz | left arm, thin periphery, relevant danger/optional safe point | hold + <=0.2 s fade | state/danger; arm pose, category-change audio and optional symbol |

## 3. VFX intensity budget

* **Level 1 — environment/readability:** subtle breath condensation, dust, tiny seam pulse, moss/rope value and sparse particles. At most three local emitters in the camera frustum. No full-screen overlay.
* **Level 2 — gameplay warning:** Charge, Shake, stamina danger and active Sense. One primary local cue plus at most one thin screen/HUD edge cue; <=25% screen area, <=2 s except a held Sense. Critical cues pre-empt decorative particles.
* **Level 3 — singular transition:** accepted purification, 3/3 and Calm transition only. One event at a time, <=1.2 s peak, then a low-motion settling phase. Level 3 cannot loop or run for every charge/shake.

Photosensitivity default: no full-field flash, no alternating high-contrast pulse above 3 Hz, and no repeated Level 3. Reduce-effects mode removes particles/camera-edge treatment but retains high-contrast geometry/shape cues.

## 4. Minimum VFX set (6 reusable systems)

| Asset/system | Parameters and reused states | Need |
|---|---|---|
| `VFX_CorruptionResponse` | location, radius, `CueStrength`, pulse rate; Charge soil, Shake body, Kakon far/near, stamina hand slip | **Required**; one system, not state copies |
| `VFX_SenseResponse` | Boundary/Corruption mode, strength/category; blade, arm, seam, route/safe point | **Required** |
| `VFX_InteractionCue` | kinari/off-white palette, contact geometry; Grab available/success, Cling, Rest | **Required** |
| `VFX_BodyContact` | dust/debris amount; hoof, impact, Kneel, climb/fall | Required |
| `VFX_KakonPurification` | progress index and radius; 1/3, 2/3, 3/3 | **Required** |
| `VFX_CalmTransition` | corruption fade, fog/key-light and nature-particle return | **Required** |

Simple mesh/material parameter animation is preferred where it reads better; “VFX” does not imply six Niagara assets. Current primitive pulses, dynamic lights and 0.35 s contraction are **prototype placeholders**, reusable as timing/reference hooks but not final Niagara proof.

## 5. Current implementation versus target

| Present now | Evidence classification | Production decision |
|---|---|---|
| Imported Ishibashiri semantic material slots and Idle/Walk/Charge/Buck/Calmed clips | Actual assets; runtime look `NOT_RUN` | Preserve and parameterize, no mandatory model change. |
| Red-black current-Kakon primitive pulse and 0.35 s purification contraction | Code-driven placeholder mesh/material | Retain hook/timing; replace or dress with shared corruption/purification system. |
| Gold foreleg Grab sphere | Prototype marker | Replace with kinari contact/rope highlight and add head/horn volume cue; do not ship a floating sphere. |
| Blade/left-arm Sense proxy primitives | Code-driven local presentation | Valid functional fallback; final response stays localized. |
| Boss state color/light, Basin atmosphere/light response and simple presentation lights | Prototype Light/material presentation | Tune as Level 1/2; a Light is not proof of final VFX. |
| HUD strings, marker spheres, route geometry, arrows and state/time | Debug or current placeholder | Classify below; never count as completed art. |
| Niagara systems / authored final particles | None evidenced | **`ASSET_REQUIRED`**. |

## 6. Debug, Production and Accessibility separation

| Existing guidance | Production | Accessibility | `-DebugGuidance` |
|---|---|---|---|
| charge directional arrow / attack direction debug | Remove charge path; attack aim may remain only where weapon targeting requires it | high-contrast dodge edge, not world arrow | retain authoring arrow/state trace |
| route lines, route nodes/numbers | Only a short usable segment while Boundary Sense is held; no number | optional thicker patterned segment | full route/nodes retained |
| Kakon marker sphere | Remove; use embedded corruption and Sense response | optional shape glyph at near range/current phase | bright markers retained |
| floating gold Grab sphere | Replace with body-contact highlight | contextual E/RB icon and high-contrast outline | volume/marker visualization retained |
| `SHAKE INCOMING`, `SHAKING`, state/time, posture | Remove from standard | concise `しがみつく`, danger icon/subtitle and configurable persistent timing ring | retain exact state/time/posture |
| permanent numeric stamina/Kakon | contextual nonnumeric stamina and 1.5 s `n/3` only | optional numeric/persistent bars and progress | retain telemetry |
| `Victory`/`Defeat` developer box | Never show Victory for Calm; Retry/ending copy follows HUD spec | readable Calm/ending captions | internal result may be logged, not treated as art |
| circles/arrows/markers used by tests | no production dependency | optional semantic icons, never raw debug primitives | retain for automation/diagnosis |

Accessibility offers independently configurable high-contrast outlines, pattern/shape identifiers, critical-event captions, stereo-to-mono compatibility, reduced effects, camera-shake reduction, controller pulse, and persistent contextual UI. It must not be implemented merely by enabling `-DebugGuidance`; Debug may expose hidden state and exact paths, accessibility may only restate information legitimately available to the player.

## 7. Multichannel critical contract

| Event | Body/world | Audio | Light/VFX | UI/haptic fallback |
|---|---|---|---|---|
| Charge | weight shift/locked launch | bodily telegraph/start/near pass | hoof soil + local cracks | edge icon/caption/pulse optional |
| Shake | breath/brace/Buck pose | warning + precise Cling catch | shoulder/rope two-stage cue | E/RB prompt + controller pulse |
| Grab | Kneel/valid hold silhouette | mount-open fitting + tactile success | kinari hold/contact contraction | contextual input icon |
| Stamina danger | strained hands/motion | patterned exertion/slip | hand cue + shaped bar notch | high-contrast/pattern bar |
| Kakon | embedded geometry | pressure texture/proximity | black mass/crimson seam + Sense | optional near glyph/category caption |
| Calm | living Calmed pose/breath | nature and slow nostril breath | corruption off, natural light returns | ending text only after Calm reads |

No row may pass on only one channel. Color-blind, audio-off and vibration-off runs are separate acceptance passes.

## 8. Validation status

Source hooks and placeholders can be checked statically. Final assets, screen occupancy, luminance, color-blind differentiation, photosensitivity, masking, fixed-60/30 performance, Legacy/HighQuality capture and first-player comprehension remain **`ASSET_REQUIRED` or `NOT_RUN`**. Until those runs exist, the claims “DebugGuidanceなしで攻略できる,” “Senseで方向が分かる,” and “Calmが死亡に見えない” are acceptance targets, not PASS results.
