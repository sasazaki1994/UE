# 淵纏い：落下復帰と再挑戦

基準は `origin/main` の `23d200a`（PR #36）。作業ブランチは `codex/work_fuchimatoi-recovery`。

追加の実時間描画計測は [実時間の描画性能検証](FuchimatoiPerformance.md) を参照。固定FPSの攻略検証と、オフスクリーン実描画の性能値を分けて記録している。

Coiling後に落下しても、地上の金色ビーコンから同じ戦闘の登攀へ戻れる。Kakon 1/3、2/3の両方で復帰でき、同じ入口を繰り返し使える。明示的なR/YのEncounter Retryだけが戦闘を初期化する。

## 実装報告

| 要求項目 | 実装・根拠 |
|---|---|
| 1. 変更ファイル | 下記のファイル一覧。共有Grab、Stamina、石走りのコード・資産は変更なし。 |
| 2. Recovery Point構成 | 既存 `AFuchimatoiRouteAnchor` を1個追加。初期世界座標 `(-850,-180,190)` cm、Boss親のローカルTransformで保持。地面から近づける、岩との接触を避けた位置。金色球・高さ4mのDebug Beam・HUDの `RETURN HERE` と距離で案内する。 |
| 3. 有効条件 | Coiling完了、Action=Coiling、Nushi=Active、Encounter=Running、未Grab、CharacterMovementがWalking、Player中心Z<200cm。Calm/Completed/Defeat/初期Phase/空中/高所の岩上では無効。Grab入力時には非Dodge、Stamina>=25、アンカーまで3D距離240cm以内も必要。 |
| 4. 再接続Node | 内部Node 3（HUD 4/10、第一禍根の蛇胴体）。復帰アンカーを `UGrabComponent::TryGrab` し、既存 `AdvanceRoute` の補間・相対Transform・隣接アンカーへの再Grabを利用して接続。速度300cm/s、約2.48秒。既存10 Node配列は維持。進捗別の別入口は追加しない。 |
| 5. Progress維持 | 復帰はPawnのGrab先、Node、補間状態のみ更新。Manager/Boss/KakonへのResetは一切呼ばない。3個の既存Kakon、NushiProgress、Boss Transform、CoilingProgress、蛇/岩アンカーをそのまま使用。 |
| 6. Stamina復帰 | 既存の未Grab時18/s、岩で静止28/sの回復を維持。通常Grabと復帰Grabは同じ最低25。復帰成功から3秒だけ消費停止し、Node 3到着後はNode 4の岩棚で休める。HP回復・無敵付与・上限変更はない。 |
| 7. Missed Grab | 未GrabのSnaggedが4.5秒で満了 → 既存WithdrawHead → Submerged 1.5秒 → 次のWindup。同じEncounterで再Snagし、Grab成功まで入力テストで確認する。 |
| 8. 被弾後の復帰 | 既存の1HP減少・Knockback → WithdrawHead → Submerged → 次のBite。Recoveryテストの最初に被弾し、HP2のまま岩誘導、2回の落下復帰、Victoryまで進める。HP0のDefeatは従来どおり。 |
| 9. Bite誘導表示 | Bite待機/Windupだけ `LURE THE BITE INTO THE ROCK`。既存金色岩・足元の金色四角に加え、Windup/Lunge中に岩への金色矢印。Coiling以降は四角を隠して復帰入口との混同を減らす。Lock後の補助文も横回避に統一。 |
| 10. Target Lock | Windup中はPlayer XYに追従するオレンジの地上円。Lunge開始で位置を保存し赤色固定に切替、`TARGET LOCKED (RED) - DODGE SIDEWAYS NOW`。攻撃終点を750cm延長して岩に当てる既存ロック処理は維持。 |
| 11. Grab可能表示 | 未GrabのSnagged窓だけ頭アンカーを大きい金色球にし、`GRAB NOW`。Grab後は通常のRoute表示へ戻し、専用Grab表示は消す。 |
| 12. Route表示 | 現在Nodeを小さく暗く、次のNodeを大きく表示。接続線は次の1区間だけ。進行中のDestination、後退入力、復帰中のNode 3への接続にも対応。水色=Snake、黄色=Rockを維持し、通過済み・遠方Nodeを隠す。 |
| 13. Kakon表示 | 次に浄化できる1個を大きい赤球、Lockedを小さい暗赤色、Purifiedは消灯。Kakon 2はCoiling完了＋Kakon 1浄化、Kakon 3はさらにKakon 2浄化が必要。表示と攻撃受付条件を揃える。 |
| 14. Camera | 淵纏いPawnの既存SpringArmに、石走り登攀Cameraと同じSphere Sweepによる退避を補足。Player中心+35cmから実CameraまでCamera channel、半径18cmで確認し、遮蔽時だけ接触手前に退避してPlayerを見る。共有Cameraは変更しない。Bait Rock/岩棚/岩柱でYaw24通り×Pitch3通りの視線検証を追加。 |
| 15. 中心線長 | テスト実測でCoiling前2728.41cm（27.2841m）、後5461.25cm（54.6125m）、約2.002倍。今回は配置を維持。1.5倍目標は未達で、長さ保存Solverは追加していない。 |
| 16. Telemetry | `FUCHIMATOI_TELEMETRY` にBite attempts、Player hits、Rock lures、Snags、Grab attempts/successes、Falls、Recovery grabs、Kakon purified、elapsed、completion_timeを記録。各イベントとCompleted/Retryに集計を出し、Retryでカウンター初期化。完了前のcompletion_timeは-1。Fallsは任意Detach・Stamina枯渇・Route接続失敗を数え、通常のNode間Releaseは数えない。外部SDKなし。 |

