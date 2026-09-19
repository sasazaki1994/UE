# 追跡時計・検証ツールの検証記録 — 2026-09-19

## 変更内容

- 石走りの追跡期限を点滅表示用の時計から分離し、追跡中のサブステップだけで進めるようにしました。期限でステップを分割するため、フレームが予告開始をまたいでも余り時間を次の状態へ渡します。Recover / Kneelから次のChaseへ入ると時計をリセットします。
- `PrototypeSmokeTest` の開始時に、60 FPS相当と400ミリ秒刻みの両方で、初回およびRecover後の追跡期限を検査します。
- `Tools/Prototype.ps1 -Action Test` に実時間タイムアウトを追加しました。テスト種別、指定したゲーム内時間、描画時の固定FPSから既定値を決め、`-TestTimeoutSeconds` で上書きできます。期限超過では、その実行で起動したテストプロセスだけを終了します。
- `Play` / `Editor` の非0終了を呼び出し元へ返し、テスト成功時は以前のネイティブ終了コードに影響されず0を返します。`-Onscreen` のウィンドウ表示、各シナリオの引数・成功マーカー・画像検査は維持しています。
- Editor Automationが使用する `FAutomationEditorCommonUtils` のリンク依存をEditorビルドだけに追加しました。最新版mainをこの作業フォルダーでクリーンにビルドした際に発見した既存のリンクエラーを解消するためです。

## 検証結果

| 検証 | 結果・証拠 |
| --- | --- |
| UE 5.6.1 Editor / Gameビルド | 両ターゲットでUHT、コンパイル、リンク成功。UE 5.4互換include順の既存警告あり |
| カメラ・追跡期限 30 FPS | 終了コード0、`PROTOTYPE_TIMING_PASS` / `PROTOTYPE_CAMERA_PASS` / `PROTOTYPE_TEST_PASS`。RunId `d63e21e3069a437e9d15d447e2036593` |
| カメラ・追跡期限 60 FPS・描画 | 終了コード0、3つのPASSマーカー。RunId `0ce55430405f47dfb3b8f95a856a47f2` |
| カメラ・追跡期限 120 FPS | 終了コード0、3つのPASSマーカー。RunId `6eecbf0a80574cc69acd5ce07dd42e47` |
| 描画確認 | `Saved/Screenshots/Prototype/0ce55430405f47dfb3b8f95a856a47f2/` の壁際、ボス遮蔽、通常視点への復帰の3画像を目視確認 |
| PowerShellツールfixture | Windows PowerShell 5.1で23項目成功。正常・非0終了、偽PASS、PASS欠落、期限超過、同名別プロセス保護、各シナリオのマーカーと制限時間、引数保持を確認 |
| Pythonソース検証 | 最新mainへのrebase後、UTF-8モードで100 passed、16 subtests passed |
| Gameplay契約 | `python Tools/GameplayContract.py` で全digest一致 |

自動タイムアウトの非描画・60 FPS時の既定値は、通常とBasinが300秒、Magatsuneが600秒、Climbing / Minedakiが720秒、Fuchimatoiが840秒、Approachが1560秒、Campaignが4920秒です。描画時は固定FPSとの比率を考慮して延長します。

## 制限

- 30 / 60 / 120 FPSの検証は現行の石走りモデルを使った自動カメラ経路です。物理キーボード・マウスによる操作感は評価していません。
- 400ミリ秒刻みの条件検査はAIの追跡期限を対象とし、移動中のプレイヤーと突進の接触時刻の精密な一致を保証するものではありません。
- Windows Developmentパッケージは今回再生成していません。
