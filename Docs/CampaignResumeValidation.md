# 章単位セーブ・再開の検証

## 仕様

- TitleのSpace / Xは新規開始。保存がなければ1回で開始し、保存があれば1回目は上書き警告だけを表示し、2回目で開始・保存する。
- 上書き確認はTitle GameModeだけの一時状態。Esc / Bで取消し、確認中もR / Yで保存された章の先頭へContinueできる。最初の入力、取消、Continueは保存を変更しない。
- 保存点はPrologue、接近章、各Encounter開始、幕間、Ending。禍根や主のPhase、スタミナ、落下復帰位置は保存しない。
- Retryは章や保存点を進めない。Endingを最後まで読むとContinueを消す。
- 保存の削除に失敗してスロットが残った場合は警告を記録し、再読込した再開地点を保持する。削除成功またはスロットが存在しない場合だけContinueを消す。
- `-CampaignE2E` と `-PrototypeTestRun` は通常の保存領域を変更しない。

## Windows UE 5.6.1での実行項目

1. `Tools/Prototype.ps1 -Action Build` とAutomation `IshibashiriPrototype.Campaign` を実行し、`SaveRoundTrip`を含めてPASSを確認する。
2. `Tools/Prototype.ps1 -Action Play -Campaign` で接近章へ進み、アプリを終了して再起動する。TitleのContinueを選び、接近章の入口から始まることを確認する。
3. 石走りの禍根を1/3まで祓った後に終了してContinueし、石走りの章先頭と禍根0/3から始まることを確認する。Retry後も同じ保存点であることを確認する。
4. 淵纏いを鎮めた後のInterlude2からContinueし、白面の段階がAdvancedであることを確認する。
5. 保存がある状態でCampaign E2Eを実行し、再起動後のContinue地点が変わらないことを確認する。
6. 保存なしでは1回のNEW GAME入力で開始する。保存ありでは1回目に警告だけが表示され、保存が不変であることを確認する。Esc / Bで取消して保存が不変であること、確認中のR / YでContinueできること、二度目のNEW GAMEで初めてPrologueを保存することをキーボードとゲームパッドで確認する。
   Prologue保存ログは1回だけで、保存失敗時は従来の`CAMPAIGN_SAVE_FAILED`処理であることも確認する。Ending読了後にContinueが消えること、削除失敗時は警告と再開地点を保持することを確認する。
7. 3回連続の全章Campaign入力攻略、30 / 60 FPS設定でのRetry・落下復帰・Sense resetを実行する。別に実描画の平均 / p95 / p99フレーム時間を測定し、固定タイムステップの攻略PASSを実FPS保証として扱わない。

## 現在の結果

Python source-contractテストは実行済み。UE 5.6.1ビルド、SaveGame実シリアライズ、アプリ再起動、人の操作、3回の長時間攻略と実時間性能は、この環境にUEがないため **NOT_RUN**。実機で確認後にRunIdとログ、Capture、frame timingを追記する。