## 検証の読み方

| 要求項目 | 結果 |
|---|---|
| 17. 通常Fuchimatoi | 60FPS Keyboard、30FPS Keyboard、60FPS GamepadすべてPASS。各実行は2周クリア・Retry・被弾リセットを含む。 |
| 18. Recovery | 60FPS Keyboard＋描画Capture、30FPS Keyboard、60FPS GamepadすべてPASS。各Recovery実行の最初の1戦で、空振り・被弾・Missed Grab・1/3と2/3での2回の落下復帰を経てVictory。低Stamina拒否、空中拒否、Progress/配置維持、Calm無効、Retry初期化も確認。 |
| 19. 60FPS | 通常攻略・復帰ともKeyboard/GamepadでPASS。描画付き復帰で15枚のPNG出力も確認。 |
| 20. 30FPS | 通常攻略・復帰ともKeyboardでPASS。 |
| 21. Gamepad | 60FPSで通常攻略・復帰をPASS。LSとRB/B/X/A/Yのシミュレート入力。実機接続試験ではない。 |
| 22. 石走りRegression | Build成功、Climbing+Basin60、ClimbingGamepad+Basin60、Grab60、Camera+Basin60の指定4件がすべてPASS。既存UE Automationも13/13成功、失敗0、テスト内警告0。 |
| 23. Screenshot保存場所 | 今回の15枚を `Docs/FuchimatoiRecoveryCaptures/` に保存。元画像は `Saved/Screenshots/Fuchimatoi/<RunId>/`。最終RunId・ログ・完了時Telemetry・各ファイルSHA256は [検証JSON](FuchimatoiRecoveryValidation.json) に保存。 |

生ログとAutomationレポートは `Saved/FuchimatoiRecoveryValidation/`。UE Editor起動時の入力設定自動生成は実行後に元のファイルへ復元し、入力テストはリポジトリ本来の設定で実行した。起動時に既存の未配置Control Rig等のエラーが出る環境のため、成否は各実行固有RunIdのPASSとAutomationのテスト結果で判定している。

最終60FPS Capture RunIdは `029e2a1c0b6644ef8ec15f021a5c9cd8`。その最初の戦闘の完了ログはBite 5回、被弾1回、岩誘導/Snag 2回、Grab試行7回・成功3回、落下2回・Recovery Grab 2回、浄化3個、完了時間64.70シミュレーション秒。Grab失敗4回は、各落下で空中拒否と低Stamina拒否を1回ずつ確認したもの。

`-Recovery` は通常Fuchimatoi入力テストの追加シナリオ。空振り→被弾→Snag時間切れ→次周期でGrab→Kakon 1→Coiling→任意Detach→着地→再Grab→Kakon 2→2回目Detach→再Grab→Kakon 3→Victoryを同じEncounterで実行し、その後通常攻略・Retryも確認する。

攻略用の移動、回避、Grab、Detach、浄化、RetryはPlayerControllerへのキー/スティック入力で行う。例外となるテスト用fixtureは、低Staminaの拒否を確認するための明示的なStamina消費と、Camera角度の網羅検証だけ。Actor位置・Kakon浄化・Encounter進捗を直接変更して攻略しない。高所からBait Rock上へ落ちた場合も、歩行入力で岩から地上へ降りてから復帰する。

```powershell
.\Tools\Prototype.ps1 -Action Build
.\Tools\Prototype.ps1 -Action Test -Fuchimatoi -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Fuchimatoi -SkipBuild -TestFPS 30
.\Tools\Prototype.ps1 -Action Test -Fuchimatoi -Gamepad -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Fuchimatoi -Recovery -Capture -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Fuchimatoi -Recovery -SkipBuild -TestFPS 30
.\Tools\Prototype.ps1 -Action Test -Fuchimatoi -Recovery -Gamepad -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Climbing -Basin -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -ClimbingGamepad -Basin -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Grab -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Camera -Basin -SkipBuild -TestFPS 60
```

