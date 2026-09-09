# 禍祓い — 石走り戦闘プロトタイプ

巨大猪「石走り」の突進を横回避し、硬直中に刀で反撃する、Windows向けUE 5.4〜5.6 / C++の最小プロトタイプです。3回反撃すると主が鎮まり、3回突進を受けると敗北します。

**UE 5.6.1でビルド・自動Play・描画検証・Windowsパッケージの起動に成功しました。** すぐ遊ぶには **[Artifacts/Windows/IshibashiriPrototype.exe](Artifacts/Windows/IshibashiriPrototype.exe)** を開いてください。隣接するフォルダーも必要なので、exeだけを移動しないでください。模擬入力だけでゲーム内時間10分以上・40戦連続の攻略を確認しました。手動の操作感と10分間の連続手動プレイは未評価です。詳しい実行結果は [検証記録](Docs/Validation.md) を参照してください。

## 最初の起動

1. Epic Games Launcherから **Unreal Engine 5.6**（5.4、5.5も対象）をインストールします。
2. **Visual Studio 2022 Build Tools** のC++ビルドツール、MSVC v143 **14.38.33130**、Windows SDK **10.0.22621.0**、**.NET Framework 4.8 SDK / Targeting Pack** を追加します。必要なコンポーネントは `.vsconfig` に記載しています。Cursorで編集する場合、Visual Studio本体のIDEは不要です。UE 5.6の場合は17.8以降、17.14推奨です。[Epic公式の互換性・セットアップ](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine?application_version=5.6)
3. リポジトリ直下の **`Play.cmd` をダブルクリック**します。UEの検出、Editorモジュールのビルド、空の `L_Prototype_01` の生成、ゲームの起動を順に行います。初回はコンパイルに時間がかかります。

独自のインストール先はPowerShellから指定できます。以下はリポジトリ直下で実行します。

```powershell
.\Tools\Prototype.ps1 -Action Check -EngineRoot 'C:\Program Files\Epic Games\UE_5.6'
.\Tools\Prototype.ps1 -Action Play -EngineRoot 'C:\Program Files\Epic Games\UE_5.6'
```

環境変数 `UE_ROOT` でも指定できます。今回の設置先 `%USERPROFILE%\UnrealEngine\UE_5.6` はスクリプトから自動検出されます。プロジェクトの関連付けは5.6ですが、スクリプトは指定されたエンジンを直接利用します。5.4/5.5で `.uproject` を直接開く場合は、エンジンの関連付けも変更してください。

Editorで確認する場合は `-Action Editor` を使用し、ツールバーの **Play** を押します。床やキャラクターはPlay時に生成されるため、停止中のレベルは空です。**SimulateではなくPlayを選択してください。** 画面をクリックして入力を捕捉し、PIEではShift+F1でマウスを解放、Escで終了します。

## Cursorで開発する

Cursorで **`IshibashiriPrototype-Cursor.code-workspace`** を開きます。フォルダーを直接開いても `.vscode` の設定が適用されます。

- **Ctrl+Shift+B**：Editorターゲットをビルド。
- **Ctrl+Shift+P → Tasks: Run Task → Ishibashiri: Play**：ビルド・マップ生成後にゲームを起動。
- **Ishibashiri: Open Unreal Editor**：Editorを起動。
- **Ishibashiri: Test combat**：開発用の自動戦闘検査。
- **Ishibashiri: Test full playthrough**：移動・カメラ・回避・攻撃・Rの模擬入力だけで3戦連続クリアする検査。
- **Ishibashiri: Check environment**：UEとC++ビルド環境を確認。
- **Ishibashiri: Generate C++ completion database**：ビルド後、clangd向けの `compile_commands.json` を生成。

