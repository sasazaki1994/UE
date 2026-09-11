# 禍祓い — 石走り・登攀プロトタイプ

UE 5.6 / Windows向け。白面の祓い手を操作し、約12mの巨猪へ取り付き、移動する背中の岩棚を登って3つの禍根を祓います。
主人公と猪に骨組み・スキニング・アニメーションを実装し、岩肌・布・皮膚の質感を改善しています。

## 起動

- `Play.cmd`：Editorモジュールをビルドしてゲームを起動。
- `Artifacts/Windows/IshibashiriPrototype.exe`：パッケージ版。隣接フォルダーも必要です。
- `Tools/Prototype.ps1 -Action Editor`：UE Editorで開きます。Playを押すとキャラクターが生成されます。

盆地の仮地形は通常アリーナを置き換える明示オプションです（同じマップを使用します）。

```powershell
.\Tools\Prototype.ps1 -Action Play -Basin
# UnrealEditorを直接使う場合: IshibashiriPrototype.uproject /Game/Maps/L_Prototype_01 -game -BasinPrototype
```

通常起動および既存テストは従来の約80m四方のアリーナのままです。盆地で登攀やカメラを検証する場合は、既存コマンドへ `-Basin` を加えます。

このPCのUEは `%USERPROFILE%/UnrealEngine/UE_5.6` から自動検出します。
独自の場所は `-EngineRoot` または `UE_ROOT` で指定できます。
ビルドにはMSVC v143とWindows SDKが必要です。導入条件は `.vsconfig` と [以前の環境説明](Docs/OriginalCombatPrototype.md) を参照してください。

## 操作

| 状況 | キーボード・マウス | ゲームパッド（Xbox表記） |
|---|---|---|
| 地上移動 / カメラ | WASD / マウス | 左 / 右スティック |
| 回避 / 斬撃 | Shiftまたは右クリック / 左クリック | B / X |
| ジャンプ | Space | A |
| 取り付く | 前脚付近の金色の印に近づきE | 同じ場所でRB |
| 登る・下りる | W / S | 左スティック上 / 下 |
| 分岐 | 背中手前でD。戻るときはAまたはS | 左スティック右。戻るときは左または下 |
| しがみつく | 揺れの予告・揺れ中はEを押し続ける | RBを押し続ける |
| 禍根を祓う | 禍根のある岩棚で左クリック | X |
| 飛び降りる | 登攀中にSpace | A |
| 再挑戦 | R。登攀中・勝利後・敗北後も有効 | Y |

突進中は取り付けません。通常のルート登攀ではE / RBを離しても登攀は継続し、しがみつきだけを解除します。
スティックのデッドゾーンは0.20。ルート上の移動は方向選択方式で、倒し量による速度調整ではありません。

地上では水色の矢印が次の斬撃方向を示し、攻撃中は白色になって判定方向に固定されます。マウス / 右スティックで狙います。矢印は方向表示で、長さは射程を示しません。登攀・Grab中は矢印を隠し、岩棚での浄化操作と専用カメラを使用します。

地上の視界が猪で塞がれた場合は、巨体の外側へカメラを退避します。壁際では上方へ退避します。遮蔽がなくなって0.15秒待ち、位置と注視点を0.35秒かけて通常視点へ戻します。途中で再び遮られた場合は退避を優先します。

前脚→肩→背中手前→中央頂上→背面の順に登ります。背中手前では右肩の禍根へ分岐できます。
右肩・頂上・背面の3か所を祓うと勝利します。地上の硬直中の反撃も利用でき、ボスHPを減らせます。
スタミナは移動中とぶら下がり中に減り、岩棚に立つと回復します。
揺れは約12秒周期で、2秒の予告後に始まります。Eを離していると振り落とされ、Eを押していてもスタミナが尽きると落下します。

## モデルとアニメーション

- `Art/Characters/Shirotsura/Rigged/Shirotsura_Rigged.blend`
- `Art/Characters/Ishibashiri/Rigged/Ishibashiri_Rigged.blend`
- 同フォルダーに骨組み付きFBX、クリップごとのFBX、アニメーション付きGLB、2048pxの色・法線マップ。
- UEアセット：`Content/Characters/Rigged/`。

主人公は待機・歩行・走行・斬撃・回避・登攀・ぶら下がり・しがみつき・空中姿勢・倒れの10クリップ。
猪は待機・歩行・突進・揺れ・鎮静の5クリップです。ゲームの状態から切り替えます。
この白面・石走りのモデルとモーションは制作スクリプトで生成しています。
main由来のCC0モデルも `Content/Characters/FreeModels/` に保持しています。出典は [モデル記録](Docs/ModelCandidates.md) を参照してください。通常プレイには白面・石走りを使用します。

