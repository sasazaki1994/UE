# Four-encounter combat HUD cleanup

## Scope and information contract

This change applies the short-text policy in `Docs/Design/04-InformationDesign.md` to Ishibashiri, Fuchimatoi, Minedaki, and Magatsune. Normal play keeps transient Kakon progress, contextual non-numeric stamina, danger anticipation, one contextual Grab/Cling/Purify/Recovery instruction, active Sense feedback, and defeat-only Retry. The current combat font's Japanese glyph rendering still requires runtime validation.

Implementation-facing labels are opt-in through `-DebugGuidance`. State/phase labels, route node numbers, recovery multipliers, height, cling time, shake/fall/recovery counters, HP/posture, and broad control telemetry are not part of the normal HUD. Minedaki route guidance names the next physical action instead of exposing node numbers.

No new UI framework is introduced. Each Canvas HUD keeps its existing immediate-mode drawing path, draws status cues only when their context is active, and maintains encounter-local presentation memory for the 1.5-second purification notice.

## Before / after comparison

| Encounter | Normal display before | Normal display after | `-DebugGuidance` after |
|---|---|---|---|
| Ishibashiri | Contextual prompts, stamina, and purification notice | Adds active-only Sense feedback and a shorter Calm result | HP, posture, state/time, route, numeric progress/stamina, and broad controls remain available |
| Fuchimatoi | Permanent title, numeric Kakon/stamina, route prose, and controls | One prompt, contextual stamina, transient progress, active Sense, recovery marker, and concise result | State, HP, coil, route node, recovery multiplier, exact resources, and controls |
| Minedaki | Permanent title, numeric Kakon/stamina, long route prose, and controls | One prompt, contextual stamina, transient progress, active Sense, recovery, and concise Calm | State, node/open limit, exact resources, height, cling time, counters, multiplier, and controls |
| Magatsune | Permanent title, numeric Kakon/stamina, phase prose, and controls | One prompt, contextual stamina, transient progress, active Sense, recovery, and concise Calm | Phase, route node, exact resources, telemetry, multiplier, and controls |

## Validation boundary

Source-contract tests verify debug gating, removal of development labels from normal strings, retained player-facing guidance, and variable panel sizing. The patch changes HUD source and acceptance documentation only; input conditions, collisions, boss state machines, Campaign progression, Sense behavior, and victory authority are untouched.

UE build/Automation and in-engine readability checks are **NOT RUN** because this environment does not provide Unreal Engine. In particular, font metrics, safe zones, ultrawide behavior, controller glyph clarity, and actual readability at supported resolutions still require an editor/device pass comparing normal launch with `-DebugGuidance`.
