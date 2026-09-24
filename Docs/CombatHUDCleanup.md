# Four-encounter combat HUD cleanup

## Scope and information contract

This change applies the short-text policy in `Docs/Design/04-InformationDesign.md` to Ishibashiri, Fuchimatoi, Minedaki, and Magatsune. Normal play keeps Kakon progress, stamina, danger anticipation, contextual Grab/Cling instructions, fall recovery, active Sense feedback, and Retry. The current combat font remains English-only because Japanese glyph rendering has not been validated.

Implementation-facing labels are opt-in through `-DebugGuidance`. State/phase labels, route node numbers, recovery multipliers, height, cling time, shake/fall/recovery counters, HP/posture, and broad control telemetry are not part of the normal HUD. Minedaki route guidance names the next physical action instead of exposing node numbers.

No new UI framework is introduced. Each Canvas HUD counts its optional Sense/debug rows and sizes its existing translucent background using the same draw scale. This prevents optional rows from extending below a fixed-height panel while keeping the existing immediate-mode drawing path.

## Before / after comparison

| Encounter | Normal display before | Normal display after | `-DebugGuidance` after |
|---|---|---|---|
| Ishibashiri | Short normal summary, but a fixed panel could be overrun by optional rows | Required status and contextual guidance in a row-counted panel | HP, posture, state/time, and broad controls remain available |
| Fuchimatoi | `primitive encounter`, action state, HP, coil percentage, route node, and recovery multiplier always visible | Kakon/stamina, danger, Grab/Cling, recovery, purification, and Retry guidance | State, HP, coil, route node, and recovery multiplier |
| Minedaki | State, route node, open-node limit, height, and telemetry always visible | Kakon/stamina, danger, route action, Grab/Cling, recovery, and Retry guidance | State, node/open limit, height, cling time, shake/fall/recovery counters, and multiplier |
| Magatsune | Phase/route already gated; recovery multiplier was normally visible and the panel height was fixed | Kakon/stamina, danger, Grab/Cling, recovery, purification, and Retry guidance | Phase and route node |

## Validation boundary

Source-contract tests verify debug gating, removal of development labels from normal strings, retained player-facing guidance, and variable panel sizing. The patch changes HUD source and acceptance documentation only; input conditions, collisions, boss state machines, Campaign progression, Sense behavior, and victory authority are untouched.

UE build/Automation and in-engine readability checks are **NOT RUN** because this environment does not provide Unreal Engine. In particular, font metrics, safe zones, ultrawide behavior, controller glyph clarity, and actual readability at supported resolutions still require an editor/device pass comparing normal launch with `-DebugGuidance`.
