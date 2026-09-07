# 検証記録 — 2026-09-07

判定：**UE 5.6.1でビルド・自動Play・描画・Windowsパッケージの起動に成功。最小プロトタイプを実行可能な状態で用意しました。**

キーボード・マウスを使った手動の操作感評価と、10分間の連続手動プレイは未実施です。以下の自動検証結果と区別します。

## 9月7日の変更

- 壁で通常カメラが近づきすぎる場合と、通常カメラが猪の外観に埋まる場合に、主人公の上方720cmの視点へ切り替えます。切り替えの境界に余裕を設け、障害がなくなると通常視点へ戻します。
- カメラ処理を `CalcCamera` に置き、戦闘処理が停止するVictory / Defeat後も視界を確保します。天井のない現在のArenaを対象としています。
- PlayerControllerへの模擬キー・マウス入力による検査を追加しました。WASD、マウスXY、Shift/右クリック、左クリック、Space、勝敗後のRが実際の入力バインドを経由します。
- 4方向の壁際と猪によるカメラ遮蔽を検査し、描画保存を5枚から7枚に増やしました。
- `PrototypePlaythroughTest` を追加。位置・向きの直接変更、戦闘関数の直接呼び出し、HP・AI時間の変更をせず、模擬キーとマウス入力だけで攻略します。各戦で実際の回避開始、3回の反撃、ノーダメージ勝利、R再挑戦を検査します。
- `Tools/Prototype.ps1 -Action Test -Playthrough` とCursorの `Ishibashiri: Test full playthrough` から再実行できます。`-TestSeconds 600` でゲーム内時間600秒以上まで繰り返します。

## 起動するファイル

- パッケージ版：`Artifacts/Windows/IshibashiriPrototype.exe`
- 開発用：`Play.cmd`（ビルド、必要に応じてマップ生成、ゲーム起動）
- Cursor：`IshibashiriPrototype-Cursor.code-workspace`
- UE設置先：`C:\Users\asgar\UnrealEngine\UE_5.6`

パッケージ版は隣接する `Engine` と `IshibashiriPrototype` フォルダーも必要です。exeだけを移動しないでください。

## 実行結果

| 検証 | 結果・証拠 |
| --- | --- |
| UE導入 | 5.6.1、CL 44394996。Epic配信から9446.01 MiB取得、24551.55 MiB配置、終了コード0 |
| 環境検出 | `Tools/Prototype.ps1 -Action Check` 成功 |
| C++ビルド | UnrealHeaderToolとEditor/Gameターゲットのコンパイル・リンクに成功 |
| マップ生成 | `Content/Maps/L_Prototype_01.umap` を実際のEditor Pythonで作成。0 errors / 0 warnings |
| 自動Play・30 FPS | `Saved/Logs/PrototypeSmoke-30.log`、終了コード0、固有RunIdのPASS |
| 自動Play・60 FPS | `Saved/Logs/PrototypeSmoke-60.log`、終了コード0、固有RunIdのPASS |
| 自動Play・120 FPS | `Saved/Logs/PrototypeSmoke-120.log`、終了コード0、固有RunIdのPASS |
| 入力だけの攻略・30/120 FPS | `Saved/Logs/PrototypePlaythrough-30.log`、`PrototypePlaythrough-120.log`。各3戦連続クリア・R再挑戦、終了コード0 |
| 入力だけの連続攻略・60 FPS | `Saved/Logs/PrototypePlaythrough-60.log`、RunId `32e90c9d19c04c2495cc29096a9c823a`。ゲーム内時間603.91秒、40戦、120回の横回避と反撃、全戦HP3で勝利・R再挑戦、終了コード0 |
| Editor実描画 | D3D11で戦闘を実行し、回避・予告・反撃・勝利・敗北・壁際・猪による遮蔽の7画像を保存・目視確認 |
| Windowsパッケージ | BuildCookRunでBuild/Cook/Stage/Archive完了。`BUILD SUCCESSFUL`、終了コード0 |
| パッケージ版の自動Play・描画 | `Saved/Logs/Packaged-Visual.log`、RunId `aeb1fc8923364e36b4d589c91de8465a`、終了コード0・入力/カメラ/戦闘すべてPASS・7画像生成 |
| パッケージ版の入力だけの攻略・描画 | `Saved/Logs/Packaged-Playthrough.log`、RunId `c1935eebb71c4b679f4f4ec7d5b99231`、3戦連続ノーダメージ勝利・R再挑戦、終了コード0・勝利画像生成 |
| 通常のゲーム起動（9月6日） | パッケージの起動用exeから通常モードで起動。`Saved/Logs/Playable-Launch.log` にArena生成・戦闘開始を確認。その後正常終了 |
| Cursor向けコンパイルDB | `CodeDatabase -SkipBuild` 成功。`compile_commands.json` を生成し、ゲームソースのエントリーを確認 |

### 自動Playで検証した内容

