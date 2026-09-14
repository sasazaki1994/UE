# 禍祓い Campaign Flow (STEP 3B)

## 目的と構成

基準は `31c95dd1c8e0e5a48b3d77eec12d31994fddb33f`。既存4戦の内部Gameplayを変更せず、プロセス内だけ保持する薄い `UCampaignGameInstance` で接続する。

`Title → Prologue → Ishibashiri → Interlude1 → Fuchimatoi → Interlude2 → Minedaki → Interlude3 → Magatsune → Ending → Completed`

状態は上記11個だけで、Kakon数、Boss phase、Recovery、Retry、SaveGameをCampaignへ複製しない。

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

## Sense Resetと遷移安全性

Travel直前に現在Pawnの `UPlayerSenseComponent::ResetSense` を呼び、Boundary/Corruption hold、reading、warning、risk tailを消す。次Encounterの既存 `ResetForEncounter` も同じResetを行う。OpenLevelは旧Worldを破棄するため、旧Boss/HUD/Manager、Grab/Cling、camera、Retry、telemetry、delegateは次Worldへ持ち越されない。GameInstanceが保持するのはStateとCampaign有効フラグだけである。

## 単体Encounter互換性

通常石走り、`-Fuchimatoi`、`-Minedaki`、`-Magatsune` は同じMap/GameMode/Retryを維持する。GameModeの完了ハンドラは `IsCurrentEncounter` がfalseならTravelしないため、単体プレイ結果は変わらない。

## Automation

`IshibashiriPrototype.Campaign` に `CampaignOrder`、`EncounterCompletionAdvance`、`RetryDoesNotAdvance`、`SenseResetBetweenChapters`、`FullCampaignStateLifecycle` を追加した。テストは明示的なCompleted fixtureでState責務だけを検証し、Kakonや通常Gameplayを自動浄化しない。acceptance specは `Specs/CampaignFlow.feature`。

## Validation

このLinux環境にはWindows UE 5.6.1 Runtimeがないため、C++ Build、Automation、入力Smoke、既存Encounter regressionは **NOT_RUN**。Python source-contractテストと静的検査のみ実施する。詳細は `CampaignValidation.json`。

## 未実装

- Save/Continue、Checkpoint、Profile。
- 本番Map、探索、村、NPC、音声、Sequencer、BGM、完成アート。
- 4戦を実入力で攻略するCampaign E2E（State fixtureのみ）。
