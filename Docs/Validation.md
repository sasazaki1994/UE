# 検証記録 — 2026-09-10

この文書は旧戦闘プロトタイプと各開発段階の検証履歴です。現行の大型モデル・登攀の検証は [登攀検証](ClimbingValidation.md)、PR #17のmain統合後の検証は [カメラ統合記録](CameraIntegrationValidation.md) を参照してください。以下の旧配置でのPASSと、当時の環境制約は現在の統合構成の合否を示しません。

キーボード・マウスを使った手動の操作感評価と、10分間の連続手動プレイは未実施です。以下の自動検証結果と区別します。

## 9月10日の変更（main統合前の旧戦闘配置）

- 遮蔽時は即座に上方へ退避し、遮蔽解消後0.15秒待ってから0.35秒で通常視点へ戻します。位置と注視点を一緒に補間し、復帰途中にも壁と猪の遮蔽を検査します。
- 猪による退避では高度1000cmを使い、主人公と猪の中点を中心に構図を取ります。上方視点でもマウスの水平操作方向を保ちます。壁際のみの高度は720cmです。
- 次の攻撃方向を水色のHUD矢印で表示します。攻撃中は白色で実際の攻撃方向に固定し、終了後に現在のマウス方向へ追従します。矢印は方向表示で、長さは射程を示しません。Debug描画には依存しません。
- 自動検査に、上方視点でのLMB入力、攻撃中・攻撃後のマウス操作と矢印の整合性、段階的な復帰、復帰中の主人公の画面内保持・衝突回避、復帰中の再遮蔽、Retryによるカメラ状態の初期化を追加しました。
- 描画保存に `09-CameraReturn.png` を追加しました。通常の描画検査は8枚を確認します。

### 9月10日の再検証結果

- UE 5.6.1のC++ Editorビルド成功。
- 戦闘・入力・拡張カメラ検査は30 / 60 / 120 FPSすべて成功。RunIdは30 FPSが `c8792fe32aca491f84ebb806c5ac4cd5`、60 FPS（描画付き）が `add838e8b1894ea8b49c2570c15cf3c4`、120 FPSが `40806596002244a6bfbafa3a78900e8d`。終了コード0と各実行固有のPASSを確認しました。
- 入力のみの3戦攻略・R再挑戦は60 FPSで成功。RunId `3ec3512aa77443239f9a5e84fd804b29`、ゲーム内時間45.32秒、終了コード0。
- 描画画像は `Saved/Screenshots/Prototype/add838e8b1894ea8b49c2570c15cf3c4/`。上方視点で主人公・猪・方向矢印が見えること、壁際の視界、復帰途中の主人公と矢印、通常視点の予告場面を目視しました。初回の描画で見つかった矢印の見切れと復帰途中の主人公の画面外への逸脱は修正後に再検証しています。
- Windows Developmentパッケージを再生成し、BuildCookRunの `BUILD SUCCESSFUL` と終了コード0を確認しました。配布先のゲーム本体と今回ビルドしたexeのSHA256は一致しています。
- 更新したパッケージでも60 FPSの描画付き戦闘・入力・カメラ検査が成功。RunId `446dba763d9b49aeba660edbe5e44e26`、終了コード0、3種のPASS、8画像を確認しました。ログは `Saved/Logs/Packaged-CameraAim.log`。パッケージ画像の上方視点と復帰途中も目視しました。
- 手動でのマウス感度・視点切り替えの操作感評価は未実施です。

## 9月9日の変更

- 猪が通常カメラの終点に重なる場合だけでなく、主人公とカメラの間に入って視線を遮る場合も上方視点へ切り替え、主人公と猪の中点を注視するようにしました。
- 自動カメラ検査の猪を視線の中間に配置し、終点の包含判定だけでは通らない回帰テストに更新しました。

### 9月9日の再検証結果

- C++ Editorビルド成功（`Tools/Prototype.ps1 -Action Test`）。
- 自動Playは30 / 60 / 120 FPSすべて成功。RunIdは30 FPSが `67302545e48d428cad45467662b3b7aa`、60 FPS（描画付き）が `74f371e9cfed41da804fa183396204d7`、120 FPSが `e8b1b698707c4173b1e5d41b038ed9d8`。
- 入力のみの3戦攻略・R再挑戦は60 FPSで成功（RunId `d32b788e4a054bcabd272939f1da492b`）。
- D3D11描画検査は成功（RunId `74f371e9cfed41da804fa183396204d7`）。猪の視線途中遮蔽を含む7画像を `Saved/Screenshots/Prototype/74f371e9cfed41da804fa183396204d7/` に保存し、`07-BossCamera.png` で主人公と猪が同時に確認できることを目視しました。

