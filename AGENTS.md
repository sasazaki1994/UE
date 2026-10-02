# 禍祓い AI Development Rules

This file is the repository-wide source of truth for Codex, Cursor, and other coding agents. Read it before changing anything. More specific instructions may refine it, but product documentation that describes an older prototype must not override the current source, tests, or accepted specifications.

## Project identity and design intent

`禍祓い` is a Japanese-world game in the direction of **「大歳ノ島風の和風世界観 × ワンダと巨像型の巨大生物戦」**. This is a tonal and structural reference, not permission to copy another work's characters, setting, assets, or prose.

Protect these pillars: a quiet Japanese world; confronting an enormous `主`; Grab; Climbing; finding and purifying `禍根` (Kakon); calming rather than killing a Nushi; minimal UI; solitude; scale; and traversal itself as the challenge.

The core loop is:

```text
主を発見 -> 接近 -> Grab -> Climbing -> 禍根探索 -> 禍根浄化
-> 主が抵抗 -> 次の禍根 -> 全禍根浄化 -> Calm
```

Do not turn this into a game about reducing enemy HP to zero or killing bosses.

## Current campaign and encounter identities

The implemented Campaign order is fixed:

```text
Title -> Prologue -> IshibashiriApproach -> 石走り -> Interlude1
-> 淵纏い -> Interlude2 -> 峰抱き -> Interlude3 -> 禍津根
-> Ending -> Completed
```

Do not add a fifth encounter or a new Nushi without an explicit request.

- **石走り / Ishibashiri:** giant boar, first encounter, and the reference encounter for scale, Grab, Climbing, and Kakon purification. It is the highest production priority. Its ground counter/posture loop creates the mount opportunity; only purification of its three Kakon leads through the shared Calm lifecycle to victory.
- **淵纏い / Fuchimatoi:** giant serpent whose route uses movement, the environment, and rock pillars.
- **峰抱き / Minedaki:** mountain-ape Nushi designed primarily as moving terrain.
- **禍津根 / Magatsune:** the natural source/outcrop of corruption and living terrain, not an evil final overlord or a fourth ordinary giant animal. Its abnormal state is calmed rather than destroyed.

`禍` is not evil itself; it is closer to a natural disaster or cyclical phenomenon. Nushi ordinarily sustain their land and ecosystem. Never villainize a Nushi, give Magatsune a malicious-demon-lord motivation, or rewrite calming as extermination. Shirotsura's baseline appearance has only two campaign-derived stages, `Early` and `Advanced`; do not proliferate cosmetic stages.

## Current implementation boundaries

Treat current source and contracts as more authoritative than historical Docs.

- `UCampaignGameInstance` owns the fixed chapter state machine, chapter travel, the Early/Advanced appearance mapping, and chapter-boundary persistence. `UCampaignSaveGame` intentionally stores only save version and resumable chapter. Campaign Save/Continue **already exists**; it is not forbidden. It does not persist Kakon, Stamina, boss phase, or other mid-encounter state. Completed clears the save, and automated campaign/demo runs isolate normal persistence.
- Each encounter GameMode owns spawning/configuration, Retry orchestration, and the guarded transition into Campaign. Retry resets the current encounter without advancing its chapter and must cancel pending victory travel.
- `ANushiBase`, `UNushiStateComponent`, `UNushiProgressComponent`, `ANushiEncounterManager`, and `AKakonActor` own the shared Nushi/Kakon lifecycle. Registered purification drives `OnAllKakonPurified -> CalmNushi -> Encounter Completed`; do not fabricate progress, bypass the manager, or invent a death path.
- `UGrabComponent` is the shared local-space attachment primitive used by later encounters. Ishibashiri additionally has its authored route behavior in `UColossusClimbingComponent`; do not force the two climbing models into a speculative framework.
- `UStaminaComponent` is shared where integrated; Ishibashiri route climbing retains encounter-specific scalar stamina behavior. Preserve each existing reset boundary.
- `UPlayerSenseComponent` is player-owned, read-only with respect to encounter authority, and answers direction/reachability and coarse danger state. It must not purify Kakon, deal damage, or own Campaign/Nushi progress.
- Encounter-specific AI, routes, environments, cameras, and solutions belong to their encounter. Share Grab, Climbing, Stamina, Kakon, Nushi lifecycle, `NushiEncounterManager`, Retry, Campaign chapter transition, PlayerSense, and minimal HUD only where the existing contract supports it.

