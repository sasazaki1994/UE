# 石走りDemo 地上ルートE2E失敗の修正検証（2026-10-04）

分類: Reliability / Production tooling。Gameplay sourceは変更していない（`python Tools/GameplayContract.py` は all digests match、`--update` 未実行）。

## 症状

最新main `079c0f1` の `Prototype.ps1 -Action Test -IshibashiriDemo -TestFPS 60` が、石走り地上フェーズで `CLIMB_TEST_FAIL <RunId> phase=-1: Ground dodge and counter route preserves health and encounter` となる。

## 原因

`AClimbingIntegrationTest::DriveMountWindow` のChase / Recover（counter不可）分岐は、誘導位置を `Boss + (-Boss).GetSafeNormal2D() * 850` というワールド原点基準で計算していた。旧blockoutは原点をはさまずに配置されるため成立していた。しかし `b6ac2aa` 以降、Demoの石走り章は `?BasinPrototype`（Player `(-3000,0)`、Boss `(550,0)`）で開始する。Chase中にBossが原点を越えると誘導方向が反転し、driverがPlayerを次の突進線上へ歩かせていた。

- 反転後のTelegraph開始距離は376 cmで、Charge 2 frame目の286 cmで被弾した。予定していた回避入力は0.1秒後だった。blockoutではTelegraph開始距離が常に779 cm以上ある。
- 修正前はenvironment asset、`-PrimitiveEnvironment`、`-Climbing -Basin` のいずれも同じ844 frameで決定的に失敗し、blockoutの `-Climbing` はPASSした。
- 物理Xbox controllerを遮断した状態（XInputDevice未mount）でもmainは844 frameで2回とも失敗した。したがってcontrollerの混入は失敗原因ではない。

Bossの回避性能、突進timing、被弾判定、Health、体勢値、禍根進行は変更していない。

## 修正（3 commit、stack）

1. `abe7f51` Isolate automated UE test runs from physical gamepads
   - `Prototype.ps1 -Action Test` に `-DisablePlugins=XInputDevice` を付与する。XInputDeviceがmountされた場合はFAILにする。Play / Editorは対象外。
   - 接続中の物理controllerがscripted driverに移動入力とAttack入力を混入させていたため、別修正として分離した。
2. `3b2361e` Fix demo review gate automation success marker for UE 5.6 log format
   - `RunIshibashiriDemoReviewGate.ps1` のregex `Result=\{Success\}\.` が、UE 5.6の実ログ `Result={Success} Name={...} Path={...}` に一致していなかった。
   - このregexは `aaae1aa` で導入され、成功したGrabAssist AutomationもFAIL扱いしてDemo runの前にゲートを停止させていた。
3. `6502c3d` Bait the Ishibashiri ground driver from the player's side of the boss
   - 誘導位置を `Boss + (Player - Boss).GetSafeNormal2D() * 850` に変更した。
   - 被弾検査、`Tap(LeftShift)` / `Tap(LeftMouseButton)` による通常入力、失敗文言はそのまま残している。

回帰テストは次のとおり。

- `Tests/test_ishibashiri_demo_source_contract.py::test_demo_ground_driver_baits_from_player_side_in_the_basin`: 修正前のdriverではFAIL、修正後はPASS。
- `Tests/test_prototype_test_input_isolation.py`
- `Tests/test_ishibashiri_demo_review_gate_runner.py::test_automation_success_marker_matches_the_ue56_log_line`: 旧regexではFAIL、修正後はPASS。

## 検証結果

環境: Windows、UE 5.6.1、ローカル。対象source SHAは `6502c3df838169ba73721b0b591a2bc7255ccf85`（作業ツリーclean）。

### Review Gate: PASS

`.\Tools\RunIshibashiriDemoReviewGate.ps1` の Verdict は **PASS**。

Evidenceはgit追跡対象外の `Artifacts/IshibashiriDemoReviewGate/20261004T172319Z-6502c3df8381/` にある。

