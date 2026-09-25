# Ishibashiri Demo Flow

## 目的と起動

`-IshibashiriDemo` は、既存の4戦Campaignを縮小せずに石走り編だけを最初から最後まで確認する、明示的で非永続なモードである。既存GameInstance、カード画面、Approach、`PrototypeGameMode` とEncounter driverを再利用し、第二のCampaign frameworkは作らない。

```powershell
.\Tools\Prototype.ps1 -Action Play -IshibashiriDemo
.\Tools\Prototype.ps1 -Action Test -IshibashiriDemo -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -IshibashiriDemo -Capture -TestFPS 60
```

## 状態遷移と結末

```text
Title → Prologue（既存4枚）→ IshibashiriApproach → Ishibashiri
      → Interlude1冒頭2枚 → Title
```

石走りの3禍根、Calm、Encounter Completedを経た後だけInterlude1へ入る。デモでは「石走りは生きている。息が戻る。」「水の底で、同じ脈動が続いている。」を表示し、「第二の主　淵纏い」は表示しない。2枚目の確定でTitleへ戻り、`ISHIBASHIRI_DEMO_COMPLETE` を一度記録する。Fuchimatoi、全編Ending、Completedおよび仮面を外す結末は通らない。

通常の `-Campaign` は `Interlude1` 全3枚からFuchimatoi、Minedaki、Magatsune、Ending、Completedへ進む。既存のSave/Continue、カード本文、SaveGame versionおよび各Encounter gameplayは変更しない。

## Save隔離

Demoではpersistenceを初期化時から無効にする。このため `MagabaraiCampaign` をロード、保存、削除せず、Titleの上書き確認とContinueも提示しない。通常Campaignだけが従来どおり同スロットを使用する。自動テスト用 `PrototypeTestRun` の既存隔離も維持する。

## E2EとCapture契約

Test runnerは既存Campaign E2Eの入力driver（Approachと石走り攻略）を使い、`ISHIBASHIRI_DEMO_E2E_PASS <RunId>` を要求する。`-Capture` の保存候補は `Saved/Screenshots/IshibashiriDemo/<RunId>/` で、命名契約は次のとおり。

1. `01-Title.png`
2. `02-Prologue.png`
3. `03-Approach.png`
4. `04-IshibashiriReveal.png`
5. `05-Charge.png`
6. `06-GroundGrab.png`
7. `07-Climbing.png`
8. `08-Kakon1.png`
9. `09-Kakon2.png`
10. `10-Kakon3.png`
11. `11-Calm.png`
12. `12-DemoEnding1.png`
13. `13-DemoEnding2.png`
14. `14-ReturnToTitle.png`

runnerとカード側には専用保存先、終了カード、Title復帰の取得処理を用意した。Approachおよび戦闘中の全候補を実描画で取得できるか、画像が意味する瞬間と一致するかはWindows実機ゲートで確認する。今回PNGを生成済みとは扱わない。

## 検証状態と未検証事項

- Python source-contract / Narrative contract / Gameplay contract: Linux上で実行する。
- UE Build / UHT / Runtime / Visual / Input / Screenshot: **NOT_RUN — WINDOWS UE 5.6.1 REQUIRED**。
- 実機では最初に、保存済み通常Campaignが不変であること、Titleからの全入力、3/3→Calm→Completed→2カード→Titleの順序、ログが1回だけであること、Fuchimatoiがspawnしないことを確認する。
- Captureは上記14場面の存在、内容、順序を同じRunIdで確認する。静的検査は画像の実在や視覚品質を証明しない。
