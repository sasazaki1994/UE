# 登攀・リグ・アニメーション検証

UE 5.6.1 / Windows / Blender 3.6.23。2026-09-08〜09の実装。

## main統合後の再検証（2026-09-09）

PR #7のブランチへmain `6d08e29`（PR #15まで）をマージ。9ファイルの競合を解消しました。
通常の白面・石走りのルート登攀へゲームパッド入力を接続し、汎用Grabは `bUseRouteClimbing = false` で選択できる形で保持しています。
FreeModelsのアセット・出典、新ボスと共通コンポーネントも保持しています。

| 検証 | 結果・Run ID |
|---|---|
| UE 5.6.1 Editor C++ビルド | 成功 |
| ルート登攀・キーボード60 FPS | `64033401750b4a1bb58e661d8166db49`、51.95秒、成功 |
| ルート登攀・キーボード30 FPS | `44e4c9d0a2454407bb21074b3d8cd4da`、52.10秒、成功 |
| ルート登攀・模擬ゲームパッド60 FPS・D3D11描画あり | `a92023bbcd424e3e873c288261a7e7b6`、51.95秒、成功。6場面を保存、頂上の描画を確認 |
| mainのGrab追従テスト60 FPS | `ca51db5629444cdd8dd7c4469a818992`、成功 |
| mainのローカル座標登攀テスト60 FPS | `554c279813c84889b87f5186f41a43d8`、成功 |
| mainの模擬ゲームパッドテスト60 FPS | `8848cffb7fe244e59c2e1ef33322231a`、成功 |
| mainのUE Automationテスト | 8件成功、失敗0、警告付き成功0。禍根2件、淵纏い2件、主の基底・状態・Encounter管理、スタミナ |

main由来のテストは汎用Grabを明示的に選ぶ配置を使い、今回のモデル寸法に合わせてリセット位置を更新しています。ルート登攀とは別のモードの検証です。
模擬ゲームパッド検証はデッドゾーン、移動、カメラ、ジャンプ、回避、攻撃、Grab、アナログ登攀速度、追従、Release、Retryを含みます。

統合時に修正した不具合：

- `UNushiStateComponent::IsActive()` がUE標準UFUNCTIONと衝突していたため、`IsNushiActive()` へ改名。
- 3つのテストの `UE_LOG` で動的なログレベルを渡していたコンパイルエラーを修正。
- PowerShell 5のStrictModeで0/1個のスイッチに対する `.Count` 参照が失敗する問題を配列化で修正。
- 汎用Grab中に背中のプレイヤーを追ってボスが反転し続ける問題を、騎乗中の移動処理へ統合して修正。
- 模擬ゲームパッドのジャンプ押下・解放と回避判定のタイミングを修正。軸入力はUEのフレーム内加算を避けるため各軸1回/フレームで送信し、中央位置の0も継続して送信。
- Encounter管理テストはActorのUFUNCTION通知が動くよう `InitializeActorsForPlay` でワールドを初期化。

Automationの再実行はUE Editor-Cmdに `-unattended -nullrhi -nosound -ExecCmds="Automation RunTests IshibashiriPrototype." -TestExit="Automation Test Queue Empty" -ReportExportPath="<出力先>"` を指定します。終了コードだけでなく `index.json` の `failed=0` と `succeeded=8` を確認しました。
記録は `Docs/MergeMainValidation.json`、詳細ログは `Saved/Logs/`、Automation元レポートは `Saved/Automation/MergeMainFinal/`。

このマージ後はWindows配布パッケージの再生成・物理コントローラー検査・長時間の手動プレイを行っていません。以下のパッケージ検証はマージ前の記録です。

## 確認したこと

- 主人公18ボーン・猪20ボーン（Blender側。UEではFBXルートが追加される）。全頂点に正規化済みの重み。
- 主人公10・猪5の15クリップをUEへ取り込み、再生時間とスケルトン参照を照合。
- GLBにもスキンとアニメーションが存在し、Blenderでボーンによって頂点が変形することを検査。
- UE EditorターゲットのC++ビルド成功。
- 30 / 60 FPSで登攀の統合検証に成功。
- Windows Developmentパッケージの生成成功。パッケージ内の実行ファイルでも `CLIMB_TEST_PASS packaged-1788903968715` を確認（ゲーム内51.95秒、終了コード0）。
- D3D11描画ありで6場面を保存。初回確認で見つかった材質変換と近距離カメラを修正。

## 実ワールド検証

`AClimbingIntegrationTest` は初期配置と単独の落下試験の配置を設定し、その後はPlayerControllerへW/S/A/D/E/Space/左クリック/Rを送ります。
主ルート・分岐・浄化中にプレイヤーを瞬間移動していません。テスト用の初期配置を使うため、通常スポーンから入力だけで攻略した記録とは区別します。

検査項目：骨組みとアニメーションの読み込み、歩行時の足ボーン変化、Eでの取り付き、岩棚のスタミナ回復、移動と旋回への追従、右肩分岐、3つの異なる禍根、二重浄化の防止、勝利、Rで初期化、Space離脱、スタミナ切れで落下、Eを離した状態の揺れで落下。

初回60FPSは `6a216a64aa994d0fa63058988e2d37ce`、ゲーム内51.95秒で全項目成功。
初回描画は `1521666972ed4b7fbe474af22d95bee5`。
材質・カメラ・補助照明の修正後は `622b43f37c404a1baab5317fcc7d4c86` で再検証し、6場面を確認。白面、濃紺の衣服、緑の苔、縄、赤い禍根がUEで表示されることを確認しました。
ログは `Saved/Logs/ClimbingTest-30.log` と `ClimbingTest-60.log`。再実行で同FPSのログは更新されます。
アセットの検証結果は `Art/Characters/rig-validation.json` と `rigged-ue-validation.json`。

## 制限

登攀は11地点のグラフと補間移動。任意のメッシュ面を登る処理、IK、布・毛の物理、モーションキャプチャ、状態間のブレンドではありません。
当たり判定は猪の根元カプセルと簡略化した岩棚です。全身の形状に忠実な物理接触ではありません。
手動による長時間の操作感評価は未実施です。
地上の硬直反撃は元の仕様を残しており、登攀以外でもボスHPを減らせます。
