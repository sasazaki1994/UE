# 石走りの共通Nushi基盤への統合

基準: `origin/main` の `6eb4d2b`。作業ブランチ: `codex/work_ishibashiri-nushi-integration`。
検証環境: Windows / UE 5.6.1、2026-09-12。

## Before / After

以前は `AIshibashiriBoss : AActor` が `PurifiedCores[3]` とCombat Healthを減らし、
Healthが0になるとローカルのCalmed状態へ移り、GameModeへ直接Victoryを通知していた。
既存のNushi基盤は通常の石走り戦には接続されていなかった。

現在は `AIshibashiriBoss : ANushiBase : AActor`。通常のGameModeがManagerを生成して石走りを登録する。

```text
通常の登攀攻撃（Rest・距離170cm未満・Buck中でない、という条件は維持）
  → 対応する AKakonActor::Purify()
  → OnPurified → UNushiProgressComponent（0/3 → 1/3 → 2/3 → 3/3）
  → OnAllPurified → UNushiStateComponent（Active → Calm）
  → ANushiEncounterManager（Running → Completed）
  → OnEncounterCompleted → GameMode::FinishEncounter(true)
  → 既存Victory表示・StopCombat・演出
```

## 責務

| 所有者 | 責務 |
|---|---|
| IshibashiriBoss | 石走りのAI、Combat Health、浄化可能な距離/Rest/Buck条件、既存の禍根表示とアニメーション選択 |
| KakonActor | 個別のExposed/Purified状態。重複Purifyを拒否 |
| NushiProgressComponent | 登録集合と浄化済み集合、浄化数、全浄化イベントの唯一の管理元 |
| NushiStateComponent | Dormant / Active / Calm。完了状態の唯一の管理元 |
| NushiEncounterManager | Idle / Running / Completed、Start / Reset / Completed通知 |
| GameMode / Player | Player初期化、入力、Retry要求、勝敗画面、Defeat。VictoryはManager通知を表示に反映 |

石走りの `EIshibashiriState::Calmed` は既存の呼び出し側向けの読取結果として残る。
`GetState()` が共通StateのCalmから導出し、AIの保存状態として独立してCalmedを書き込まない。
Healthや独自の浄化数からGameModeへ直接Victoryを通知する経路を削除した。

## 禍根の配置とHealth互換性

3個の `UChildActorComponent` が `AKakonActor` の生成・破棄を所有し、Creatureに追従する。
可視MeshやColliderは追加せず、従来のCoreMarkerと骨の非表示処理をそのまま利用する。
KakonActorにScene Rootを追加し、個別Actorの座標も元の位置と一致させた。

| Index | 禍根 | Creatureローカル座標(cm) |
|---|---|---|
| 0 | 右肩 | `(218, -155, 611)` |
| 1 | 中央頂上 | `(0, 60, 784)` |
| 2 | 背面 | `(-122, 305, 688)` |

石走りには殻を壊す段階がないため、各Kakonの `MaxShellHealth=0` とする。
共通KakonのResetは、殻HPが0ならExposed、正なら従来どおりCoveredへ戻す。
既存の殻ありKakonのテストも維持する。

従来は「反撃3回」「浄化3個」「反撃と浄化の合計3回」で勝利できた。
この操作感・難易度を保つため、浄化1個につきCombat Healthも従来どおり1減る。
反撃や混合経路でHealthが0になった場合は共通Stateの `CalmNushi()` を使い、
同じManager経由で完了する。未浄化Kakonを浄化済みに偽装しない。
一方、全禍根の浄化はHealthが残っていても共通Progressから完了することをテストしている。

## Start / Retry

初回にGameModeが `ConfigureEncounter(Spawn, Player)` と `Manager::SetNushi(Boss)` を行う。
Retry入力は従来のR / Gamepad Yのまま、以下を同期的に一回ずつ実行する。

```text
GameMode::RetryEncounter
  → Player::ResetForEncounter（登攀/入力/HP/Stamina/位置を復元）
  → Manager::ResetEncounter
      → virtual Boss::ResetNushi
          → ANushiBase::ResetNushi
              → NushiState::ResetNushi
                  → NushiProgress::ResetProgress
                      → 登録済みKakonそれぞれのResetKakon
                  → State = Dormant
          → 石走りのHP/AI/位置/禍根表示/既存アニメーションを復元
      → Manager = Idle、Reset通知
  → Manager::StartEncounter
      → State = Active、Manager = Running、Start通知
```

GameModeから石走りを別途Resetする呼び出しはない。KakonをRetryごとに生成・登録し直さない。
イベントは `AddUniqueDynamic` で一度接続し、重複Purify/Start/Calmによる多重完了を防ぐ。
共通Resetを仮想化したため、Managerが基底ポインターを通して石走り固有のResetも実行できる。

## 変更しなかったもの

11地点Route、隣接表、Grab位置、Rest定義、禍根座標、Chase/Telegraph/Charge/Recoveryの数値、
突進方向の固定、命中判定、Counter窓、RiderTime、Buck/Shake、Stamina、Camera、入力割当、Basin。
Content / Art / Tools / Config、およびPlayer、登攀Component、GrabComponent、BasinArena、
Fuchimatoiの実装は `main` と一致する。Material、Texture、Rig、Animation、地形、描画設定は変更していない。

## 自動テストと検証結果

