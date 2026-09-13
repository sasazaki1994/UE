# 淵纏い Primitive Vertical Slice

基準は `sasazaki1994/UE` の `main`、`52411f55e21ef5d68912457f4a9e96cf89de6cf4`（PR #35統合後）。作業ブランチは `codex/work_fuchimatoi-primitive-slice`。

噛みつきを岩へ誘導し、回避して生まれたGrab機会から登り、第一禍根で身体を巻き付かせ、蛇と岩棚・岩柱を行き来して残りの禍根を祓う。共通Nushiの完了をVictoryへ接続し、入力Retryから再クリアまで実装した。

## 起動と操作

```powershell
.\Tools\Prototype.ps1 -Action Play -Fuchimatoi
.\Tools\Prototype.ps1 -Action Test -Fuchimatoi -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Fuchimatoi -SkipBuild -TestFPS 30
.\Tools\Prototype.ps1 -Action Test -Fuchimatoi -Gamepad -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Fuchimatoi -Capture -SkipBuild -TestFPS 60
```

`-Fuchimatoi` は既存マップの起動URLに専用GameModeを指定する。通常の起動URLと石走りGameModeは維持。マップ・入力設定・モデル資産の変更は不要。

| 操作 | キーボード・マウス | ゲームパッド |
|---|---|---|
| 地上移動 | WASD | 左スティック |
| カメラ | マウス | 右スティック |
| 横回避 | 移動方向 + Shift | 移動方向 + B |
| 頭にGrab | E | RB |
| 隣接Routeへ前進／後退 | W / S | 左スティック上下 |
| 禍根を祓う | 左クリック | X |
| Jump／Detach | Space | A |
| Retry（攻略途中でも可能） | R | Y |

金色の床印で予告を待ち、頭が迫ったら横へ回避する。岩へ衝突した頭へ近づいてGrab。赤い禍根のNodeで停止して祓い、巻き付きが終わったら進む。金色マーカーの岩棚と岩柱では入力を止めてStaminaを回復する。登攀中の接続線は蛇が水色、岩への移動が黄色。

## 要求28項目への報告

