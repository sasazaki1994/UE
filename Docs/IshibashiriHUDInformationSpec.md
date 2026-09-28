# 石走り編 HUD / Information Final Specification

Status: **DESIGN LOCK; implementation delta listed below**  
Baseline and evidence labels are defined in [IshibashiriFinalGameplaySpec](IshibashiriFinalGameplaySpec.md).

## 1. Information hierarchy

Production answers only the next player question: “am I in danger?”, “can I act now?”, “where is the corruption when I choose to Sense?”, and “can I survive the next crossing?” The body, left arm, boundary blade, light, future audio, camera and animation speak first; text confirms rather than replaces them. Boss HP never appears because Ishibashiri is not killed.

The existing Canvas HUD currently always draws a large title panel, `STAMINA n/100`, `KAKON n/3`, Campaign Sense controls, and transient feedback; it draws attack direction, climbing instructions, and a Victory/Defeat box. `-DebugGuidance` additionally exposes Player HP, posture, state/time and broad controls. This is **SOURCE**, not the final Production layout.

## 2. Final classification

| Information | Final class | Player-facing contract and current delta |
|---|---|---|
| Player HP | **状況によって表示** | Hidden at full health; three non-numeric wound/petal pips fade in for 3 s after damage and remain when one hit from Defeat. Current numeric HP is Debug-only, so a compact production indicator is a small delta. |
| Stamina | **状況によって表示** | Hidden on ground/full safe rest. Fade in at Grab, while climbing below 90, during drain/recovery, warning/shake, and for 1.5 s after reaching full. Bar/pips, no `n/100`. Current permanent numeric row must change. |
| Kakon remaining/progress | **状況によって表示** | No permanent counter. Show `禍根 1/3`, `2/3`, `3/3` for 1.5 s after purification; the ending confirms completion. Current permanent `KAKON n/3` must change. |
| Boss HP | **完全削除** | Never in Production or planned Debug; there is no boss-health authority. |
| Ishibashiri State | **Debug限定** | State and seconds remain under `-DebugGuidance`. |
| Posture | **Debug限定** | Numeric posture remains Debug. Ground bulges/stagger and first-cycle prompt carry Production meaning. |
| Charge warning | **状況によって表示** | Body tell/camera/ground response always. No normal `DANGER` word; an icon/edge pulse may appear during the 1.0 s telegraph. Accessibility can enable `危険` text. |
| Recover possible | **状況によって表示** | First successful evade and until the first valid counter: short `今だ：攻撃` prompt plus vulnerable body/left-arm response. Later cycles use presentation only unless button hints are set Always. |
| Grab possible | **状況によって表示** | During Kneel and within 4 m: gold hold glint; within activation range show `E / RB 取り付く` according to button-hint preference. Never visible during normal/Charge. |
| Cling warning | **状況によって表示** | 2 s pre-shake icon + stamina emphasis; first occurrence shows `E / RB 長押し`. Accessibility may keep `揺れ：しがみつく` text. |
| Rest point | **Sense中だけ表示** | Normally communicated by stable ledge/body motion and stamina recovery. Corrupted Arm Sense may softly reveal the next reachable safe ledge; no `SAFE` word in default mode. |
| Route number | **Debug限定** | Never Production. |
| Kakon marker | **Sense中だけ表示** | Only current reachable, unpurified target; a restrained world response, not a screen-space red pin. Brief context highlight at interaction range is allowed. Current always-current marker must be gated. |
| `KAKON 1/3` | **完全削除** as permanent English row | Replaced by transient localized `禍根 1/3`; Debug may log/count. |
| `DANGER` / `SAFE` / `TRANSITION` | **Sense中だけ表示** | Default favors arm intensity/pattern and icon. These exact words are allowed when danger-text accessibility is ON; remain exact strings in Debug/test telemetry. |
| `WEAK / MEDIUM / STRONG` | **Sense中だけ表示** | Default is three-step blade pulse/haptic pattern; optional Sense-direction aid may add coarse wedge and localized strength. Exact labels remain for text accessibility and Debug. |
| Operation buttons | **初回のみ表示** by default | Movement/camera, Sense, Dodge, Attack, Grab, Cling, Purify appear once at their safe/relevant context. Preference supports Off / Contextual(default) / Always. Never list all controls throughout combat. |
| Retry | **状況によって表示** | Defeat and optional pause/help; not always visible after success or during ordinary play. R/Y remains functional where current code permits it. |
| Victory / Calm | `Victory`: **完全削除**; Calm: **状況によって表示** | Use the living calm pose, environment and `石走りは鎮まった` / `Encounter Completed`; no green Victory box. Internal enum/log/test names may remain. |
| Sense direction | **Sense中だけ表示** | Boundary blade points by response; accessibility option adds a broad edge wedge, never exact distance/path. Current text reports strength only, so directional aid is a future small presentation delta. |
| Attack direction arrow | **状況によって表示** | Retain on ground while aiming/attacking only; hide in Approach after teaching, during Grab/climb/Calm. Its fixed screen length means direction, not range. |
| Gold Grab marker | **状況によって表示** | Retain only during Kneel, reduce collectible-like sphere appearance when final assets replace the primitive. |

