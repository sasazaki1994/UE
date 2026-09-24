# 禍祓い Campaign Flow (STEP 3B foundation / STEP 3C E2E)

> **現況注記（2026-09-24）:** 以下の `31c95dd...` は本資料作成時の実装基準であり、現在 main の基準ではない。現在の統合ソース、素材、UE 未検証項目は [main 開発状況監査](MainStatusAudit-2026-09-24.md) を参照する。静的 source-contract の PASS を Campaign の UE 動作確認とは扱わない。

## 目的と構成

基準は `31c95dd1c8e0e5a48b3d77eec12d31994fddb33f`。既存4戦の内部Gameplayを変更せず、プロセス内だけ保持する薄い `UCampaignGameInstance` で接続する。

`Title → Prologue → IshibashiriApproach → Ishibashiri → Interlude1 → Fuchimatoi → Interlude2 → Minedaki → Interlude3 → Magatsune → Ending → Completed`

状態は上記11個だけで、Kakon数、Boss phase、Recovery、RetryをCampaignへ複製しない。保存対象は再開用の章境界だけで、主戦の途中は章の先頭からやり直す。

## 起動とChapter遷移

```powershell
.\Tools\Prototype.ps1 -Action Play -Campaign
.\Tools\Prototype.ps1 -Action Test -Campaign -SkipBuild -TestFPS 60
```

全Chapterは既存 `L_Prototype_01` を使用し、`UGameplayStatics::OpenLevel` の `game=` URL optionで専用GameModeを選ぶ。石走りは `PrototypeGameMode`、残りは各Encounterの既存GameMode、カード画面だけ `CampaignGameMode` を使う。巨大な統合GameModeや本番Mapは作らない。

## Completed検知、Result、Retry

各GameModeは自身の `ANushiEncounterManager::OnEncounterCompleted` を検知する。Completed後は既存Victory HUDを2秒表示してから、Campaignへ現在Encounterと一致する完了だけを通知する。通知先は次のInterlude（禍津根のみEnding）。別Encounterや重複通知は拒否される。

R / Yは従来どおり各GameModeの `RetryEncounter` だけを呼ぶ。Campaignへの通知がないためChapterは進まず、Progress、Grab/Cling、Stamina、camera、telemetryを既存Reset処理で初期化する。

## カード仕様

- Title: `禍祓い` とSTART。
- Prologue: 白面の祓い手、破壊された境界石、左腕の禍、主の禍を鎮める使命を4枚で示す。
- Interlude: 黒背景、各1〜3枚。次の主を示し、禍津根を「第四の主」とは表記しない。
- Ending: 禍津根は鎮まったこと、主が生存すること、左腕に禍が残ること、再発の可能性を4枚で示す。終了後Completedのタイトルへ戻り、再STARTできる。
- Space / X（既存Jump / Attack mapping）でカードを進める。石走り開始時だけ既存HUDに `Q / LT: 境断ち` と `F / LB: 左腕` を表示する。
- TitleでSpace / Xは新規開始。保存がなければ1回、保存があれば上書き警告後の2回目で開始する。確認中はEsc / Bで取消し、R / Yで前回の章の先頭からContinueできる。Endingを読み終えると保存を消去する。

## 保存と再開

`UCampaignSaveGame` は版数と `ECampaignState` の章境界だけを `MagabaraiCampaign` slotへ保存する。カード送り、接近章の出口、Encounter完了時に更新し、現在の章でRetryしても保存状態は変えない。再開は現在章のGameModeを通常どおり生成するため、禍根、体力、スタミナ、Grab、Senseを初期値へ戻す。TitleとCompleted、不明な版数・章は再開候補にしない。`-CampaignE2E` と実行固有 `-PrototypeTestRun` がある自動攻略では保存を読み書きしない。

## Sense Resetと遷移安全性

Travel直前に現在Pawnの `UPlayerSenseComponent::ResetSense` を呼び、Boundary/Corruption hold、reading、warning、risk tailを消す。次Encounterの既存 `ResetForEncounter` も同じResetを行う。OpenLevelは旧Worldを破棄するため、旧Boss/HUD/Manager、Grab/Cling、camera、Retry、telemetry、delegateは次Worldへ持ち越されない。GameInstanceが保持するのはStateとCampaign有効フラグだけである。

## 単体Encounter互換性

通常石走り、`-Fuchimatoi`、`-Minedaki`、`-Magatsune` は同じMap/GameMode/Retryを維持する。GameModeの完了ハンドラは `IsCurrentEncounter` がfalseならTravelしないため、単体プレイ結果は変わらない。

## Automation

`IshibashiriPrototype.Campaign` のState fixtureは引き続きState責務だけを検証する。STEP 3Cの `Prototype.ps1 -Action Test -Campaign` はそれとは別に、カードと既存Encounter DriverのPlayerController入力を連結する。acceptance specは `Specs/CampaignFlow.feature`、実行可否と結果は `Docs/CampaignE2EValidation.md`。

## Validation（当時の記録）

このLinux環境にはWindows UE 5.6.1 Runtimeがないため、C++ Build、Automation、入力Smoke、既存Encounter regressionは **NOT_RUN**。Python source-contractテストと静的検査のみ実施する。詳細は `CampaignValidation.json`。

## 現在も未実装または未確認

- Encounter途中のCheckpoint、Profile、複数Save Slot。
- 本番Map、探索、村、NPC、音声、Sequencer、BGM、完成アート。
- 章単位の保存・再起動・破損Save・既存Campaign回帰のWindows UE 5.6.1検証は未実行。