| # | 項目 | 実装・結果 |
|---|---|---|
| 1 | 変更ファイル | 既存は `FuchimatoiBoss.h/.cpp` と `Tools/Prototype.ps1`。新規ファイル一覧は下記。石走り本体、11地点Route、共通Grab/Stamina/Nushi、Config/Content/Artには内容差分なし。 |
| 2 | 新規クラス | `AFuchimatoiArena`、`AFuchimatoiPlayer`、`AFuchimatoiGameMode`、`AFuchimatoiHUD`、`AFuchimatoiRouteAnchor`、`UFuchimatoiSimulationComponent`、`AFuchimatoiIntegrationTest`。既存 `AFuchimatoiBoss` を拡張。 |
| 3 | Arena | 64m × 40m床、26m × 23m青い水場、誘導岩、岩棚、岩柱、最終棚、渓谷壁、誘導床印。Cubeのみ。専用照明を配置。 |
| 4 | Primitive | 頭・身体接合部・尾のSphere計11、接続Cylinder10、目Sphere2、Route Marker10、Kakon Marker3。プレイヤーもSphere2とCharacter Capsule。Engine BasicShapeMaterialの色パラメータを使用。 |
| 5 | Boss全長 | 初期の中心線長27.28m。巻き付き後は配置優先の補間により54.61mとなる。長さ保存や物理蛇は実装していない。 |
| 6 | Action State | Submerged → BiteWindup → BiteLunge → 指定岩ならSnagged → 第一禍根でCoiling。外れ・被弾・未Grabの時間切れはSubmergedへ戻る。Coiling完了後は既存APIどおりActionを維持し、共通StateのCalmで停止。 |
| 7 | Bite Target | Windup終了時のPlayer XYを頭の高さへ投影し、その方向へ750cm延長した終点を既存ローカルTargetに一度だけ保存。Lunge中に追尾更新しない。Windupは編集可能な1.25秒、速度は既存1200cm/s。 |
| 8 | Rock衝突 | 頭の移動区間を半径105cmのSphereでWorldStaticへSweep。指定BaitRockコンポーネントへの命中だけが `NotifyHeadSnagged()` を呼ぶ。岩までの区間にPlayerがいて回避中でなければ1ダメージ＋Knockbackとなり、Grab機会は発生しない。 |
| 9 | Snagged | 頭を接触位置に固定。編集可能な4.5秒の新規Grab窓。成功後は時間切れで解除せず登攀を継続する。 |
| 10 | Grab再利用 | 既存 `UGrabComponent::TryGrab` と相対Transformを利用。Snagged・距離440cm・Stamina25以上・Grab入力を条件に専用頭Anchorへ接続。共有コンポーネントは無変更。 |
| 11 | Climbing再利用 | 既存Grabの追従とReleaseを利用し、専用Pawnに隣接Node間の補間を実装。石走りの登攀処理やRouteデータはコピーしていない。自由登攀用 `Grab::Climb` は使わない。 |
| 12 | Route数 | 10。Phase 1は0〜3、巻き付き完了後は0〜9。表は下記。 |
| 13 | Snake→Rock | 隣接Node間を300cm/sで補間し、到着時に共有GrabをRelease→次のAnchorへTryGrab。蛇Anchorの親はBoss、岩Anchorの親はArenaなので、所属先が実際に変わる。 |
| 14 | Coiling | 既存 `BeginCoiling` / `AdvanceCoiling` / `CoilingProgress` を使用。2秒間でBodyとSnake Anchorを配置間で線形補間。移動を一時停止し6Stamina/sを消費、枯渇時は落下。岩Anchorは固定。移動Component→Boss Simulation→Pawn→GrabのTick順序で追従。 |
| 15 | Kakon位置 | 第一は首から上胴のNode3、第二は巻き付き胴Node7、第三は高所の頭頂Node9。第一は最初から、第二・第三はCoiling完了後に操作可能。攻撃時は同一Nodeで停止し距離150cm以内を確認。 |
| 16 | Progress | 3個の既存 `AKakonActor` を `ANushiBase::RegisterKakon` で登録。浄化状態の独自配列を持たず `UNushiProgressComponent` が0/3〜3/3を管理。 |
| 17 | State | 共通Progressが全浄化を通知し `UNushiStateComponent` がCalmへ移行。Boss固有の完了フラグは追加しない。 |
| 18 | Manager | `ANushiEncounterManager` がCalmを観測してCompleted。専用GameModeはManagerの `OnEncounterCompleted` のみ購読し操作を止める。HUDのVictoryもManagerから導出。BossからVictoryを直接呼ばない。 |
| 19 | Retry | Pawnの入力・Health・Stamina・Grab・移動・カメラを初期化。ManagerのReset→StartがBossの仮想Resetを経由し、Transform、Action、Target、Head、Coil、Route、3Kakon、Progress、Stateを復元。同じActor群を再利用し、2回のRetryでActor数増加なしを確認。 |
| 20 | Automation | `IshibashiriPrototype.` 全13件PASS、失敗0、テスト内警告0。既存ActionLifecycle / BiteLungeMovement / CoilingProgressを維持し、新規PrimitiveIntegrationで配置・共有進行・Resetを検証。既存Python27件もPASS。 |
| 21 | 入力Playthrough | `AFuchimatoiIntegrationTest` がPlayerControllerへキー／スティック入力を送る。位置変更・状態変更・Purify直接呼び出しで攻略を代替しない。各実行で2周クリアとRetry、最後に意図的な被弾と再Resetを確認。 |
| 22 | 60 FPS | キーボード＋描画Capture、ゲームパッドともPASS。Target固定、岩命中、Grab、登攀、Coil追従、岩での回復、3禍根、Calm、Completed、Victory、Retryを確認。 |
| 23 | 30 FPS | キーボードで同一攻略PASS。BiteとCoilは最大1/120秒の内部刻み、RouteはDeltaSecondsで進行。固定シミュレーションFPSであり実GPU性能の保証ではない。 |
| 24 | 石走りRegression | 指定のClimbing Basin60、ClimbingGamepad Basin60、Grab60、Camera Basin60が全PASS。石走りのコード・Route・Camera・Basin/Legacy/HighQuality資産は変更なし。 |
| 25 | Screenshot | `Saved/Screenshots/Fuchimatoi/<RunId>/` に10枚。最終RunId・各ファイル・SHA256は `Docs/FuchimatoiValidation.json` に記録。 |
| 26 | 未実装 | 完成モデル、Skeletal/Final Animation、本格Water、Niagara、MetaSounds、Landscape/PCG、本制作Rig/IK、Motion Matching、GAS、StateTree、物理蛇、自由登攀、峰抱き、物語演出。通常パッケージ化・全カメラ角度・全失敗経路の網羅試験も未実施。 |
| 27 | Gameplayの弱点3つ | 下記の3点。 |
| 28 | 次の改善 | まず初見プレイヤーの誘導位置・回避タイミング・Grab成功率を計測し予告と岩配置を調整。次に蛇の長さを保つ配置と接続距離を調整。最後に落下後の再接続経路を追加する。 |

