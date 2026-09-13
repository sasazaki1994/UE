# 淵纏い：実時間の描画性能検証

落下復帰の実装に続き、固定DeltaSecondsでの攻略検証と実際のフレーム時間を分けて計測する `-Realtime` を追加した。Boss、Grab、Camera、StaminaのGameplay実装は今回の追加計測では変更していない。

## 計測条件

- UE 5.6.1 Editor Development、D3D11、NVIDIA GeForce RTX 4060 Laptop GPU。
- 1280×800、既存Prototypeの標準品質、オフスクリーン実描画。NullRHIではない。
- Windows電源はAC接続、Battery Saver無効、Turboプラン。ほかのアプリの負荷は制御していない。
- `FApp::SetUseFixedTimeStep(false)` にし、`GEngine::SetMaxFPS` で上限だけを指定。壁時計のフレーム間隔を計測する。
- 開始2秒経過後から終了までのPostUpdate Tick間隔を保存。平均FPSはフレーム数÷計測秒、P95/P99はnearest-rankで集計。
- スクリーンショットのファイル書き込みと、Cameraの72角度検証は計測から除外。通常の描画・移動・戦闘・Telemetry・Retryは含む。
- `over_budget_plus_1ms` は設定上限のフレーム時間+1msを超えた数、`over_50ms` は50ms超の数。攻略のPASSと性能目標の達成は別判定。

## 結果

全6実行で攻略はPASS。Recoveryは同一戦闘内の2回の落下復帰、空振り・被弾・Grab時間切れからの再挑戦、Victoryを含む。追加後の従来の固定60FPS Recoveryも再実行してPASSを確認した。

| ケース | FPS上限 | 平均FPS | P95 ms | P99 ms | 最大 ms | 50ms超 |
|---|---:|---:|---:|---:|---:|---:|
| 通常・Keyboard | 60 | 45.65 | 30.284 | 33.938 | 39.006 | 0 |
| 通常・Keyboard | 30 | 26.69 | 46.818 | 49.278 | 55.395 | 10 |
| Recovery・Keyboard | 60 | 48.94 | 29.599 | 31.827 | 35.377 | 0 |
| Recovery・Keyboard | 30 | 26.54 | 46.964 | 49.237 | 56.179 | 21 |
| Recovery・Gamepadシミュレーション | 60 | 48.77 | 29.621 | 32.121 | 37.854 | 0 |
| Recovery・Keyboard・上限解除 | なし | 239.83 | 5.982 | 6.553 | 25.077 | 0 |

上限付きのオフスクリーン計測は、設定した30/60FPSを維持できていない。一方、同じ解像度・同じ攻略の上限解除では平均239.83FPS、P99 6.553msであり、この環境には描画処理の余裕がある。比較結果から、上限待機またはオフスクリーン時のフレーム間隔制御の影響が大きいと推測する。GPU処理不足や今回のGameplay変更による性能低下と断定できる結果ではない。

Unrealの上限処理は実フレーム間隔から待ち時間を求めてSleepする。今回のフレーム間隔だけでは、その待機とOSのスケジューリングを分離できないため、根本原因は未確定。上限解除の測定だけで画面提示時の60FPS保証とはしない。

通常ウィンドウでの追加測定を下記に記録する。上限待機時間とGame/Render/GPU時間の分離、およびパッケージ版の確認は未実施。

## 通常ウィンドウでの追加確認

`-Onscreen` を追加し、`-RenderOffscreen` を付けずに同じ1280×800で起動した。60FPS設定のRecoveryは攻略PASS、平均46.58FPS、P95 30.380ms、P99 31.548ms、最大49.705ms、50ms超0フレーム。60FPS維持は未達。これはゲームTick間隔であり、画面のPresentを外部計測した値ではない。また、この60FPS実行中の前面表示は目視確認していない。

続く30FPS・Keyboard実行では、ウィンドウを前面に出してゲーム描画を確認した。その実行は頭Grab後のphase 7でタイムアウトし、性能集計は生成されなかった。前面切替によるキー保持の解除が疑われるが原因は未確定であり、成功例には含めない。