最終結果・RunId・ログは [IshibashiriNushiValidation.json](IshibashiriNushiValidation.json) に記録する。
ログ全文と画像は `Saved/NushiIntegration/`、`Saved/Screenshots/Climbing/` に保存。

- `Nushi.IshibashiriIntegration`: 実際の石走りを生成し、3登録、座標追従、進行、完了、通知回数、Reset、再完了を検証。
  rendererやmapは不要だが、通常のBossコンストラクターは既存のVisual Assetを読み込む。
- `Nushi.ShelllessKakonLifecycle`: ANushiBase / Kakon / Managerだけで2サイクルを検証する、3D Asset非依存のテスト。
- `ClimbingIntegrationTest`: 実際の入力からProgress 1/3、3/3→Calm→Completed→Victory、Retry後0/3と全Kakon Exposed、再クリアを確認。
- `PrototypeSmokeTest`: 地上反撃3回の勝利が共通Completedへ到達し、浄化数は0のままであることを確認。

登攀テストは旧Fixtureの接近方向のみ修正した。変更前mainの固定world-X方向では、
現在のGrab対象方向から約128度ずれ、mainに存在する100度の開始制限に拒否されていた。
変更前mainでも60/30FPSとGamepadで同じ失敗を再現済み。
Fixtureを前脚の外側から内向きに接近させ、Gameplayの角度制限・距離・座標は変更していない。
Retry後の2周目は、入力Retryの直後に追加Resetを挟まず走行する。

| 検証 | 結果 |
|---|---|
| Editor Build | PASS |
| 共通UE Automation | 12 PASS、失敗0、警告0 |
| Pythonの既存テスト | 27 PASS（UE同梱Python、UTF-8モード） |
| 指定のClimbing + Basin 60 / 30FPS | PASS、両方で2回の全浄化・Victory・Retry |
| 指定のClimbingGamepad + Basin 60FPS | PASS、2回の全浄化・Victory・Retry |
| 指定のGrab / LocalClimbing / Camera + Basin 60FPS | 3件すべてPASS |
| Legacy描画付き登攀60FPS | PASS、2回の全浄化とRetry、6画像を生成 |
| HighQuality + Basin描画付き登攀60FPS | PASS、2回の全浄化とRetry、6画像を生成 |
| 通常Gamepadテスト60FPS | PASS |
| 通常Arenaの入力のみのPlaythrough 60FPS | PASS、3回の勝利・Retry |
| BasinScenario 60 / 30FPS | 既存失敗、変更前mainと同じ接近中のFall |
| Playthrough + Basin 60FPS | 既存失敗、変更前mainと同じ1周目14.05秒の被弾 |
| ClimbingIK / GrabMotionWarp | 既存失敗、必要なControlRig / Warp Asset契約が未成立 |

Legacy / HighQualityのVictory画像で、CORES 3/3、CALMED、Victory表示と既存のRetry案内を目視確認。
フレームごとの画素一致や実機の操作感を比較したものではない。

## 未解決・未検証事項

- 上表のBasin自動攻略、IK、MotionWarpの失敗は独立した変更前mainのworktreeでも同じ箇所で再現した。
  BasinScenarioは失敗時のPlayer/Boss座標、Playthroughは時刻・反撃数・回避数まで一致。
  今回はそのGameplay・Assetを変更していない。
- 入力はUE PlayerControllerへの模擬イベント。物理Keyboard/Gamepadの手動操作、UE 5.4/5.5、パッケージ化は未検証。
- 混合勝利は既存Health減少経路と共通State経路を維持し、部分進行のままの完了をAutomationで検証。
  反撃と登攀浄化を組み合わせる専用の入力Playthroughは未実施。
- Editor実行によるDefaultInput.iniの自動正規化は元に戻した。最終Automationは `-nowrite`、
  最終の指定6テストは元の入力設定で実行。入力設定の差分は納品しない。

## 次の淵纏いへ再利用する部分

ANushiBaseのProgress/State、Kakon登録、Kakon→Progress→Calm→Completedのイベント連鎖、
Managerのライフサイクル、virtual ResetNushi、二重通知・再挑戦を検証するAsset非依存テストを再利用できる。
石走りのAI、Healthの減り方、登攀Routeや浄化可能条件を淵纏いへ流用する必要はない。
AFuchimatoiBossは削除・変更していない。

## 変更ファイル

- `Source/IshibashiriPrototype/Public/IshibashiriBoss.h`
- `Source/IshibashiriPrototype/Private/IshibashiriBoss.cpp`
- `Source/IshibashiriPrototype/Public/NushiBase.h`
- `Source/IshibashiriPrototype/Public/NushiStateComponent.h`
- `Source/IshibashiriPrototype/Private/NushiStateComponent.cpp`
- `Source/IshibashiriPrototype/Private/KakonActor.cpp`
- `Source/IshibashiriPrototype/Public/PrototypeGameMode.h`
- `Source/IshibashiriPrototype/Private/PrototypeGameMode.cpp`
- `Source/IshibashiriPrototype/Public/ClimbingIntegrationTest.h`
- `Source/IshibashiriPrototype/Private/ClimbingIntegrationTest.cpp`
- `Source/IshibashiriPrototype/Private/PrototypeSmokeTest.cpp`
- `Source/IshibashiriPrototype/Private/IshibashiriNushiIntegrationTests.cpp`
- `Docs/IshibashiriNushiIntegration.md`
- `Docs/IshibashiriNushiValidation.json`
