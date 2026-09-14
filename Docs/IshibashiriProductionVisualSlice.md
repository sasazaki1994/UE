# 石走り Production Visual Vertical Slice — STEP 4A

Status: **SOURCE IMPLEMENTED / VISUAL VALIDATION NOT RUN**
Baseline: `0f712b5e1203accfee91fee54b74b79675916b83`
Scope: Ishibashiri only. Fuchimatoi, Minedaki and Magatsune presentation was not propagated.

## Asset audit and protection decision

The runtime references were traced from the constructors, rather than inferred from files in `Art/`.

| Role | Runtime reference | supporting assets | status |
|---|---|---|---|
| Shirotsura mesh | `/Game/Characters/Rigged/Shirotsura/SK_Shirotsura` | same-folder skeleton, 10 `AN_Shirotsura_*` clips, 14 `M_Baked_*` slots | retained baseline |
| Ishibashiri mesh | `/Game/Characters/Rigged/Ishibashiri/SK_Ishibashiri` | same-folder skeleton, 5 `AN_Ishibashiri_*` clips, 15 `M_Baked_*` slots | retained baseline |
| maps | BaseColor, Normal and Roughness 2048px atlases for each character | `/Game/Characters/Rigged/<name>/T_<name>_*` | present in runtime folder |
| Kakon | three `AKakonActor` child actors at mesh-local `Cores[]` | current core only is presented normally | coordinates unchanged |
| climbing | `Route[]` and `Neighbors` in `IshibashiriBoss.cpp` | 11 mesh-local points | unchanged |

The reference Shirotsura documented in `ShirotsuraReferenceCheckpoint.md` is a static, incomplete candidate and was **not** substituted. No character binary, skeleton, animation, atlas, or material asset was overwritten. The imported rigged production assets remain the baseline. The repository metadata reports 38,536 Shirotsura vertices and 103,975 Ishibashiri vertices; static model metadata reports 32,194 and 205,442 triangles respectively. These records are not a fresh UE triangle measurement.

## Presentation implementation

### Shirotsura

The existing rig, geometry, clips, and material slots remain intact. Two collision-free, non-shadow-casting presentation proxies follow the rig's existing `weapon` and `hand_L` bones. The narrow desaturated blade response reinforces the readable sword silhouette; the compact red-black arm response reinforces the abnormal left arm. They do not perform traces, select targets, modify stamina, or own encounter state. The existing whitewood mask, charcoal/indigo clothing, cord, straw, bell/fittings, corruption and blade material separation remains the authored base; no unvalidated static reference model was imported.

### Ishibashiri material/readability

The retained runtime mesh already separates umber hide, charcoal bristles, four granite slots, moss, lichen, straw, blighted root, crimson fracture, red-black mineral, nostril shadow, and paper offerings. Existing baked materials use BaseColor/Normal/Roughness atlases and roughness by semantic slot. This change does not claim newly baked pixels: runtime visual validation is required. Presentation code replaces the normal-play full-red core marker with a smaller red-black pulse for only the currently reachable core. `-DebugGuidance` retains the old bright red authoring marker.

### Kakon states

The common lifecycle is untouched. `Covered` remains the shared actor's shell state; Ishibashiri intentionally configures its three route cores with zero shell health, so its normal encounter begins each current core as `Exposed`. `Exposed` is a restrained pulse blended into the local corruption. `Purified` hides the marker and the corresponding imported `core_N` bone. The next route core becomes current. Consequently all three lifecycle appearances are specified, but a **Covered Ishibashiri capture is NOT_RUN/not reachable without a presentation-only inspection setup**; gameplay was not changed merely to produce it.

### Boundary and Corruption Sense

Boundary Sense is direction feedback on 境断ち: response amplitude uses the existing normalized target/facing strength, with a slow surface pulse and no laser. Corruption Sense is state feedback on the left arm: Danger is fast/strong, Transition flows slowly, Safe recedes, and Quiet stays subdued. Both are downstream readers of `UPlayerSenseComponent`. The division remains sword = direction and left arm = danger/state.

### Purify and Calm

Existing Attack/Purify timing and Slash selection are retained. The sequence is input -> Slash clip -> existing `TryPurify` -> Kakon state broadcast -> local core marker and core bone extinguish. No VFX owns progress. This is a minimal source presentation hook, not a claim of final Niagara quality.