## mainから統合した機能

Generic Gamepadの移動・カメラ・各アクションを、白面の操作とルート登攀に接続しています。
汎用 `UGrabComponent` の対象ローカル座標による登攀も保持し、`APrototypePlayer::bUseRouteClimbing = false` で切り替えられます。このモードはE / RBを離すと即座にReleaseし、上下左右の連続移動と速度に応じたアナログ入力を使用します。2つの登攀処理は同時に開始しません。

`NushiBase`、`NushiEncounterManager`、禍根・状態・スタミナの各コンポーネントと `FuchimatoiBoss` も保持しています。これらは再利用用の独立した実装であり、石走りのHP・スタミナ処理や通常マップのボスを置き換えるものではありません。
主の活動状態を調べる関数はUE標準の `UActorComponent::IsActive()` と区別するため `UNushiStateComponent::IsNushiActive()` です。

## 検証とパッケージ化

```powershell
.\Tools\Prototype.ps1 -Action Build
.\Tools\Prototype.ps1 -Action Test -Camera -Capture -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Climbing -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Climbing -SkipBuild -TestFPS 30
.\Tools\Prototype.ps1 -Action Test -Climbing -Capture -SkipBuild
.\Tools\Prototype.ps1 -Action Test -Climbing -Capture -Basin -SkipBuild
.\Tools\Prototype.ps1 -Action Test -Camera -Capture -Basin -SkipBuild
.\Tools\Prototype.ps1 -Action Test -ClimbingGamepad -SkipBuild
.\Tools\Prototype.ps1 -Action Test -Grab -SkipBuild
.\Tools\Prototype.ps1 -Action Test -LocalClimbing -SkipBuild
.\Tools\Prototype.ps1 -Action Test -Gamepad -SkipBuild
.\Tools\Prototype.ps1 -Action Package
```

描画検査は6場面を `Saved/Screenshots/Climbing/<RunId>/` に保存します。
結果と制限は [登攀の検証記録](Docs/ClimbingValidation.md) を参照してください。
`-Climbing` / `-ClimbingGamepad` は通常のルート登攀をキーボード / 模擬ゲームパッド入力で攻略します。
`-Grab` / `-LocalClimbing` / `-Gamepad` は汎用Grabを明示的に選んだテスト用配置で、mainの相対Transform・Clamp・アナログ速度を検証します。物理コントローラーの検証とは異なります。
以前の戦闘専用テストは小型のPrimitive猪と旧Arenaを前提とするため、現在のモデルの合格証明には使用しません。

`-Camera` は現行モデルとArenaで4方向の壁際、猪の遮蔽、攻撃方向の固定、段階的な復帰、再遮蔽、Retryを検証します。`-Capture` 併用時は `Saved/Screenshots/Prototype/<RunId>/` に3場面を保存します。統合後の検証は [カメラ統合記録](Docs/CameraIntegrationValidation.md) にまとめています。

Cursorでは `IshibashiriPrototype-Cursor.code-workspace` を開き、Ctrl+Shift+Bでビルドできます。TasksメニューのPlay / Open Unreal Editor / Generate C++ completion databaseも利用できます。従来の戦闘テストタスクには上記の旧配置の制限があります。

## 実装の範囲

登攀は設定済みの11地点を連続移動する方式です。移動する猪に追従し、分岐、休息、しがみつき、振り落とし、一回限りの浄化、勝利、再挑戦まで動作します。
岩棚には簡略化した接地用コリジョンを配置しています。
任意の面の自由登攀、精密な全身コリジョン、手足のIK、指先の接触合わせ、布・毛の物理、滑らかなアニメーションブレンドは未実装です。
操作と造形を検討するためのプロトタイプです。

## 制作スクリプト

1. `Tools/CreateCharacterModels.py`：初期形状を生成。
2. `Tools/RefineCharacterModels.py`：丸み、風化した岩肌、毛束、布の細部を追加。
3. `Tools/RigCharacterModels.py`：骨組み・重み・15クリップ・テクスチャを生成。
4. `Tools/ImportRiggedCharacters.py`：UEへ取り込み、クリップ長とスケルトンを照合。
5. `Tools/ApplyRiggedMaterials.py`：色・法線マップをUEマテリアルへ接続。
6. `Tools/VerifyRiggedCharacters.py`：GLBと実際の頂点変形を検査し、プレビューを描画。

1〜3・6はBlenderの `--background --python`、4〜5はUEのPython commandletで実行します。
再生成は専用出力を上書きします。手修正したモデルは別名保存してください。
