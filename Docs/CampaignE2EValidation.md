# STEP 3C — Full Campaign E2E Validation Gate

## Scope and baseline

- Work-start `main` SHA: `dac60cb41979254388955b75b97c939c90695701` (the checkout's recorded `FETCH_HEAD`).
- A fresh fetch of `https://github.com/sasazaki1994/UE.git main` was attempted on 2026-09-15, but the container proxy returned HTTP 403. The remote tip therefore could not be independently refreshed.
- Target runtime: Windows, Unreal Engine 5.6.1.
- Available runtime here: Linux container without Unreal Engine. Runtime results below are deliberately **NOT_RUN / UNVERIFIED**.

## Acceptance and driver contract

`Specs/CampaignFlow.feature` now contains the input-only full-campaign acceptance scenario. `-CampaignE2E` is a thin coordinator: card screens are advanced through simulated `APlayerController::InputKey`, Ishibashiri Approach is walked by its controller-input driver, and each encounter GameMode starts its existing input playthrough driver. Encounter-specific route logic is not copied into the Campaign card controller.

The campaign run emits one `CAMPAIGN_E2E` timeline for chapter start and encounter clear times and ends with `CAMPAIGN_E2E_PASS total_campaign_time=...`. Existing encounter logs retain Kakon, retry, fall/recovery, transform-follow, stamina, Calm, Completed, Victory, and gamepad details.

The opening order is now `Title → Prologue → IshibashiriApproach → Ishibashiri`. The Ishibashiri campaign path deliberately uses movement/turn/dodge/grab/purify/retry/Sense input instead of the standalone climbing fixture's position setup. It holds Corruption Sense at the final transition so `TravelToCurrentChapter` exercises `ResetSense` while tearing down the old world. Fuchimatoi's campaign recovery path omits its standalone fixture-only direct stamina depletion.

## Windows UE 5.6.1 commands

```powershell
.\Tools\Prototype.ps1 -Action Build
.\Tools\Prototype.ps1 -Action Test -Campaign -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Campaign -SkipBuild -TestFPS 30
.\Tools\Prototype.ps1 -Action Test -Campaign -SkipBuild -Gamepad -TestFPS 60
```

Add `-Capture` to a keyboard run for rendered evidence. Encounter drivers write their existing ordered captures below `Saved/Screenshots/Climbing`, `Fuchimatoi`, `Minedaki`, and `Magatsune`; the `CAMPAIGN_E2E` timeline establishes their chapter ordering. No screenshots were produced in this environment.

## Result matrix

| Gate | Result | Evidence / note |
|---|---|---|
| UE version | UNVERIFIED | UE is not installed; target is 5.6.1 |
| UHT / Development Editor build | NOT_RUN | Requires Windows UE toolchain |
| UE Automation | NOT_RUN | Requires Windows UE runtime |
| Campaign keyboard 60 FPS | NOT_RUN | Command prepared above |
| Campaign keyboard 30 FPS | NOT_RUN | Command prepared above |
| Campaign gamepad 60 FPS | NOT_RUN | Command prepared above |
| Ishibashiri input route + one Retry + Sense | NOT_RUN | Driver wired; runtime proof required |
| Fuchimatoi normal + Fall Recovery | NOT_RUN | Driver wired with recovery path |
| Minedaki full + Fall Recovery | NOT_RUN | Driver wired |
| Magatsune full calming route | NOT_RUN | Driver wired; no HP/death behavior added |
| Sense reset at transition | NOT_RUN | Held-Sense runtime path prepared |
| Ending / Completed | NOT_RUN | Card input driver prepared |
| Individual encounter regressions | NOT_RUN | Requires Windows UE runtime |
| Screenshots | NOT_RUN | No render-capable UE runtime |

## Static validation, fixes, and remaining risk

Python source-contract tests and repository-level static checks are runnable in this container. No runtime bug is claimed as reproduced here. The implementation gap fixed at source level was that `-Campaign` previously ran only state-fixture Automation and could not launch a single input-driven four-encounter process. Campaign gamepad routing and the transition-safe success marker are now wired.

The decisive risk remains runtime behavior: compilation, fixed-step timing, collision-dependent input routes, camera usability, screenshot continuity, retry timer behavior, and Sense target teardown must all be executed on Windows UE 5.6.1 before this gate may be marked PASS. Until then the release decision is **UNVERIFIED — UE RUNTIME NOT AVAILABLE** and the next production/visual step must not treat STEP 3C as passed.
