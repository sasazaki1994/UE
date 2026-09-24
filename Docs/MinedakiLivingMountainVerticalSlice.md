# 峰抱き Living Mountain Vertical Slice

## Status and intent

This slice keeps the established Gameplay Contract: fourteen ordered route nodes, shared Grab and Stamina, three `AKakonActor` instances, common Nushi progress/state, non-lethal Calm, recovery anchors, Retry, and campaign completion. It adds no HP, death, route HUD, new Sense, or content system. The goal is for the existing traversal to read as climbing a living mountain rather than fighting another boss.

## Before this change

The complete leg/back/arm/shoulder/head route existed, but stable phases were visually static. The phase-one Shake entered its violent motion before presenting an explicit named warning state. Arm Bridge movement and a forced Shake check happened in the same transition, so the signature route-forming moment could feel like an arbitrary punishment. The camera used one mounted distance and snapped directly to its desired location. Minedaki also did not consume the shared purification presentation envelope introduced by the previous encounter-polish work.

## IMPLEMENTED

- A bounded 14 cm, six-second breathing translation moves `BodyRoot`, the body-relative Grab frame, route, and Kakon together. Shared Calm interpolation reduces it to zero, preserving local attachment and route authority.
- Phase one now has an explicit `ShakeWarning` interval. Slow three-degree body tension, amber pulsing local light, camera response through body motion, and existing Corruption Sense precede the later single Cling check. The existing rest nodes and restore rates remain authoritative.
- Arm Bridge reserves its first quarter for anticipation, then eases the arm and torso into the bridge over the remaining time. Camera focus opens toward the bridge and route nodes remain closed until formation completes. Route-forming transitions no longer call `ResolveShake`; only the authored phase-one Shake can throw a player for a missed Cling.
- Grab travel uses eased interpolation and body-relative facing, reducing visible sliding while keeping the same nodes, speed budget, input, and `UGrabComponent` ownership.
- Mounted camera position now uses low-frequency lag. Arm Bridge opens distance/FOV and frames its destination; shoulder/head traversal biases toward the crown. Purification adds only a 1.5-degree FOV response, not rotational camera shake.
- Minedaki calls the shared gameplay-neutral purification and Calm presentation envelopes. Purification drives a short teal local-light pulse; rest states use a dim safe light; Calm suppresses breathing and warning light. Kakon progression remains immediate and unchanged.
- Fall still returns only to the latest recovery shelf (ground, node 7, or node 11), restores partial stamina, and requires normal Grab to reconnect. Retry still resets all encounter and camera presentation state without respawning actors.

## Reused versus Minedaki-specific

Reused: `ANushiBase::NotifyPurificationPresentation`, `AdvancePresentation`, `GetPurificationPresentation`, and `GetCalmPresentation`; shared Kakon/progress/state/encounter completion; Grab, Stamina, player Sense, recovery, and Retry.

Minedaki-specific: breathing displacement of the climbable body frame, the phase-one tension/Shake cadence, arm-bridge formation, body-relative route alignment, rest lighting, and route-aware camera framing. These are deliberately not pushed into a common framework.

## VERIFIED

Linux source-contract and narrative/gameplay contract checks are recorded by the PR test run. They verify the warning state, anticipation fraction, absence of a transition Shake check, shared presentation calls, camera interpolation, route/Kakon count, and prohibited narrative regressions. They do not validate rendered feel.

## ASSET_REQUIRED

The checkout has no production Minedaki skeletal boss model, breathing/brace/shoulder/arm-bridge/Shake/Calm animations, AnimBP, Control Rig or IK Rig, dedicated surface-direction material, Kakon subsurface material, Niagara, or sound. The primitive source implementation is not a substitute for those assets.

Editor hookup should keep `BodyRoot` as gameplay authority. Drive a future AnimBP with `ActionState`, `TransitionAlpha`, `PurificationPresentation`, and `CalmPresentation`; use material scalar parameters `CorruptionPulse`, `CalmAmount`, and `RouteWarmth`; align authored bridge sockets to nodes 8–10 and rest sockets to nodes 4, 7, and 11. Animation root motion must not own route transforms.

## NOT_RUN

Windows, UE 5.6.1, MSVC, and a rendering RHI are unavailable here. Development Editor/Game builds, Unreal automation, standalone play, fixed-timestep 30/60 FPS runs, real rendering performance, simulated/physical gamepad, all gameplay beats, campaign regression, and the nine requested captures are therefore `NOT_RUN`. Fixed-timestep correctness and measured GPU frame time remain separate validation gates.

## Remaining debt and next PR

Primitive limbs cannot prove hand contact, surface readability, silhouette, or animation timing. Camera collision and comfort need all-angle human review; the body-relative facing basis needs verification on the production mesh; local-light intensity needs Legacy/High Quality capture comparison. The next single PR should be a Windows UE 5.6.1 validation/tuning pass: build, run automation and full keyboard/gamepad loops at fixed 30/60, capture first sight through Calm, measure real frame time, then adjust only warning duration, bridge framing, camera lag, and safe amplitudes from evidence.