| Gate | Result | RunId / log |
|---|---|---|
| build | PASS | `logs/build.stdout.log` |
| grabAssistAutomation | PASS | `logs/grabAssistAutomation-automation.log` |
| demo60 | PASS | `36d1432d337d42e287c555e83941cc11` / `demo/per-run/IshibashiriDemo-60-backup-2026.10.04-17.24.01.log` |
| demo30 | PASS | `0c416aedbc74430dbe66c6ab6a65d71f` / `demo/IshibashiriDemo-30.log` |
| demoGamepad（模擬入力） | PASS | `7760c30d82f14f5f95c67fcf88ce7ea9` / `demo/per-run/IshibashiriDemo-60-backup-2026.10.04-17.24.36.log` |
| demoCapture | PASS | `0fae9a35a0cc4834ad1ca3337d10d712` / 14枚 `screenshots/IshibashiriDemo/0fae9a35.../` |
| demoHighQuality | PASS | `26a7194bf25b4ad6b56a96646eaa0236` / `demo/IshibashiriDemo-60.log`, 14枚 |
| demoCompletion / saveIsolation / retry / senseReset | PASS | 上記Demo logs（`routes=2 retry=1 sense=boundary,corruption`） |
| normalCampaignRegression | PASS | `5b78e63211864641a6304b5550a7707d` / `campaign-regression/Campaign-60.log` |
| package | PASS | `logs/package.stdout.log`, `package/Windows/` |

- Prototype.ps1経由の全Demo / Campaign runでは、XInputDeviceがmountされていない。
- GrabAssist AutomationはEditor起動であり、`Prototype.ps1 -Action Test` を経由しないため、XInputDeviceがmountされる。

### 集中run（修正後、物理入力遮断あり）

- PASS:
  - `-Climbing -Basin` 60 FPS ×2: `6366d03c...`, `ada26793...`
  - `-Climbing -Basin` 30 FPS: `1966cb85...`
  - `-Climbing` blockout 60 FPS: `bec54455...`
  - `-Climbing` blockout 30 FPS: `8b1c76ef...`
  - `-ClimbingGamepad -Basin`: `1abbcc31...`
- FAIL: `-GrabMotionWarp`（`59ba2de9...`）は phase 0 "asset contract required" で失敗した。地上ルートはKneelまで到達している。既知の未作成Montage assetによる別件である（[GrabMotionWarpValidation](GrabMotionWarpValidation.md)）。
- 修正前のrunを含む全logは、Evidence内の `investigation/` にある。

### Static

- `python -m pytest Tests -q`: 230 passed, 18 subtests passed（STATIC_PASS）。
- `python Tools/GameplayContract.py`: all digests match（STATIC_PASS）。

### NOT_RUN

- Package exeでのE2E（このゲートの対象外）
- 物理ゲームパッド操作
- 人間の初見プレイ
- 地上ルートの見た目・カメラ・操作感

## 別件として未解決のもの

- **BasinScenario 落下失敗**: 未調査。driverは `BasinPlaythroughTest.cpp` で、今回のワールド原点基準の誘導式は含まない。同一原因とは判断していない。
- **`04-Victory.png` 不整合**: 未調査。既定のPrototype Capture（`PrototypeSmokeTest`）の画像で、Demoの14枚には含まれない。
- **原点基準の誘導式の残存**: `PrototypePlaythroughTest.cpp` 196行と `PrototypeSmokeTest.cpp` 212・380行に同種の原点基準式が残る。Basin配置で使うと同じ問題が起こり得るが、今回は変更していない。
- **`Config/DefaultInput.ini` の書き換え**: GrabAssist Automation（Editor起動）のたびに全量シリアライズされた内容へ書き換えられる。BaseInput.iniとの合成後の値は同等と見られるが、追跡対象ファイルを汚すため、実行後に復元した。
