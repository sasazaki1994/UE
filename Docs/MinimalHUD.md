# Minimal encounter HUD

## Information classification

The four encounter HUDs share an information policy, not a new gameplay or generic HUD framework.

| Classification | Information |
|---|---|
| Normal play | One highest-priority contextual Grab, Cling, Purify, Dodge, or Recovery prompt; a short danger cue; active Sense feedback; a non-numeric stamina bar only while mounted and stamina is relevant; a transient Kakon notice immediately after purification; concise Calm or Defeat presentation |
| Context only | Recovery direction/marker, stamina bar, purification progress, Boundary Sense strength, Corruption Sense warning, and input prompt |
| `-DebugGuidance` only | Developer encounter name, exact HP/Kakon/stamina values, action state or phase, route node/open limit, recovery multiplier, height, cling/shake/fall/recovery counters, telemetry, and the complete control legend |

Normal HUD is **contextual, non-numeric, and transient**. `-DebugGuidance` is **numeric and may expose state, route, and telemetry**. The HUD reads existing encounter state and never owns purification, stamina, route, recovery, Campaign, or encounter completion.

## Prompt priority

Each encounter selects at most one normal prompt. Immediate survival comes first, followed by Purify, Grab, Recovery, and other local route guidance. Sense feedback and the stamina bar are status cues rather than additional action instructions. Existing world recovery markers remain intact.

Presentation memory is encounter-local: the HUD observes purification count changes and displays progress for 1.5 seconds. It is reset when the observed boss changes or encounter progress resets; nothing is added to Save data.

## Validation boundary

Source-contract tests protect debug isolation, the absence of persistent control legends, active-only Sense labels, single prompt selection, transient Kakon progress, non-numeric contextual stamina, and read-only HUD authority. Unreal runtime visual validation remains required for font glyphs, safe zones, spacing, overlap, and controller readability.