継続的に軸入力を送るGamepadシミュレーションでの30FPS再測定は攻略PASS。平均26.65FPS、P95 46.370ms、P99 49.329ms、最大53.706ms、50ms超14フレームで、30FPS維持は未達。この再測定は物理パッド入力の試験ではなく、前面表示の継続も確認していない。

2件の成功実行は生CSVのフレーム数・平均FPS・P99を別途再計算して一致を確認し、失敗した試行も含めて [通常ウィンドウ検証JSON](FuchimatoiOnscreenValidation.json) に保存した。前面表示を維持した条件での性能と、上限待機の原因調査は残る。

## 再現方法

```powershell
.\Tools\Prototype.ps1 -Action Build
.\Tools\Prototype.ps1 -Action Test -Fuchimatoi -Realtime -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Fuchimatoi -Realtime -SkipBuild -TestFPS 30
.\Tools\Prototype.ps1 -Action Test -Fuchimatoi -Recovery -Realtime -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Fuchimatoi -Recovery -Realtime -SkipBuild -TestFPS 30
.\Tools\Prototype.ps1 -Action Test -Fuchimatoi -Recovery -Realtime -Gamepad -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Fuchimatoi -Recovery -Realtime -Onscreen -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Fuchimatoi -Recovery -Realtime -Onscreen -Gamepad -SkipBuild -TestFPS 30
```

`-Realtime` は `-Action Test -Fuchimatoi` 専用。`-Capture` との併用は計測を乱すため拒否する。既存の `-Realtime` なしの固定FPSテストは維持する。

`-Onscreen` は `-Realtime` が必須。通常ウィンドウを生成するが、最前面の維持や画面へのPresent計測を自動保証するものではない。

上限解除の比較は、同じ実時間Recoveryの起動引数に `-ExecCmds="t.MaxFPS 0"` を追加して実行。`Saved/FuchimatoiPerformance/RunUncapped.ps1` が完全なコマンドを保持する。ログの `t.MaxFPS = "0"` も確認している。

## 保存物と変更ファイル

- [計測結果JSON](FuchimatoiPerformanceValidation.json)：各RunId、要約値、CSVとログのSHA256、ビルド証跡。
- `Saved/FuchimatoiPerformance/<RunId>/Frames.csv`：各フレームの壁時計間隔。
- `Saved/FuchimatoiPerformance/*.log`：計測結果と攻略PASS、完了Telemetry。
- `Source/IshibashiriPrototype/Public/FuchimatoiIntegrationTest.h` / `Private/FuchimatoiIntegrationTest.cpp`：実時間モードとフレームサンプル保存。
- `Tools/Prototype.ps1`：`-Realtime` の引数検証、実描画起動、専用ログと計測完了確認。

JSONの平均FPS・P99・フレーム数は、生CSVから別途再計算して一致を確認する。以前の [落下復帰検証JSON](FuchimatoiRecoveryValidation.json) は追加計測前の証跡として保持する。

## 物理ゲームパッドと検証の限界

WindowsのPnPにはXbox Wireless ControllerとBluetooth LE XINPUT互換入力デバイスが正常登録されていた。ただし `XInputGetState` の4スロットは未接続（1167）で、実ボタン・スティック入力は取得できなかった。デバイス登録と現在の入力接続は別として扱う。2026-09-13、ユーザーの「物理パットの確認は大丈夫」を受け、追加の物理機器確認は不要として終了した。接続成功を検証済みとは扱わない。

`Tools/Test-PhysicalGamepad.ps1` は入力を生成せず、XInputの状態変化を最大60秒読み取り、日時付きJSONをSavedへ保存する任意の診断用スクリプト。今回のGamepad攻略テストはPlayerControllerへのシミュレート入力である。

いずれもEditorビルドであり、画面へのPresent/VSync、パッケージ版、異なるPCの性能を保証する結果ではない。フレーム間隔にはCPU・GPU待ち・上限待機・OSのスケジューリングが混在し、GPU単体の処理時間ではない。