Boss-specific behavior must not be generalized merely for DRYness. Prefer the existing responsible class over a giant manager, utility dumping ground, global singleton, or generic framework. `APrototypePlayer` can run under `AIshibashiriApproachGameMode`, so casts to `APrototypeGameMode` must remain nullable. Remember that command-line flags (`FParse::Param`) and map URL options (`UGameplayStatics::HasOption`) are separate channels.

## Scope and production priority

The priority is not more encounters or more features. It is a high-quality Ishibashiri vertical slice and a reliable four-encounter Campaign. Favor usability, readability, Grab success, Climbing recovery, camera and input feedback, Retry/Campaign/Save reliability, restrained HUD/presentation, and honest validation.

Do not add the following unless explicitly requested: large NPC systems, villager crowds, an open world, equipment collection, Inventory, Crafting, Skill tree, Multiplayer, GAS, large Quest sets, Dialogue trees, many Save Slots, new bosses, unnecessary Plugins, or speculative abstractions. “More elaborate” is not sufficient justification. Ask whether a proposal materially improves Ishibashiri or the existing Campaign.

Gameplay-specific AI and routes may remain encounter-specific. Do not move authority into presentation code, and do not change gameplay merely to accommodate visual assets.

## Classify every task first

- **Gameplay:** directly changes existing play rules. Handle cautiously and require regression coverage.
- **Reliability:** Retry, Save, Campaign, state, timers, and reset. Strong function-body/order/guard source contracts are encouraged.
- **Presentation:** HUD, camera, animation/VFX/audio hooks. It must not change gameplay authority.
- **Production tooling:** import, validation, build scripts, and evidence collection. Keep it separate from gameplay specifications.
- **Documentation:** Docs, Specs, acceptance criteria, and agent rules. Keep them aligned with current code.

## Required task-start workflow

1. Confirm the latest `main` commit and inspect open PRs. If network/authentication prevents either, report the limitation; do not claim it was checked.
2. Read the requested files.
3. Read related tests.
4. Read related current Docs and Specs.
5. Inspect current source responsibilities and determine whether an existing feature already solves the problem.
6. Avoid unnecessary edits to files being changed by an open PR.
7. Choose the smallest coherent change.

Use **Small PRs**: one problem, one contract, or one usability improvement. Do not bundle unrelated cleanup or lead with a large refactor.

## Implementation and validation

- `.clang-format` is authoritative for C++; retain existing Unreal Engine naming and patterns.
- Use `Tests/` Python source-contract tests when UE Runtime is unavailable. Prefer extracting a function body and checking ordering, guards, authority boundaries, and reset responsibility over fragile tests that merely search the whole repository for a string.
- `Tests/production-gameplay-contract.json` and `Tools/GameplayContract.py` protect gameplay source. After an **intentional gameplay-source change only**, run `python Tools/GameplayContract.py --update`; never edit the JSON manually and never baseline unintended changes merely to make a test pass.
- Run a focused test first when one exists, then run `python -m pytest Tests -q` where possible.
- With an appropriate Windows Unreal environment, use `Tools/Prototype.ps1` for Check, Build, Setup, Play, Test, Package, and related Automation/E2E paths. Do not replace this UE project with another engine or a web implementation.

When UE is unavailable, prioritize C++ review, source contracts, Campaign/Save/Retry state, timer safety, input routing, Specs, acceptance criteria, CI, production tooling, and import contracts. Static evidence can improve confidence but cannot establish runtime quality.

Report validation truthfully with these terms:

- **PASS:** the stated runtime/test command was actually executed and succeeded.
- **FAIL:** it was actually executed and failed.
- **NOT_RUN:** it was not executed or the required environment was unavailable.
- **STATIC_PASS:** only a source-contract or other static check succeeded.

Python passing on Linux is not “UE Runtime PASS.” Do not mark gameplay feel, camera feel, animation quality, Grab or Climbing visual contact, physical gamepad behavior, Japanese glyph rendering, material quality, Lumen/VSM, audio balance, packaged builds, or a human first play as complete from static checks. If not performed in the required environment, record them as `NOT_RUN`.

## Documentation discipline

Specs and acceptance criteria describe intended behavior, but old status reports are historical snapshots. Before repeating claims, compare them with current C++, tests, `README.md`, and the newest relevant validation record. Never describe the already implemented Campaign, four encounters, IshibashiriDemo, Campaign Save/Continue, Grab, Climbing, Stamina, Kakon, PlayerSense, or shared Nushi lifecycle as forbidden or wholly unimplemented.