## Generic Gamepad対応（9月8日）

### Gamepad mappings implemented

- 左スティックを既存 `MoveForward` / `MoveRight` に追加しました。通常時はCharacterMovement、Grab中は同じ `ForwardInput` / `RightInput` から既存 `UGrabComponent::Climb` へ流れ、専用Climbing経路はありません。
- 右スティックは `TurnRate` / `LookUpRate` とし、120 deg/s yaw、90 deg/s pitchをDeltaSecondsで積分します。右入力はyaw増加、上入力はpitch減少です。
- A=Jump、B=Dodge、X=Attack、RB=Grab（Press/Release）、Y=Retryです。Space、Shift/RMB、LMB、E、RおよびWASD/Mouseは残しています。
- 左右両スティックのX/Yに `AxisConfig` dead zone 0.20、Sensitivity 1.0、Exponent 1.0を設定しています。

### Simulated gamepad test

- `Docs/Acceptance/Gamepad.feature` と `PrototypeGamepadTest` を追加しました。0.15の無入力、0.25/0.5/1.0のアナログ強度、移動、カメラ符号、A/B/X、RB Grab/Climb/Release、Boss追従、Grab中Y Retryを実入力mapping経由で検査する設計です。
- 起動コマンドは `Tools/Prototype.ps1 -Action Test -Gamepad -TestFPS 60` です。`-Gamepad` は `-Grab` / `-Climbing` / `-Playthrough` と排他です。
- **未実行（PASSではありません）。** 現在のLinux環境には `pwsh` とWindows版Unreal Engineがないため、Simulated Gamepad InputをUE Runtime上で実行できませんでした。

### UE runtime verified / Physical controller verified

- **UE runtime verified: 未検証。** 今回のC++はビルド未実行で、Gamepad / Grab / Climbing / Smoke / Playthroughも再実行できていません。
- **Physical controller verified: 未実施。** 物理XInputコントローラーおよびKB/Mとの途中切替は確認していません。実機確認後まで「Xbox Controller動作確認済み」とは扱いません。

| 再検証コマンド | 現在の結果 |
| --- | --- |
| `pwsh -NoProfile -File ./Tools/Prototype.ps1 -Action Build` | 実行不可：`pwsh`なし |
| `pwsh -NoProfile -File ./Tools/Prototype.ps1 -Action Test -Gamepad -TestFPS 60 -SkipBuild` | 実行不可：`pwsh`なし |
| `pwsh -NoProfile -File ./Tools/Prototype.ps1 -Action Test -Grab -TestFPS 60 -SkipBuild` | 実行不可：`pwsh`なし |
| `pwsh -NoProfile -File ./Tools/Prototype.ps1 -Action Test -Climbing -TestFPS 60 -SkipBuild` | 実行不可：`pwsh`なし |
| `pwsh -NoProfile -File ./Tools/Prototype.ps1 -Action Test -TestFPS 60 -SkipBuild` | 実行不可：`pwsh`なし |
| `pwsh -NoProfile -File ./Tools/Prototype.ps1 -Action Test -Playthrough -TestFPS 60 -SkipBuild` | 実行不可：`pwsh`なし |


## Minimal Climbing（9月8日）

### Implemented

- Grab中だけWASDを `UGrabComponent::Climb` へ渡し、W/Sを対象ActorローカルZ、A/Dを対象ActorローカルYとして `RelativeGrabTransform` の位置へ加算します。通常のCharacterMovementとの二重適用はありません。
- `ClimbSpeed` の既定値は180cm/sです。対象ローカル位置をX=-650..650、Y=-350..350、Z=-350..650cmにClampします。複雑な表面投影は行いません。
- HUDにClimbing操作と現在のローカルXYZを表示します。Acceptance specは `Docs/Acceptance/Climbing.feature`、専用テストは `Tools/Prototype.ps1 -Action Test -Climbing -TestFPS <30|60|120>` です。
- 専用テストコードは上下左右、Boss回転後のローカル移動、実Boss Chase TickとClimbingの同時進行、Release、再Grab後のRetryを検査します。

