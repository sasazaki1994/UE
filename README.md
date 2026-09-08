# 禍祓い — 石走り・登攀プロトタイプ

UE 5.6 / Windows向け。白面の祓い手を操作し、約12mの巨猪へ取り付き、移動する背中の岩棚を登って3つの禍根を祓います。
主人公と猪に骨組み・スキニング・アニメーションを実装し、岩肌・布・皮膚の質感を改善しています。

## 起動

- `Play.cmd`：Editorモジュールをビルドしてゲームを起動。
- `Artifacts/Windows/IshibashiriPrototype.exe`：パッケージ版。隣接フォルダーも必要です。
- `Tools/Prototype.ps1 -Action Editor`：UE Editorで開きます。Playを押すとキャラクターが生成されます。

このPCのUEは `%USERPROFILE%/UnrealEngine/UE_5.6` から自動検出します。
独自の場所は `-EngineRoot` または `UE_ROOT` で指定できます。
ビルドにはMSVC v143とWindows SDKが必要です。導入条件は `.vsconfig` と [以前の環境説明](Docs/OriginalCombatPrototype.md) を参照してください。

## 操作

| 状況 | 操作 |
|---|---|
| 地上移動 / カメラ | WASD / マウス |
| 回避 / 斬撃 | Shiftまたは右クリック / 左クリック |
| ジャンプ | Space |
| 取り付く | 前脚付近の金色の印に近づきE。突進中は取り付けません |
| 登る・下りる | 登攀中にW / S |
| 分岐 | 背中手前の分岐でD。主ルートへ戻るときはAまたはS |
| しがみつく | 揺れの予告・揺れ中はEを押し続ける |
| 禍根を祓う | 禍根のある岩棚で左クリック |
| 飛び降りる | 登攀中にSpace |
| 再挑戦 | R。登攀中・勝利後・敗北後も有効 |

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
外部のモーション素材や他作品の3Dモデルは使っていません。

## 検証とパッケージ化

```powershell
.\Tools\Prototype.ps1 -Action Build
.\Tools\Prototype.ps1 -Action Test -Climbing -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Climbing -SkipBuild -TestFPS 30
.\Tools\Prototype.ps1 -Action Test -Climbing -Capture -SkipBuild
.\Tools\Prototype.ps1 -Action Package
```

描画検査は6場面を `Saved/Screenshots/Climbing/<RunId>/` に保存します。
結果と制限は [登攀の検証記録](Docs/ClimbingValidation.md) を参照してください。
以前の戦闘専用テストは小型のPrimitive猪と旧Arenaを前提とするため、現在のモデルの合格証明には使用しません。

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