- ローカルのPlayerControllerが主人公を操作対象にし、HUDを持つ。
- WASDの4キーがそれぞれ対応する方向へのCharacterMovement移動を起こす。
- マウス右・上の模擬入力でカメラが右・上へ向き、Spaceでジャンプ・着地する。
- Shiftと右クリックの両方で回避が始まり、左クリックで刀を振る。
- 回避が420cm移動し、継続入力で延長されず、クールダウン中は再使用できない。
- 回避中と被弾直後の無敵がダメージを拒否する。
- 通常状態のボスには刀が効かず、硬直中は実際の刀スイープで1ダメージ入る。
- 同じ硬直では、攻撃クールダウン経過後も2回目の反撃が入らない。
- ボスの突進開始後に主人公を移動させても、突進方向が変わらない。
- 3回反撃でVictory、3回の突進被弾でDefeatになる。
- 突進中ずっと接触させても、同じ突進では1HPだけ減る。
- 勝利・敗北・被弾直後のRetryでHP、位置、無敵、クールダウン、予約ノックバックがリセットされる。
- Retryを繰り返してもArenaのActorが増えない。
- 勝利後と敗北後にRの模擬キー入力でHPと戦闘状態が戻る。
- 4面の壁際および猪で通常カメラが塞がる位置で上方視点になり、実際のカメラ座標が壁・猪の内部にない。
- リトライやカメラ状態のリセットを使わず、障害物から離れるだけで通常視点へ戻る。

これらは実際のUEワールド上の自動テストです。入力の項目はPlayerControllerに模擬イベントを送り、その他の戦闘条件の再現には位置変更や攻撃・Retry関数の直接呼び出しも使います。物理キーボード・マウスの操作を行ったと主張するものではありません。

追加した `PrototypePlaythroughTest` は、上記の条件別テストとは独立しています。予告を待ち、突進開始の約0.1秒後に横回避し、猪が通り過ぎたら通常移動で追いかけ、硬直中に左クリックで反撃します。戦闘終了後はRを入力します。位置変更やゲームプレイ関数の直接呼び出しを使わず、毎回の硬直が終わるまでに反撃でき、再挑戦でActorが増えないことを検査しています。600秒の検証は固定時間刻みで高速実行したゲーム内時間であり、実時間10分の手動プレイではありません。

### 画像

最新のEditor描画：`Saved/Screenshots/Prototype/d1c51f776e7e4eddb064cb87c9ad2877/`

パッケージ版の描画：`Artifacts/Windows/IshibashiriPrototype/Saved/Screenshots/Prototype/aeb1fc8923364e36b4d589c91de8465a/`

入力だけで到達した勝利画像：Editorは `Saved/Screenshots/Prototype/ab77629183424037b257439efb6dcb9a/08-InputVictory.png`、パッケージ版は `Artifacts/Windows/IshibashiriPrototype/Saved/Screenshots/Prototype/c1935eebb71c4b679f4f4ec7d5b99231/08-InputVictory.png`。

撮影は `-Action Test -Capture` で再実行できます。各実行のRunIdごとに保存し、古い画像を成功の証拠として再利用しません。テスト内の位置変更後、カメラ更新を3フレーム待ってから撮影します。

パッケージの検証は `Artifacts/Windows/IshibashiriPrototype/Binaries/Win64/IshibashiriPrototype.exe` に `-PrototypeSmokeTest -PrototypeCapture -PrototypeTestFPS=60 -PrototypeTestRun=<新しいRunId> -RenderOffscreen -unattended -nosound` を指定して実行しました。PowerShellの `Start-Process -Wait -PassThru` で実プロセスの終了を待ち、終了コード0と同じRunIdの3種類のPASSマーカー、7画像を確認しています。

入力だけの攻略は `-PrototypeSmokeTest` を `-PrototypePlaythrough` に置き換え、別のRunId・ログで実行しています。最終パッケージはWindowsで不要なAndroidFileServerを無効にした構成です。配布用exeと最終ビルドのSHA256一致も確認しました。

## 導入済みの開発環境

- Visual Studio Build Tools 2022 17.14.39
- ビルドで使用したMSVC 14.38.33145（フォルダー14.38.33130）
- Windows SDK 10.0.22621.0
- .NET Framework 4.8 SDK / Targeting Pack（初回ビルドのNetFxSDK不足を解消）
- Epic Games Launcher
- CursorのAnysphere C/C++と依存拡張

UEはLegendary 0.21.0で取得しました。開発元GitHub ReleasesのWindows x64版を公開SHA256と照合し、ユーザーの許可を得てLauncherのログインを引き継ぎました。新Launcherの `WindowsEditor` 設定フォルダーに対応する一時アダプターを使用しています。認証情報はプロジェクト外のユーザー用フォルダーで管理しています。

標準のProgram Filesへの配置はWindowsの権限エラーで中断し、現在のユーザー用設置先で導入を完了しました。起動・ビルド用スクリプトはその場所を自動検出します。Launcherのライブラリーへの表示・管理連携は未検証です。

## 制限・残る評価

- 手動のキー入力、マウス感度、壁際のカメラ、戦闘の面白さ、10分間の連続プレイは未評価です。
- 強制的に重ねる接触テストでも、敗北時にカメラが主人公や猪の内部へ埋まる問題を改善しました。重なった主人公は猪に隠れますが、上方から猪・床と勝敗表示を確認できます。視点の切り替わり方の好みは手動評価が残ります。
- 猪と主人公の物理的な押し合いはなく、通常移動では猪の内部を通過できます。
- 見た目はPrimitive、HUDは英語です。本番モデル、音、登攀、物語、成長、セーブ、複数ボス、マルチプレイは未実装です。
- UE 5.4/5.5での実行は未検証です。実測結果は5.6.1のみです。
- Cursorでの補完操作そのものは未検証です。拡張、生成ヘッダー、コンパイルDBは揃っています。
- UE 5.4互換のinclude順に関する警告と、一部の実行でZenキャッシュの再起動警告が出ています。上記のビルド・テスト・パッケージは成功しています。

次は実行ファイルで10分程度遊び、予告時間、横回避距離、硬直中の接近のしやすさ、壁際のカメラを評価します。