At 3/3, the existing Nushi Calm lifecycle selects `AN_Ishibashiri_Calmed`; there is no death, explosion, disappearance, or destructive geometry event. The Basin hook slightly lowers fog density/opacity and lifts key intensity. Retry restores the encounter atmosphere and Kakon state.

## Basin, lighting, camera, and UI

The existing 80m Basin boundary and gameplay coordinates remain. Six cheap old-cedar silhouettes were added outside the central route as scale references, alongside existing rocks, cliffs, ritual path and stakes. They are non-colliding and cannot block Grab/climbing. The palette is restrained wet soil, charcoal rock, moss green and aged timber.

Basin lighting remains one movable shadow-casting directional key plus one non-shadow point fill. SkyAtmosphere and volumetric ExponentialHeightFog remain. HighQuality continues to request DX12/SM6, Lumen GI/reflections, VSM, volumetric fog, bloom, and auto exposure through the explicit launcher switch; Legacy stays DX11/SM5 and remains default. No lighting result is claimed without UE.

The camera contract, FOV, distances, occlusion escape, route camera, and controls were not changed. A reveal cinematic was deliberately not added without runtime framing validation. Normal HUD now prioritizes Stamina, Kakon progress, active Sense and contextual feedback/Victory. Boss HP, internal state/time and broad control telemetry are behind `-DebugGuidance`; climbing's contextual instruction remains while attached.

## Audio hooks

No audio files were fabricated. Existing transition sites are the future hook points: `EnterState(Charge)`, `BeginGrab`/climbing attach, buck warning/buck checks, `Attack`/`TryPurifyCore`, `HandleCorePurified`, `HandleNushiStateChanged(Calm)`, and Boundary/Corruption begin/end. Audio must remain an observer of those states.

## Performance budget

| item | budget / source accounting |
|---|---|
| character triangles | recorded static metadata: Shirotsura 32,194; Ishibashiri 205,442; runtime skeletal triangles NOT_RUN |
| material slots | runtime asset folders: Shirotsura 14; Ishibashiri 15 |
| textures | three 2048px maps per character (BaseColor, Normal, Roughness) |
| added geometry | 2 tiny player proxies; 6 trees, each 1 trunk + 1 crown; no character duplication |
| Niagara | 0 systems / 0 emitters added |
| dynamic lights | Basin: 2 (1 key casting shadows, 1 fill without shadows) |
| principal shadow casters | key sun, characters, floor/rocks/trees; sense proxies explicitly do not cast |
| HighQuality | DX12, SM6, Lumen GI/reflections, VSM, volumetric fog, bloom, auto exposure requested |

GPU/frame costs and actual runtime mesh LOD triangle counts are **NOT_RUN**.

## Validation and captures

Python source-contract tests and JSON parsing are local checks, not substitutes for UE visual assessment. This Linux container has neither Windows UE 5.6.1/MSVC nor Blender; UHT, Editor Development build, Automation, gameplay at fixed 60/30, Legacy/HighQuality render checks, FPS/GPU profiling, and screenshots are **NOT_RUN**. See `IshibashiriVisualValidation.json` for machine-readable status.

Required HighQuality captures remain NOT_RUN: Encounter opening, Shirotsura close, Ishibashiri full body, scale comparison, Charge, Ground Grab, Climbing, Boundary Sense, Corruption Sense, Kakon Covered, Kakon Exposed, Purify, Kakon Purified, Buck/Cling, Kakon 3/3, Calm, Victory, and Basin wide shot. Legacy before/after is also NOT_RUN.

## Known issues / three principal visual weaknesses

1. The source-only sense proxies use Engine primitives and the current basic material; authored translucent/emissive character material instances and close-up tuning still require UE visual work.
2. Character atlases and their PBR slot separation were not rebaked or visually inspected here; wetness, moss/rock blending, silhouette intersections, and exposure may not meet final quality yet.
3. Basin trees/rocks are low-cost primitives and there are no authored vegetation, decals, wind, local mist Niagara, or production captures. Their composition and route clearance need in-engine review.

## Baseline decision

Do **not** propagate this baseline to Fuchimatoi, Minedaki, or Magatsune yet. Source hooks and acceptance coverage are ready for the Ishibashiri review gate, but the required HighQuality visual captures, UE build, gameplay regression, and performance measurement must pass first.

**STEP 4A BLOCKED — SOURCE IMPLEMENTED / VISUAL VALIDATION NOT RUN**
