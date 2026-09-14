# Player Sense Gameplay — STEP 3A

Status: source implementation complete / UE runtime validation pending
Baseline: `ca4d763eb72c5571e22c64cda7c5cefa90e5b27e`

## Roles

The two holds intentionally answer different questions. **Boundary Sense / 境断ち** answers “where is the corruption?” by reading explicitly registered, currently reachable Kakon and by softly revealing only phase-enabled route markers. **Corruption Sense / 穢れた左腕** answers “what is about to happen?” through a small encounter adapter reading existing boss state. Neither damages a Nushi, changes attack power, purifies a Kakon, or owns progress.

## Input

| Sense | Keyboard | Xbox-style gamepad | Mode |
|---|---|---|---|
| Boundary Sense | Q | LT | Hold |
| Corruption Sense | F | LB | Hold |

Grab/Cling, Attack/Purify, Jump, Dodge, Retry and camera mappings are unchanged.

## Boundary target selection and output

`UPlayerSenseComponent` is player-owned. Each encounter registers its three existing `AKakonActor` instances once during player/boss configuration. The player refreshes only those three availability bits as Kakon progress changes; there is no per-frame world actor query. Purified and future-phase targets are excluded.

While held, the component selects the best available target using distance attenuation multiplied by facing alignment. It exposes a normalized world direction, strength 0–1, and Near (<=700 cm), Medium (<=1800 cm), or Far band. The HUD deliberately renders only `BOUNDARY SENSE: WEAK / MEDIUM / STRONG`, not an exact objective arrow.

## Boundary reveal

Fuchimatoi route anchors and the Minedaki/Magatsune route primitives are normally hidden. Holding Boundary Sense reveals only the route nodes already enabled by the encounter phase. Future routes remain hidden. Ishibashiri and encounter Kakon marker visibility likewise moves behind the sense/debug gate where connected; no “go to node N” text is added by the new HUD.

## Corruption encounter adapters

The adapter is deliberately read-only and based on existing states:

- Ishibashiri: buck warning/Shake, charge telegraph/Charge = Danger; recovery = Safe.
- Fuchimatoi: BiteWindup/BiteLunge = Danger; Coiling = Transition; Snagged recovery window = Safe.
- Minedaki: Shaking = Danger; ArmBridgeTransition and FinalTransition = Transition.
- Magatsune: Large Pulse = Danger; RootRockRoute and FinalRise = Transition; configured safe route nodes = Safe.

The HUD shows only `CORRUPTION SENSE: DANGER / SAFE / TRANSITION / QUIET`. The active hold continuously refreshes this coarse category from existing encounter state; authored lead-time curves and VFX/audio are intentionally deferred.

## Risk and stamina

The sole risk is a recovery multiplier. Recovery is 50% while Corruption Sense is held and for 2 seconds after release, then returns to 100%. Fuchimatoi, Minedaki and Magatsune multiply their existing restore calls at the player call site, leaving `UStaminaComponent`'s public contract, maximum stamina, health, consumption, and delegates unchanged. Ishibashiri still uses its older route-climbing scalar stamina rather than `UStaminaComponent`; integrating that legacy recovery call is listed as unverified/deferred rather than changing the shared component contract in this slice.

## Debug guidance compatibility

Normal play uses the sense gate. Launch with `-DebugGuidance` to retain persistent route/Kakon primitives needed by visual debugging and older automation. Existing Grab Marker, recovery beacon, cling warning, and gameplay eligibility checks are not deleted. `DebugGuidance` changes presentation only.

## Retry and telemetry

Every player calls `ResetSense()` from its encounter reset. It clears both holds, warning, risk tail, elapsed durations and the cached reading while retaining the small registered target list for the restarted encounter.

Debug log events use `PLAYER_SENSE`: `sword_start`, `sword_end duration`, `strong_detection`, `arm_start`, `arm_end duration/risk_tail`, `danger_detected`, `purify_after_sense`, and `reset`. Existing Kakon telemetry remains the authoritative source for purification; the sense event records only that observation preceded it. A dedicated analytics backend is out of scope.

## Automation

`PlayerSenseComponentTests.cpp` covers front/right/back direction strength, phase filtering, warning pass-through, held/tail recovery risk expiry, and reset. `Tests/test_player_sense_source_contract.py` covers mappings, explicit registration/no world scan, progress isolation, recovery wiring, HUD labels and required automation names. The Gherkin acceptance contract is `Specs/PlayerSenseGameplay.feature`.

## Not yet validated / limitations

- UE 5.6.1 Build, Automation, keyboard, simulated gamepad and encounter regressions require Windows + UE and have not been run in the Linux Codex container.
- Ishibashiri's legacy `UColossusClimbingComponent` recovery is not yet multiplied by arm risk.
- Corruption warning categories refresh continuously, but encounter-authored lead-time values, VFX, audio and haptics remain future work.
- A full input-driven “sense, navigate, cling, purify, victory” automation playthrough is not added in this minimal source slice.