## RouteとStamina

Node番号は内部0始まり、HUDは1〜10表示。座標単位cm。蛇NodeはBody中心からZ+178、KakonはNodeからZ-30。

| Node | 所属・役割 | Coiling後のAnchor世界座標 |
|---|---|---|
| 0 | 蛇・Grab入口の頭 | (-120, -280, 2000) |
| 1 | 蛇・首 | (250, -260, 1780) |
| 2 | 蛇・上胴 | (430, 100, 1550) |
| 3 | 蛇・第一禍根 | (-620, 80, 848) |
| 4 | 岩棚・休憩 | (-450, 400, 950) |
| 5 | 蛇・中胴 | (-120, 570, 1150) |
| 6 | 岩柱・休憩 | (220, 430, 1350) |
| 7 | 蛇・第二禍根 | (430, 100, 1550) |
| 8 | 蛇・上部首 | (250, -260, 1780) |
| 9 | 蛇・第三禍根 | (-120, -280, 2000) |

Phase 1の頭はBiteに伴い移動する。蛇では静止3/s、移動7/s、Coiling中6/s消費。岩では静止28/s、地上では18/s回復。上限100、既存 `UStaminaComponent` を利用。蛇Node0/9、1/8、2/7は同じ身体部位をPhase別の役割として使う。

## 残るGameplayの弱点

1. 誘導岩が1個で攻略ルートも一本道。慣れると毎回同じ待機・回避・前進になり、経路選択や攻撃の変化が少ない。
2. Coilingは配置の線形補間で中心線長が伸びる。蛇から岩への接続も直線で、手足接触や飛び移り動作がないため、移動の身体感覚は弱い。初期視点は岩による遮蔽を減らしたが、手動で岩の背後へ回したカメラには遮蔽が残る。
3. 巻き付き完了後に落下すると、新規GrabはSnagged限定のため地上から再登攀できない。R/Yで遭遇をやり直す必要がある。途中復帰経路・チェックポイントは未実装。

## ファイル構成

既存変更：

- `Source/IshibashiriPrototype/Public/FuchimatoiBoss.h`
- `Source/IshibashiriPrototype/Private/FuchimatoiBoss.cpp`
- `Tools/Prototype.ps1`

新規（各クラスは `Source/IshibashiriPrototype/Public/<名前>.h` と `Private/<名前>.cpp`）：

- `FuchimatoiArena`
- `FuchimatoiPlayer`
- `FuchimatoiGameMode`
- `FuchimatoiHUD`
- `FuchimatoiRouteAnchor`
- `FuchimatoiSimulationComponent`
- `FuchimatoiIntegrationTest`
- `Source/IshibashiriPrototype/Private/FuchimatoiPrimitiveTests.cpp`
- `Docs/FuchimatoiVerticalSlice.md`
- `Docs/FuchimatoiValidation.json`
- `Docs/FuchimatoiCaptures/*.png`（10枚、最終60 FPS実行の複製）

## PRで確認できるキャプチャ

最終Capture RunId: `4b6d6794206947f6afe4d3903ce2e663`。

| 場面 | 画像 |
|---|---|
| BiteWindup | [01-BiteWindup](FuchimatoiCaptures/01-BiteWindup.png) |
| BiteLunge | [02-BiteLunge](FuchimatoiCaptures/02-BiteLunge.png) |
| Snagged | [03-Snagged](FuchimatoiCaptures/03-Snagged.png) |
| Head Grab | [04-HeadGrab](FuchimatoiCaptures/04-HeadGrab.png) |
| Kakon 1到達 | [05-Kakon1](FuchimatoiCaptures/05-Kakon1.png) |
| Coiling中 | [06-Coiling](FuchimatoiCaptures/06-Coiling.png) |
| Snake→Rock | [07-SnakeToRock](FuchimatoiCaptures/07-SnakeToRock.png) |
| Kakon 2到達 | [08-Kakon2](FuchimatoiCaptures/08-Kakon2.png) |
| Final climb | [09-FinalClimb](FuchimatoiCaptures/09-FinalClimb.png) |
| Kakon 3浄化後・Victory | [10-Victory](FuchimatoiCaptures/10-Victory.png) |

検証の生ログは `Saved/FuchimatoiValidation/`。Automationの13件PASSは `Automation/index.json` で確認できる。エンジン起動時には以前の検証ログにも存在する `CR_Shirotsura_Climbing` 未配置エラーと、Automation起動時の `Condition failed` 記録が残る。今回のテスト結果はエラー文字列の単純検索ではなく各RunIdのPASSとAutomationレポートで判定した。