### Runtime Verified

- **未検証（PASSではありません）。** この環境には `pwsh`、Windows版Unreal Engine 5.6.1のEditor/ビルドツールがないため、C++ビルドもClimbing / Grab / Smoke / Playthroughの実ワールドテストも起動できません。
- コードとspecの静的確認のみ実施しました。Climbingの操作感、カメラ、Collision、30 / 60 / 120 FPSでの挙動はRuntime Verifiedではありません。

| 再検証が必要なコマンド | 現在の結果 |
| --- | --- |
| `pwsh -NoProfile -File ./Tools/Prototype.ps1 -Action Build` | 実行不可：`pwsh`なし |
| `pwsh -NoProfile -File ./Tools/Prototype.ps1 -Action Test -Climbing -TestFPS 30 -SkipBuild` | 実行不可：`pwsh`なし |
| `pwsh -NoProfile -File ./Tools/Prototype.ps1 -Action Test -Climbing -TestFPS 60 -SkipBuild` | 実行不可：`pwsh`なし |
| `pwsh -NoProfile -File ./Tools/Prototype.ps1 -Action Test -Climbing -TestFPS 120 -SkipBuild` | 実行不可：`pwsh`なし |
| `pwsh -NoProfile -File ./Tools/Prototype.ps1 -Action Test -Grab -TestFPS 60 -SkipBuild` | 実行不可：`pwsh`なし |
| `pwsh -NoProfile -File ./Tools/Prototype.ps1 -Action Test -TestFPS 60 -SkipBuild` | 実行不可：`pwsh`なし |
| `pwsh -NoProfile -File ./Tools/Prototype.ps1 -Action Test -Playthrough -TestFPS 60 -SkipBuild` | 実行不可：`pwsh`なし |

## Grab / Moving Actor追従（9月8日）

- 操作はEを押している間Grab、離すとReleaseです。石走りのActor中心から360cm以内でのみ開始できます。
- `UGrabComponent` が対象、開始ワールド位置、対象基準の相対Transform、Grab状態を保持します。Grab中はCharacterMovementを無効化し、対象の移動後に相対Transformから主人公のワールドTransformを再計算します。Release、被弾、Victory / Defeat、Retryでは必ず解除し、MovementをFallingへ戻します。
- Acceptance specは `Docs/Acceptance/GrabFollow.feature`、自動検証は `Tools/Prototype.ps1 -Action Test -Grab -TestFPS 60` です。テストは実入力バインド経由のE、テストが与える平行移動と90度回転、実際のBoss Chase Tickによる移動、Release、再Grab後のRリセットを分離して検査します。
- **自動検証結果：未実施（PASSではありません）。** 2026-09-08に下表のコマンドを実行しましたが、すべて `pwsh: command not found`（終了コード127）で、UEプロセスは起動していません。RunIdとUEログは生成されていません。下記「実行結果」の既存PASSはGrab追加前の実績であり、今回の結果ではありません。
- 実Boss確認を静的なTransform操作だけで済ませないため、GrabテストにBoss Tickを再有効化するChase追従シナリオを追加しました。このシナリオ自体もUE上では未実行です。
- GUI、物理キーボード、マウスを利用できないため、手動確認も未実施です。
- 現在の制限：中心距離だけの判定で、表面・部位・遮蔽を考慮しません。StaminaとGrab / Climbing専用アニメーションはありません。Climbingは上記の直方体Clampによる最小実装です。

### Grab追加後の再検証試行

| コマンド | 結果 |
| --- | --- |
| `pwsh -NoProfile -File ./Tools/Prototype.ps1 -Action Build` | 未開始。`pwsh`なし、終了コード127 |
| `pwsh -NoProfile -File ./Tools/Prototype.ps1 -Action Test -Grab -TestFPS 60` | 未開始。`pwsh`なし、終了コード127 |
| `pwsh -NoProfile -File ./Tools/Prototype.ps1 -Action Test -Grab -TestFPS 30 -SkipBuild` | 未開始。`pwsh`なし、終了コード127 |
| `pwsh -NoProfile -File ./Tools/Prototype.ps1 -Action Test -Grab -TestFPS 120 -SkipBuild` | 未開始。`pwsh`なし、終了コード127 |
| `pwsh -NoProfile -File ./Tools/Prototype.ps1 -Action Test -TestFPS 60` | 未開始。`pwsh`なし、終了コード127 |
| `pwsh -NoProfile -File ./Tools/Prototype.ps1 -Action Test -Playthrough -TestFPS 60` | 未開始。`pwsh`なし、終了コード127 |
| `pwsh -NoProfile -File ./Tools/Prototype.ps1 -Action Package` | 未開始。`pwsh`なし、終了コード127 |