拡張機能は **Anysphere C/C++ (`anysphere.cpptools`)** を使用します。このPCでは拡張とUEヘッダーを導入し、`.generated.h` と `compile_commands.json` の生成も完了しました。C++の構成を変えたらデータベース生成タスクを再実行してください。Cursor上での補完操作そのものは未検証です。[Cursor公式サポートのC++設定説明](https://forum.cursor.com/t/f12-cant-go-to-definition/163103/5)

Cursor向けにビルド生成物の監視・検索を抑え、プロジェクトの範囲と検証条件を `.cursor/rules/ishibashiri.mdc` に保存しています。グローバルのキーバインドや既存プロジェクトの設定は変更しません。

Visual StudioのIDEを開く必要はありませんが、**MSVC / Windows SDKは別途必要**です。これはCursorを使う場合も同じです。Build Toolsだけで構いません。[Epic公式のエディターとコンパイラーの説明](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-code-for-unreal-engine?application_version=5.6)

## 操作・戦闘

| 操作 | キー |
| --- | --- |
| 移動 | WASD（カメラ基準） |
| カメラ | マウス |
| 回避 | Shift / マウス右。WASDで方向を指定、無入力ならキャラクター正面 |
| 刀攻撃 | マウス左。水色の矢印の方向へ攻撃。マウスで方向を変更 |
| ジャンプ | Space |
| 戦闘を最初から | R（戦闘中・勝利後・敗北後すべて可） |
| Standalone終了 | Alt+F4 |

猪が赤く点滅し、床に予告の矢印が出ます。予告の**終了時**に進行方向を固定するので、その前に回避し終えると狙い直されます。突進が始まる瞬間に横回避し、通り過ぎた猪を追い、緑色の硬直中に接近して攻撃します。刀の水色のカプセル表示が判定範囲です。

有効な反撃は1硬直につき1回です。反撃済みの硬直は黄色、鎮まった猪は青色です。画面にHP、ボス状態、残り時間、反撃可否、無敵状態、回避クールダウン、操作方法、Victory / Defeat / Retryを表示します。フォント依存を減らすためHUDは英語表記です。

主人公のそばの水色の矢印は次の攻撃方向です。振っている間は白色になり、その攻撃の判定方向に固定されます。途中でマウスを動かしても、進行中の攻撃は曲がりません。矢印の長さは見やすさのため一定で、射程を示すものではありません。

## 実装した範囲

- 三人称移動・カメラ、短距離回避、回避中の無敵、被弾後の無敵、近接攻撃。
- 壁際でカメラが近づきすぎる場合や、猪が主人公とカメラの間に入って視線を遮る場合は上方視点へ即座に退避します。猪が原因なら高度を上げて両者を収め、マウスの水平操作方向を保ちます。障害がなくなって0.15秒待ち、位置と注視点を0.35秒かけて通常視点へ戻します。復帰途中の遮蔽も検査し、再び塞がれた場合は退避します。勝敗確定後も動作します。
- 猪の追尾→1秒の予告→方向固定の直線突進→壁または時間で停止→1.65秒の硬直。
- 硬直中だけ、1硬直につき1ダメージ。突進による被弾も1突進につき最大1回。
- 双方HP3、Victory / Defeat、RでHP・位置・AI状態・タイマーをリセット。
- 空レベルから床、4面の壁、照明、主人公、猪、カメラ、HUDを生成。BlueprintやActorの手配置は不要。
- 標準Primitiveと標準マテリアルのみ。回避はスイープ移動、突進もスイープし、接触を別途検査して高速移動時のすり抜けによる見落としを抑えます。

数値はPlayer / Bossのヘッダーにある `UPROPERTY(EditAnywhere)` で調整できます。C++で既定値を変更した場合は再ビルドします。移動600、回避1500 / 0.28秒 / CD 0.55秒、攻撃0.32秒 / CD 0.48秒、突進1500 / 最大1.35秒が初期値です。単位はcm・秒です。

## 主要クラスと変更ファイル

| ファイル | 役割 |
| --- | --- |
| `Source/IshibashiriPrototype/Public/PrototypePlayer.h`、`Private/PrototypePlayer.cpp` | 主人公、入力、カメラ、攻撃、回避、HP |
| `Source/IshibashiriPrototype/Public/IshibashiriBoss.h`、`Private/IshibashiriBoss.cpp` | 猪のPrimitive外観、状態遷移、突進、反撃の制限 |
| `Source/IshibashiriPrototype/Public/PrototypeGameMode.h`、`Private/PrototypeGameMode.cpp` | Arena生成、Spawn、勝敗、Retry |
| `Source/IshibashiriPrototype/Public/PrototypeHUD.h`、`Private/PrototypeHUD.cpp` | Canvas HUD |
| `Source/IshibashiriPrototype/Public/PrototypeSmokeTest.h`、`Private/PrototypeSmokeTest.cpp` | 開発用の実ワールド自動戦闘検査 |
| `Source/IshibashiriPrototype/Public/PrototypePlaythroughTest.h`、`Private/PrototypePlaythroughTest.cpp` | 瞬間移動や戦闘関数の直接呼び出しを使わない、入力だけの連続攻略検査 |
| `Source/IshibashiriPrototype/Private/PrimitiveAppearance.h` | 標準マテリアルの色設定 |
| `Source/IshibashiriPrototype/Private/IshibashiriPrototype.cpp` | ゲームモジュール登録 |
| `Source/IshibashiriPrototype/IshibashiriPrototype.Build.cs` | モジュールの依存関係 |
| `Source/IshibashiriPrototype.Target.cs`、`Source/IshibashiriPrototypeEditor.Target.cs` | Game / Editorビルドターゲット |
| `IshibashiriPrototype.uproject` | プロジェクトとEditor用Pythonプラグイン |
| `Config/DefaultEngine.ini`、`DefaultGame.ini`、`DefaultInput.ini` | 起動レベル、GameMode、描画、パッケージ、入力 |
| `Tools/Prototype.ps1`、`Play.cmd` | 環境確認、ビルド、起動、テスト、パッケージ化 |
| `Tools/CreatePrototypeMap.py` | 空の `Content/Maps/L_Prototype_01.umap` を一度だけ生成。既存ファイルを保存したまま利用 |
| `.gitignore`、`README.md`、`Docs/Validation.md` | 生成物除外、起動説明、検証記録 |
| `IshibashiriPrototype-Cursor.code-workspace`、`.vscode/tasks.json`、`settings.json`、`extensions.json` | Cursorのワークスペース、ビルド・起動タスク、C++拡張の推奨 |
| `.clangd`、`.vsconfig`、`.cursor/rules/ishibashiri.mdc` | C++補完DBの参照先、必要なビルドツール、Cursor用の作業ルール |

PythonはEditorで空のマップを保存するためだけに使います。実行時の戦闘はC++です。`L_Prototype_01.umap` はスクリプトから再生成可能なためGit管理から除外しています。

## ビルド・検証・パッケージ化

```powershell
# C++のビルドのみ
.\Tools\Prototype.ps1 -Action Build

# ビルドとマップ生成
.\Tools\Prototype.ps1 -Action Setup

# UEの実ワールドで自動戦闘検査（画面の検査は含まない）
.\Tools\Prototype.ps1 -Action Test -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -TestFPS 30 -SkipBuild
.\Tools\Prototype.ps1 -Action Test -TestFPS 120 -SkipBuild

# 実際に描画し、戦闘5場面とカメラ3場面をSaved/Screenshots/Prototype/<RunId>へ保存
.\Tools\Prototype.ps1 -Action Test -TestFPS 60 -Capture -SkipBuild

# 入力だけで横回避→通常移動で接近→反撃→勝利→Rを3戦繰り返す
.\Tools\Prototype.ps1 -Action Test -Playthrough -TestFPS 60 -SkipBuild

# ゲーム内時間で600秒以上、戦闘とR再挑戦を繰り返す（手動プレイの代用ではありません）
.\Tools\Prototype.ps1 -Action Test -Playthrough -TestSeconds 600 -SkipBuild

# 入力だけの攻略を描画し、最初の勝利を08-InputVictory.pngに保存
.\Tools\Prototype.ps1 -Action Test -Playthrough -Capture -SkipBuild

# 前回ビルドを使い、実画面で操作を確認
.\Tools\Prototype.ps1 -Action Play -SkipBuild

# Windows Developmentパッケージ。Artifacts\Windows以下に出力
.\Tools\Prototype.ps1 -Action Package
```

自動検査は実際のCharacterMovement・床・攻撃スイープ・ボスAI・HP・Retryを使います。WASD、マウスXY、Shift/右クリック、左クリック、Space、勝敗後のRはPlayerControllerに模擬入力を送り、設定済みの入力バインドを通して検査します。攻撃位置への移動や一部の戦闘操作にはテスト用の位置変更・関数呼び出しを使います。4方向の壁際、主人公とカメラの間に入った猪による視線遮蔽、通常視点への復帰も検査します。物理デバイスによる操作感や戦闘の面白さは実プレイで別途確認が必要です。成功判定には終了コードと、その実行固有の `PROTOTYPE_TEST_PASS` ログの両方を必要とします。ログは `Saved/Logs/PrototypeSmoke-60.log` などに保存します。

## 制限と次の作業

- 入力だけの連続攻略検査は、通常のCharacterMovementで接近し、マウス入力で狙い、横回避・刀攻撃・Rでの再挑戦を行います。検査コードから位置、HP、AIの時間、戦闘結果を変更していません。ボスの状態を読んで操作する自動検査なので、人間の見切りや楽しさの評価とは別です。
- **UE 5.6.1でEditor/Gameのビルド、30/60/120 FPSの自動戦闘検査、D3D11描画、Windowsパッケージの起動・自動戦闘検査が成功しています。** 物理キーボード・マウスを使った手動の操作感評価は残っています。
- 猪と主人公の物理的な押し合いは実装していません。猪の内部を歩いて通過できます。回避を体で阻まないため、突進ダメージを独立したスイープで判定しています。
- 形状に合わせた精密な当たり判定ではありません。猪は縦カプセル、攻撃は前方への球スイープです。予告矢印と攻撃判定表示はDevelopment用のDebug描画を使います。
- 近距離・壁際では上方視点に切り替わります。上方視点はこの天井のないArenaを対象とした処理です。視点切り替えの操作感は手動で評価する必要があります。
- 未実装：本番モデル、アニメーション、音、豪華なVFX、Climbing、Grab、Stamina、禍根の本番システム、物語、NPC、装備、成長、セーブ、複数ボス、マルチプレイなど。今回の範囲を超える機能は追加していません。
- 実プレイ確認後、10分程度繰り返し遊び、予告・回避距離・反撃へ接近できる時間だけを調整します。本編機能の追加はその後の判断です。
