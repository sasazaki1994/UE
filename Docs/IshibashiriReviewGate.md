# STEP 4B — Ishibashiri First Playable Review Gate

## Verdict

**UNVERIFIED — WINDOWS UE RUNTIME NOT AVAILABLE**

This review was performed on 2026-09-15 in a Linux container. It does not contain PowerShell, a Windows UE 5.6.1 installation, MSVC, a render-capable Windows RHI, or an identifiable test GPU. Consequently, no source-only result is promoted to a runtime, visual, IK, performance, or package PASS.

## Revision and environment

| Item | Recorded value |
|---|---|
| Requested upstream | `sasazaki1994/UE` latest `main` |
| Fetch result | **UNVERIFIED** — `git fetch origin main --prune` failed with `CONNECT tunnel failed, response 403` |
| Local baseline | PR #48 merge (`34303d1e693eea83226f6a0f906b1b7625352a30`) |
| Start SHA | `34303d1e693eea83226f6a0f906b1b7625352a30` |
| Source finish SHA | `34303d1e693eea83226f6a0f906b1b7625352a30` (no runtime or gameplay source was changed) |
| Review record | The commit containing this document; use `git rev-parse HEAD` after checkout |
| Host | Linux 6.18.44, x86_64 |
| Required runtime | Windows + Unreal Engine 5.6.1 |
| Available runtime | None |
| GPU | **UNAVAILABLE / NOT IDENTIFIED**; no value is inferred |

The requested documents and launcher were present and reviewed: `IshibashiriApproachSlice.md`, `IshibashiriProductionVisualSlice.md`, `ClimbingIKValidation.md`, `CampaignE2EValidation.md`, `VisualQualityUpgrade.md`, `3DProductionPolicy.md`, and `Tools/Prototype.ps1`. The acceptance contract added for this gate explicitly prevents a Linux/source-only run from being recorded as PASS.

## Gate results

| Gate | Result | Evidence / limitation |
|---|---|---|
| Windows UE 5.6.1 Build | **NOT_RUN** | `pwsh`/Windows UE/MSVC are absent; Compile and UHT status are unknown |
| Approach 60 FPS | **NOT_RUN** | No Windows UE runtime |
| Approach 30 FPS | **NOT_RUN** | No Windows UE runtime |
| Player spawn and input traversal | **NOT_RUN** | No position mutation was used to claim success |
| Boundary Stone / footprints / damage trail | **NOT_RUN** | No rendered inspection |
| Reveal once / approximately 3 s / no combat | **NOT_RUN** | Source documentation is not runtime proof |
| Approach duration | **NOT_MEASURED** | No playthrough; the documented estimate is not reused as a result |
| Basin gate / Approach to Encounter | **NOT_RUN** | Transition state and collision are unknown |
| Ishibashiri 60 FPS | **NOT_RUN** | Ground through Victory was not played |
| Ishibashiri 30 FPS | **NOT_RUN** | No Windows UE runtime |
| Gamepad | **NOT_RUN** | No runtime device/input execution |
| Retry regression | **NOT_RUN** | Approach replay and Encounter reset were not observed |
| Sense regression | **NOT_RUN** | held state, old target, warning, recovery tail, and old world pointer were not observed |
| Legacy visual gate | **NOT_RUN** | No rendered capture |
| HighQuality visual gate | **NOT_RUN** | D3D12/SM6/Lumen/VSM were not initialized or verified in logs |
| Performance | **NOT_MEASURED** | No FPS, unit, GPU, or RHI-memory values are guessed |
| Campaign 60 / 30 / gamepad | **NOT_RUN** | Opening sequence and Ending are unverified |
| Package | **NOT_RUN** | Correctly skipped because the major gates did not pass |

## Climbing IK

`/Game/Characters/Rigged/Shirotsura/CR_Shirotsura_Climbing` is absent from this checkout. The runtime therefore remains the documented safe no-op. A Control Rig `.uasset` cannot be authored and saved without the UE Editor, so this review did not fabricate a binary asset or add another source-only generation hook.

The requested node 0 to node 3 scope remains unchanged. Grab, front leg, left/right hand, left/right foot, shoulder rest, boss walking/turning, buck warning, shake, cling, and retry are all **NOT_RUN**. IK-off/IK-on images and Bone-to-Target distances are **NOT_CAPTURED / NOT_MEASURED**. The 8 cm good and 20 cm acceptable thresholds therefore have no result.

## Visual and capture review

No screenshots were created. Required paths are consequently **NONE** rather than placeholders. The Approach set (entrance through Encounter opening), Legacy set, 18-image HighQuality set, and seven matched IK before/after views all remain outstanding. Covered Kakon should be recorded as `NOT_APPLICABLE / presentation inspection only` if it remains unreachable through normal gameplay.

Without rendered evidence, this review makes no claims about Shirotsura tonal separation, weapon/arm emission, Ishibashiri's boar/mountain silhouette, Kakon readability, Basin composition, fog, exposure, camera clipping, primitive appearance, or player/environment scale.

## Commands and evidence policy

The mandatory first command is not executable on this host:

```powershell
.\Tools\Prototype.ps1 -Action Build
```

The same limitation blocks Approach 60/30, Legacy, HighQuality, Campaign 60/30/gamepad, and Package commands. Package was not attempted because section 22 permits it only after the principal gates pass. Repository source-contract tests may still be run as a consistency check, but their outcome does not alter this verdict.

## Fixed bugs and scope control

- Runtime-reproduced bugs fixed: **none**. Runtime reproduction was impossible.
- Gameplay/source hooks added: **none**.
- Bosses, gameplay systems, Sense types, chapters, PCG, Niagara, Motion Matching, and climbing scope added: **none**.
- Documentation gap fixed: the gate now has an explicit Gherkin-style acceptance contract and a durable result matrix that distinguishes `NOT_RUN` from failure and forbids source-only PASS.

## Remaining issues and production decision

All decisive STEP 4B evidence remains outstanding: fresh upstream confirmation; Windows build/UHT; normal-input Approach and Encounter runs; reveal timing; Retry and Sense cleanup; actual Control Rig creation; matched IK comparison; Legacy and HighQuality captures; RHI feature logs; performance telemetry; Campaign E2E; and package smoke.

**Do not proceed to Production Asset replacement or other-boss visual production.** Run this checklist from the recorded local baseline (or rebase it onto a newer fetched `main`) on Windows UE 5.6.1, attach logs and captures, and only then choose PASS or PARTIAL.
