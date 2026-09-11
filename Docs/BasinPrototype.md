# 「岩壁に囲まれた盆地」仮地形

## 目的と寸法

`-BasinPrototype` を指定したときだけ、通常マップ上で従来アリーナの代わりに `ABasinPrototypeArena` を1体生成する。中央は既存値と同じ半径40m（約80m四方）、床上面Z=0cmの平坦な戦闘域で、細かな障害物は置かない。岩壁の基準高は13.5m。8枚の重なる不可視Boxを八角形の移動境界とし、その外側へ標準Sphereを不均一に重ねて風化した輪郭を作る。

白面は西側入口寄りの `(-30m, 0m, 0.92m)`、石走りは中央付近 `(5.5m, 0m, 3.52m)` から開始する。両者の間は開けているため、遮蔽を考慮しない現行Chaseが入口壁越しに詰まらず、全身を見ながら短く接近できる。境界は `BlockAll`、見た目の岩はCollisionなしとして、複雑な隙間への挟まりを避ける。境界内面と岩の最前面は概ね同じ帯に置く。

色はくすんだ土、灰色2階調、苔、朱、藍に限定し、標準BasicShapeMaterialのDynamic Materialで設定する。西の入口から中央に向かう七枚の浅い土色の道標と、朱の祭柱・藍の横木を配置し、民俗的な祭祀の印象と進行方向を両立する。特定作品の図案や神社建築は直接模倣せず、形と色のリズムだけで世界観を作る。これらはすべてCollisionなしで、登攀と戦闘の空間を変えない。

斜めの暖色主光と影なしの寒色Fillで、白面、金色の取り付き印、赤い禍根と予告表示の明度差を残す。地面より下に置いた薄いHeight Fogは、視界1,200cm以内に影響せず最大不透明度22%とし、外周の岩だけを寒色へ退ける。外部素材、プラグイン、植生は追加しない。

調整値はActorの `ClearingHalfExtent`、`RockWallHeight`、`PlayerStart`、`BossStart` に集約している。地形はStartPlayで一度だけ生成し、Retryは既存Player/BossのTransformと状態だけを戻すため増殖しない。

## 起動と検証

```powershell
.\Tools\Prototype.ps1 -Action Play -Basin
.\Tools\Prototype.ps1 -Action Test -BasinScenario -Basin -Capture -SkipBuild
.\Tools\Prototype.ps1 -Action Test -Climbing -Capture -Basin -SkipBuild
.\Tools\Prototype.ps1 -Action Test -Camera -Capture -Basin -SkipBuild
```

前者で手動確認する。`-BasinScenario` は既存の部品検証とは別に、通常開始位置からAIを停止せず、距離と状態を観測しながら通常入力経路で接近、取り付き、最初の休息棚、飛び降り、床への着地、地上移動・回避、Retry、再取り付きを検証する。段階別タイムアウト時にはRunIdと両者の位置、取り付き距離、Boss・登攀・Movement状態を記録する。`-Capture` 併用時は `Saved/Screenshots/Basin/<RunId>/` に開始、取り付き直前、休息棚、着地後、Retry後を保存する。

`-Climbing` と `-Camera` は従来どおり、既存の11地点・休息・しがみつき・3浄化・勝利・単独落下・Retryと、壁際/Boss遮蔽カメラを盆地配置で再実行し、`Saved/Screenshots/Climbing/<RunId>/` と `Saved/Screenshots/Prototype/<RunId>/` に実画像を保存する。既存テストは取り付き配置やBoss停止を使う部品検証として維持し、盆地シナリオの代用にはしない。

Linux環境などWindows版UE/PowerShellがない場合、ビルド、プレイ、画像取得は実施できない。上記コマンドをUE 5.6環境で実行し、初期埋まり、全身視認、地上操作、外周カメラ、通常AI下の盆地シナリオ、全状態からのRetry、および指定画像を目視判定することが完了条件となる。Python静的検査のみをプレイ成功の根拠にはしない。