## ファイル一覧

- `Source/IshibashiriPrototype/Public/FuchimatoiBoss.h` / `Private/FuchimatoiBoss.cpp`：復帰アンカー・有効条件・誘導・Telemetry・Kakon順序。
- `Source/IshibashiriPrototype/Public/FuchimatoiPlayer.h` / `Private/FuchimatoiPlayer.cpp`：既存Grab/Routeへの復帰・Stamina救済・Detach計測・Camera退避。
- `Source/IshibashiriPrototype/Public/FuchimatoiRouteAnchor.h` / `Private/FuchimatoiRouteAnchor.cpp`：現在/次/Grab用のMarker表示。
- `Source/IshibashiriPrototype/Public/FuchimatoiArena.h` / `Private/FuchimatoiArena.cpp`：Coiling後のBait Spot非表示。
- `Source/IshibashiriPrototype/Private/FuchimatoiHUD.cpp`：状況別の短文と復帰位置案内。
- `Source/IshibashiriPrototype/Private/FuchimatoiGameMode.cpp`：Completed時のTelemetry記録。
- `Source/IshibashiriPrototype/Public/FuchimatoiIntegrationTest.h` / `Private/FuchimatoiIntegrationTest.cpp`：復帰・失敗再挑戦・Cameraの検証。
- `Tools/Prototype.ps1`：`-Fuchimatoi -Recovery`、専用ログ名、15枚のCapture検証。
- `Docs/FuchimatoiRecovery.md` / `Docs/FuchimatoiRecoveryValidation.json` / `Docs/FuchimatoiRecoveryCaptures/`：今回の報告と証跡。
- `Docs/FuchimatoiVerticalSlice.md`：旧検証記録から今回の報告への参照。

## Gameplayキャプチャ

| 場面 | 画像 |
|---|---|
| 岩へ誘導・Windup | [01-BiteWindup](FuchimatoiRecoveryCaptures/01-BiteWindup.png) |
| Target Lock | [02-BiteLunge](FuchimatoiRecoveryCaptures/02-BiteLunge.png) |
| Snag＋Grab案内 | [03-Snagged](FuchimatoiRecoveryCaptures/03-Snagged.png) |
| 頭Grab | [04-HeadGrab](FuchimatoiRecoveryCaptures/04-HeadGrab.png) |
| 第一禍根 | [05-Kakon1](FuchimatoiRecoveryCaptures/05-Kakon1.png) |
| Coiling | [06-Coiling](FuchimatoiRecoveryCaptures/06-Coiling.png) |
| 1/3で意図的落下 | [11-IntentionalFall](FuchimatoiRecoveryCaptures/11-IntentionalFall.png) |
| 地上の復帰入口 | [12-RecoveryPoint](FuchimatoiRecoveryCaptures/12-RecoveryPoint.png) |
| 再Grab | [13-RecoveryGrab](FuchimatoiRecoveryCaptures/13-RecoveryGrab.png) |
| 既存Routeへ復帰 | [14-RouteRecovered](FuchimatoiRecoveryCaptures/14-RouteRecovered.png) |
| 岩棚で休息 | [07-SnakeToRock](FuchimatoiRecoveryCaptures/07-SnakeToRock.png) |
| 第二禍根 | [08-Kakon2](FuchimatoiRecoveryCaptures/08-Kakon2.png) |
| 2/3で2回目の落下 | [15-SecondFall](FuchimatoiRecoveryCaptures/15-SecondFall.png) |
| 最終登攀 | [09-FinalClimb](FuchimatoiRecoveryCaptures/09-FinalClimb.png) |
| Victory | [10-Victory](FuchimatoiRecoveryCaptures/10-Victory.png) |

## 未検証と残る弱点

24. 実ゲームパッド機器の操作、全地形・全落下位置、パッケージ版、Shipping版、画面提示時の30/60FPS維持は未検証。本書のFPS指定は固定DeltaSecondsでのシミュレーション検証。追加のオフスクリーン実描画計測では上限付きのフレーム間隔に課題が見つかり、詳細は実時間性能報告に記載。初見の人による理解度・発見までの時間も未計測。
25. 残るGameplayの弱点は3つ。①復帰入口が1か所で、後半の落下では既に通った中盤を登り直す。②蛇の中心線が約2倍に伸び、復帰やRoute接続は直線補間のため身体動作が弱い。③Camera退避は視線確保を優先し、近接時の急な寄りや見下ろしへの変化が残る。
26. 次は初見プレイヤーに触ってもらい、Telemetryと観察で「岩誘導を理解するまで」「落下から入口発見まで」「25Staminaで復帰後に岩棚へ到達できるか」を測る。その結果から入口位置・短文・回避予告時間を小さく調整する。