したがって、Grabのビルド、30 / 60 / 120 FPSテスト、実Boss追従、Release後のWalking復帰、Collision、Camera、Retry、Smoke、Playthrough、PackageはいずれもGrab追加後のPASS判定をしていません。UE 5.6.1とPowerShellを備えたWindows環境で上記を再実行することが完了条件です。完了するまでは最小Climbing実装へ進みません。

## 9月7日の変更

- 壁で通常カメラが近づきすぎる場合と、通常カメラが猪の外観に埋まる場合に、主人公の上方720cmの視点へ切り替えます。切り替えの境界に余裕を設け、障害がなくなると通常視点へ戻します。
- カメラ処理を `CalcCamera` に置き、戦闘処理が停止するVictory / Defeat後も視界を確保します。天井のない現在のArenaを対象としています。
- PlayerControllerへの模擬キー・マウス入力による検査を追加しました。WASD、マウスXY、Shift/右クリック、左クリック、Space、勝敗後のRが実際の入力バインドを経由します。
- 4方向の壁際と猪によるカメラ遮蔽を検査し、描画保存を5枚から7枚に増やしました。
- `PrototypePlaythroughTest` を追加。位置・向きの直接変更、戦闘関数の直接呼び出し、HP・AI時間の変更をせず、模擬キーとマウス入力だけで攻略します。各戦で実際の回避開始、3回の反撃、ノーダメージ勝利、R再挑戦を検査します。
- `Tools/Prototype.ps1 -Action Test -Playthrough` とCursorの `Ishibashiri: Test full playthrough` から再実行できます。`-TestSeconds 600` でゲーム内時間600秒以上まで繰り返します。

## 9月8日のモデル差し替え

- CC0のQuaternius Warriorを肩防具・剣込みでFBX化し、待機・走行・攻撃・ロール・被弾・死亡をUEへ取り込みました。
- CC0のTeh_Bucket BoarをFBX化し、毛皮テクスチャ、待機・歩行・攻撃を取り込みました。突進中は歩行を高速再生します。
- テクスチャ座標、足元の高さ、向き、戦闘色のフラッシュを調整し、モデルが読み込まれない場合だけ従来のPrimitiveへ戻る構成にしています。

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
| モデル取り込み | `Tools/Prototype.ps1 -Action ImportModels -SkipBuild` 成功。`FREE_MODELS_IMPORT_PASS`、Warrior 6クリップ、Boar 3クリップ、マテリアルスロットを確認 |
| モデル描画 | `Saved/Logs/PrototypeVisual-60.log`、RunId `3435734854e8405685875b9a93618da1`、終了コード0、7画像を目視確認 |

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

最新のEditor描画：`Saved/Screenshots/Prototype/add838e8b1894ea8b49c2570c15cf3c4/`

パッケージ版の描画：`Artifacts/Windows/IshibashiriPrototype/Saved/Screenshots/Prototype/446dba763d9b49aeba660edbe5e44e26/`

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
- 主人公と猪は無料CC0モデル、HUDは英語です。音、登攀、物語、成長、セーブ、複数ボス、マルチプレイは未実装です。専用の猪の疾走・倒れアニメーションは元素材にないため、歩行の高速再生と状態色で表現しています。
- UE 5.4/5.5での実行は未検証です。実測結果は5.6.1のみです。
- Cursorでの補完操作そのものは未検証です。拡張、生成ヘッダー、コンパイルDBは揃っています。
- UE 5.4互換のinclude順に関する警告と、一部の実行でZenキャッシュの再起動警告が出ています。上記のビルド・テスト・パッケージは成功しています。

次は実行ファイルで10分程度遊び、予告時間、横回避距離、硬直中の接近のしやすさ、壁際のカメラを評価します。