## 3. Three canonical screen states

### Normal play (`-DebugGuidance` absent, no Sense hold)

- **Ground:** normally no panel. Contextual charge edge cue, first-use Dodge/Recover/Grab prompt, recent-damage HP and attack direction may appear.
- **Climbing:** stamina bar appears only when relevant; shake icon/prompt, interaction prompt and the 1.5 s purification count are transient. No state, posture, route, boss HP or permanent objective block.
- **Calm:** all combat gauges and warnings fade within 0.5 s. The living body and camera carry the result; a restrained Calm/Encounter Completed line may bridge into the ending cards.

### Sense held

- Boundary Sense: blade response + current reachable corruption/world response; optional coarse direction wedge. No exact metres, route number or future Kakon.
- Corrupted Arm: arm pulse pattern for danger/transition/safe and a soft safe-ledge response. Default contains no large status panel. Accessibility danger text may render one localized word.
- Sense ending fades its overlays in 0.20 s. The existing 50% stamina-recovery risk while Arm Sense is held and for 2 s afterward must be integrated into Ishibashiri before being used as a Production balancing assumption; current Ishibashiri legacy stamina does **not** implement it.

### `-DebugGuidance`

Draw normal information plus a clearly labelled `DEBUG` panel containing Player HP, posture, Ishibashiri state/time, route node/destination, stamina numeric/rates, shake phase, Sense labels/strength, camera mode/collision fallback, purified count and broad controls. World arrows, route markers and Kakon primitives may persist. Debug never changes eligibility, timing, progress or collision.

## 4. Prompt sequence

| Beat | Trigger | Default prompt | Exit / repeat |
|---|---|---|---|
| Approach | first control | move + camera | input received; never repeats in the session |
| Boundary | before stone | Q/LT 境断ち, F/LB 穢れた左腕 | either used or 6 s; optional, does not block |
| Final rumble | safe approach segment | Dodge then Attack buttons as two short cards | relevant input received |
| First Telegraph | 0.2 s into tell | Dodge button only | dodge/charge end |
| First Recover | successful evade | Attack button, `今だ` | valid counter/Recover end |
| Kneel | within 4 m | Grab button | mount/window end |
| First shake warning | warning start | Hold Cling | shake end; subsequent icon only |
| First Kakon range | safe ledge | Purify button | press/leave range |
| First Rest | stamina <75 at safe ledge | `ここでは回復する` | 2 s or stamina rise recognized |

Prompts queue rather than overlap, use the active input device, stay inside safe areas, and never pause gameplay. The Japanese font/glyph path must be runtime-validated; until then English fallback is evidence of a limitation, not approval to ship mixed debug prose.

## 5. Accessibility-ready seams (no Settings screen required now)

Expose a small presentation settings struct/config interface; do not build a framework. Required future toggles:

- **Button prompts:** Off / Contextual / Always.
- **Sense direction aid:** blade only / coarse screen-edge wedge / high-contrast wedge.
- **Danger warning text:** Off / icon / localized text.
- **Shake warning:** visual only / visual + text / visual + controller vibration (when implemented).
- **Subtitles:** Off / On; speaker-independent environmental captions, safe-area and background opacity support.
- **UI Scale:** 75–150%, independent of resolution auto-scale; default 100%.

Color is never the sole channel. Warning, safe, Grab and Kakon differ by shape/pulse/text as configured. Reduced-motion camera settings belong to the camera spec. Save these preferences separately from Demo campaign persistence when implemented.

## 6. Minimal implementation and validation

Required changes are limited to the current Canvas HUD and presentation gates: remove the permanent title/status panel in normal play; add contextual fade/state bookkeeping; gate Kakon markers; replace Victory copy; add prompt preferences as data seams; enrich Debug. No new UI framework is authorized.

First-play checks: identify the charge without accessibility text; find Recover action by cycle 2; mount on first eligible press; notice warning before shake; state whether stamina is safe before leaving a rest; find each Kakon with/without Sense; understand 1/3→3/3; understand Calm rather than death. Also test 720p, 1080p, 1440p, ultrawide safe margins, UI 75/100/150%, keyboard and physical gamepad. Current-SHA UE readability remains **NOT_RUN**.
