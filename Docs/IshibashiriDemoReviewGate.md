# Ishibashiri Demo Review Gate

## 目的

`Tools/RunIshibashiriDemoReviewGate.ps1` は、Windows + Unreal Engine **5.6.1** で石走り専用Demoを1コマンドでBuildからPackage可否まで判定し、同一SHAのEvidenceを保存するためのゲートである。静的テストやDryRunは実ゲームの成功を意味しない。現時点のUE Runtime、PNG、Packageは **NOT_RUN — WINDOWS UE 5.6.1 REQUIRED**。

## Full Review Gateとの違い

既存の `RunIshibashiriReviewGate.ps1` はApproach、石走り単体、4戦Campaign、および各ボスの30/60 FPS・Gamepad matrixを扱う歴史的なFull gateであり、変更しない。本ゲートは見せられる石走りDemoを最初の完成目標にし、通常Campaignは60 FPS E2Eを1本だけ回す。Fuchimatoi、Minedaki、Magatsuneの単体matrixは再実行しない。

## Windowsでの実行方法と実行順

```powershell
.\Tools\RunIshibashiriDemoReviewGate.ps1
```

1. `Prototype.ps1 -Action Build`
2. IshibashiriDemo 60 FPS
3. IshibashiriDemo 30 FPS（固定step差の整合確認であり性能測定ではない）
4. IshibashiriDemo Gamepad / 60 FPS
5. IshibashiriDemo Capture / 60 FPS
6. IshibashiriDemo HighQuality Capture / 60 FPS
7. Demoログから完走、Save隔離、Retry、Boundary/Corruption Sense resetを判定
8. 通常Campaign 60 FPSを最小Regressionとして1本実行
9. 必須GateがすべてPASSの場合だけPackage

Buildが失敗した場合は直ちにFAILとして終了し、Gameplayには進まない。

## 必須Gateと成功証拠

`build`, `demo60`, `demo30`, `demoGamepad`, `demoCapture`, `demoHighQuality`, `demoCompletion`, `saveIsolation`, `retry`, `senseReset`, `normalCampaignRegression` がPackage前の必須Gateで、`package` は独立結果である。初期値はすべて `NOT_RUN`。

- 各Demo E2Eはexit codeだけでなく `ISHIBASHIRI_DEMO_E2E_PASS <RunId>` を `Prototype.ps1` が要求する。
- 完走は石走り開始、3禍根・Calm・Encounter Completedの内部check後にだけ出る `CLIMB_TEST_PASS`、Interlude1、`ISHIBASHIRI_DEMO_COMPLETE`、Title復帰、E2E PASSをログで要求する。
- Save隔離はDemo E2E完走を要求したうえで、Demo evidence内に `CAMPAIGN_SAVED`、`CAMPAIGN_CONTINUE`、`CAMPAIGN_SAVE_CLEAR_FAILED` が一つもないことを要求する。ソース契約ではDemo modeがload前からpersistenceを無効化することも別途固定する。
- Retryは既存Campaign E2Eの石走りdriverが、Retry後に全ルートを再攻略して `routes=2 retry=1` を記録することを要求する。これにはKakon、Stamina、Grab/Climb、Demo mode、Save隔離の継続が含まれる。
- Senseは既存の `PLAYER_SENSE reset` とBoundary/Corruption両方の入力証拠を要求する。
- Captureは `Saved/Screenshots/IshibashiriDemo/<RunId>/` の `01-Title.png` から `14-ReturnToTitle.png` まで全14ファイルの実在と100 byte以上を `Prototype.ps1` が検査する。
- HighQualityはprocess成功だけでは足りず、D3D12、SM6、Lumen GI、Lumen Reflections、Virtual Shadow Mapsの実ログ証拠が必要。

## Optional quality gate

Control Rigは **BLOCKED / OPTIONAL QUALITY GATE** として完成度改善項目に残す。asset不在をRuntime PASSとは扱わない一方、それだけを理由にDemo Packageを永久にblockしない。

## Package条件とVerdict

上記11個の必須Gateがすべて `PASS` の場合だけ `Prototype.ps1 -Action Package` を実行する。GameplayまたはPackage失敗は `FAIL`、必須実行が残れば `PARTIAL`、全必須GateとPackage成功は `PASS`、DryRunまたは利用不能環境では `NOT_RUN`。Python testの成功だけでRuntime `PASS` にはしない。

## Evidence構造

既定保存先は `Artifacts/IshibashiriDemoReviewGate/<Timestamp>-<SHA>/`。

```text
environment.json
summary.json
summary.md
logs/
screenshots/
performance/
demo/
campaign-regression/
package/
```

`environment.json` と各run recordにはsource SHA、UE version、command、start time、exit code、stdout、stderr、expected artifact、actual artifact、resultを残す。生成されたUE logs/screenshotsとPackageも同じrunのEvidenceへコピーする。

## DryRun

```powershell
.\Tools\RunIshibashiriDemoReviewGate.ps1 -DryRun
```

予定する8コマンドを順番付きで表示し、Build、Unreal process、Screenshot、Packageは一切起動しない。全GateとVerdictは `NOT_RUN` のままであり、DryRun成功をRuntime成功として扱わない。
