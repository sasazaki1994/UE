# 峰抱き — 完成Primitive主戦

基準コミット `01b7cbf017ca98c7b8bfaa21acbe91eb0a68dae6`（PR #40 merge済み）の3/3実装を、既存の Grab / Cling / Stamina / Kakon / Nushi Progress / State / EncounterManager / Retryだけを対象に静的監査した。境断ち、左腕能力、札、GAS等は導入していない。

## 起動と最終フロー

```powershell
.\Tools\Prototype.ps1 -Action Play -Minedaki
```

`Ground → 左脚Grab → 背中 → 20m崖登り → Phase 1 Shake → 上段 → 禍根1 → 左腕を掛ける身体変形 → 腕橋 → 禍根2 → 反対腕と上体を掛け直す身体変形 → 肩/首 → 最終Cling → 禍根3 → Calm → Encounter Completed → Victory → R/Y Retry`。

WASD/左スティックで移動、E/RBでGrabおよびCling、Space/Aで離脱、左クリック/Xで露出禍根を祓う。身体変形中は移動せずE/RBを保持し、緑の次Routeが現れてから進む。

## Phase / Route / Anchor

| Phase | Node | 構成 | 状態変化 |
|---|---:|---|---|
| 1 | 1–8 | 左脚、腿、腰、背中、上段、肩 | 既存Pitch 70° / Yaw 20° / Roll 24°と相対Transform追従を維持 |
| 2 | 8–11 | 背中上部から左肩、上腕、前腕 | 禍根1後にBodyRootがPitch -12° / Yaw 18° / Roll -20°、左腕が橋へ変形。変形完了までNode 9以降は無効 |
| 3 | 12–14 | 反対肩、首、頭頂 | 禍根2後にPitch 28° / Yaw -16° / Roll 30°、右腕を掛け直す。完了まで最終Routeは無効 |

禍根1は肩甲骨上部(Node 7)、禍根2は左前腕先端(Node 11)、禍根3は頭頂(Node 14)。マーカー色と大きく離れた位置で視覚・Gameplay上を区別する。各変形は固定Nodeの単純表示ではなくBodyRootと腕Primitiveを補間し、完了時だけ次Anchorを有効にする。

## Cling / Stamina

Phase 1は横Shake、Phase 2は左腕を掛ける傾斜、Phase 3は頭部を持ち上げる大姿勢変更で同じE/RB Clingを再利用する。判定時に保持していない、またはShakeCost以下なら落下する。安全休息点はPhase 1のNode 5、Phase 2開始のNode 8、Phase 3開始のNode 12。既存 `UStaminaComponent` は無変更で、峰抱きPlayer側のDrain/Restoreだけを使う。Recovery再Grab後は2秒の消費猶予がある。

## Fall Recovery

0/3は地上、1/3は上段左の小棚、2/3は上段右の小棚へ着地後にRecoveryを開始する。浄化済みKakon、Progress、Nushi Active、Encounter Running、開通済みPhaseをResetしない。棚上で少し移動してRecovery Anchorへ近づき、既存 `UGrabComponent::TryGrab` でBodyGrabFrameへ再接続するため、攻略を飛ばす専用Teleport操作ではない。R/Yは従来通り全Resetする。

## Calm / Completed / Victory / Retry

3つの `AKakonActor` は正式なPlayable対象として登録される。3個目の `Purify` が `UNushiProgressComponent::OnAllPurified`、`UNushiStateComponent` のCalm、`ANushiEncounterManager` のCompletedを発火し、HUDはCompletedをVictoryとして表示する。独自Victory/Completed/進行配列はない。Calming中はBodyRootを2秒かけて静かな姿勢へ戻す。

Retryは既存Actorを再Spawnせず、Player、Grab、Stamina、Boss/Body transform、Action、Route、3 Kakon、共通Progress/State/Encounter、RecoveryとCamera入力を初期化する。

## Telemetry

`MINEDAKI_TELEMETRY` はGrab試行/成功、各Phase開始/完了、Kakon 1–3、Body-route transition、Cling秒、Shake成功/失敗、Fall、Recovery開始/成功、Stamina exhaustion、Calm、Encounter Completed/Victory、Retry、clear timeを同じ軽量ログへ記録する。

## Automation / Input Playthrough

追加Acceptanceは `Specs/Acceptance/MinedakiFullEncounter.feature`。Automationは `FullEncounterLifecycle`、`BodyRouteTransition`、`FallRecovery`、`RetryReset` を追加し、既存 `ActionLifecycle`、`LocalTransformFollow`、`ExhaustionRetry` を残す。Integration driverはPlayerControllerのKeyboard/模擬Gamepad入力だけで、60 FPS 2周、30 FPS、Gamepad 60 FPS、1/3落下RecoveryからVictoryを通る。位置、Stamina、Boss Phase、Kakon、Progressを攻略目的で直接変更しない。

## 検証記録（この変更環境）

| Check | Command | Result |
|---|---|---|
| 静的差分検査 | `git diff --check` | PASS |
| UHT / Editor Development | `.\Tools\Prototype.ps1 -Action Build` | 未検証: このLinuxコンテナにWindows UE 5.6.1なし |
| Nushi/Minedaki Automation | UnrealEditor-Cmd `Automation RunTests IshibashiriPrototype.Nushi` | 未検証: 同上 |
| Keyboard 60/30, Gamepad 60, Capture | `Prototype.ps1 -Action Test -Minedaki ...` | 未検証: 同上 |
| 石走り / 淵纏いRegression | 指定の各 `Prototype.ps1 -Action Test` | 未検証: 同上 |

既存の `Docs/MinedakiCaptures/` は旧1/3 Runの画像であり、実行していない新Runを装って置換していない。UE 5.6.1環境で `-Capture` を実行すると、Toolsが要求する16枚（Ground Grab、Phase1 wall climb、First Shake、Kakon1、Phase2 transition、Arm route、Kakon2、Phase3 transition、Final Route、Final Cling、Kakon3、Calm、Victory、Fall、Recovery、Retry）を検証する。Capture要求を同一frameで上書きしないよう、Calm/Victory/RetryおよびFinal Routeを別frameに分離した。

## 未検証・残る弱点

未検証はUHT/Win64 build、Automation実行、実描画、物理Gamepad、人間の連続操作、カメラ全方位遮蔽、Regression、15枚の新Capture。

1. Primitive腕は伸縮する一本形状で、関節接地の説得力が弱い。
2. Route判断は「変形を待ち、開いた緑Nodeを進む」一段階で、リプレイ時の分岐はない。
3. Recoveryは安全棚への復帰を明確化するため着地後に位置補正し、棚から再Grabするが、落下軌道そのものを連続的に誘導する表現は未完成。

次はUE 5.6.1でUHT/Editor buildと全Automationを先に通し、入力driverのタイミング、Recovery棚への着地、Camera遮蔽を確認する。その後16 Captureと指定Regressionを取得し、人間操作でCling予告とStamina余裕を確認する。これらが未実行のため、現時点の判定は `STEP 1 NOT VALIDATED` とする。
