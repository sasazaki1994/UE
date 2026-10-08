# 石走り戦「古祀の盆地」

## 基準となる既存盆地

`ABasinPrototypeArena` の灰箱と戦闘空間を再利用し、山奥の祭祀場が自然に飲み込まれた「古祀の盆地（こしのぼんち）」へ背景を更新する。基準は最新main `98e73a4942cc1ed45b0de00a18dcf7389156539f`。新しい戦闘ルールや主は追加しない。

`-BasinPrototype` を指定したとき、通常マップ上で従来アリーナの代わりに盆地Actorを1体生成する。`Tools/Prototype.ps1` の `-Basin` がこの引数を渡す。通常Campaignの石走り戦は従来アリーナを使い、今回の盆地制作でCampaignのtravelやGameModeを変更しない。

| 保護する値 | 基準 |
| --- | --- |
| `ClearingHalfExtent` | 4000 cm。約80m四方の戦闘領域 |
| 床 | 上面Z=0 cm、厚さ100 cm、`BlockAll` |
| `RockWallHeight` | 1350 cm |
| 外周Collision | 八角形を作る重なった不可視Box 8枚、`BlockAll` / `WorldStatic` |
| `PlayerStart` | `(-3000, 0, 92)` cm |
| `BossStart` | `(550, 0, 352)` cm |

白面から石走りへ至る入口側の接近線を開け、追尾・突進・回避、怯みと膝つき、Grab、11地点の登攀、3禍根、落下と復帰、Retryを既存仕様のまま確認する。120〜150mへの戦闘領域拡大は行わない。森林下の装飾床と遠景は広く見せるためのNoCollision領域である。

## 現況と今回の背景構成

基準mainには杉A/B、苔岩A/B、倒木、シダ、PBR地面、境界石、祭杭、縄、踏石がある。杉・岩はasset名から読み込み、素材が欠けた場合はEngine Basic Shapesへ戻る。`-PrimitiveEnvironment` は既存の灰箱比較経路として残す。素材の出典と実サイズは [現行素材記録](../Art/Environment/Ishibashiri/PolyHaven/README.md) を参照する。

基準の配置は岩壁40個と入口肩岩2個、近景・外周杉26本、遠景杉72本、シダ140株、小岩35個、倒木3本、浅い踏石7枚、祭杭2本、縄1本、境界石1個。遠景杉・シダ・小岩はInstanced Static Meshでまとめ、枝葉を含む高密度の近景杉を大量に増やさない。視覚MeshはCollisionなしで、床と不可視境界を別に管理する。

今回の構成は、自然を主役にしながら外周へ少数の崩れた鳥居、注連縄を伴う石柱、境界石、小祠候補、巨大祭祀石、石碑の候補を置く。石灯籠は差し替え先だけを用意し、専用素材がない間は配置しない。既存の倒木・祭杭・岩・境界石・踏石を組んだ形状を優先し、専用Meshは任意の差し替え先として用意する。専用モデルがない状態の組み合わせは本番モデル生成済みとは扱わない。寸法、配置、差し替え先と素材待ちは [古祀の盆地制作契約](Art/KoshiBasinProduction.md) に記録する。

中央には新しい障害物を置かず、足跡には専用の平面matte `D_Ishibashiri_BasinFootprint`、亀裂には既存の `D_Ishibashiri_CorruptionCrack_A` を浅い床投影として使う。足跡は既存BaseColor textureのalphaだけを再利用し、Normalを加えずroughness .94 / specular .04で盛り上がった光沢形状に見えるのを抑える。足跡は石走りが長く歩いてきた痕跡、黒い亀裂は外周地表の異変を示す。大きな明暗模様で突進予告や金色の取り付き印、赤い禍根を隠さない。地面のCollision、移動速度、攻撃判定への作用は持たせない。

新規Materialは4点、新規textureは0点。湿土/苔Decal2点と土埃Material1点はtexture-freeで、専用足跡Material1点は既存maskを再利用する。岩壁の根元へ64個の崩落岩・64株のシダをISMで足し、外側に16個の遠景岩盤を配置する。元の内周杉6本は各方向を維持して半径 `H+850 = 4850` cmの外周へ移し、入口・接近・登攀の見通しを作る。杉の総本数と境界Colliderは維持する。

## 光、鎮静、再生成

既存の主光、影なしの寒色Fill、Sky Atmosphere、Sky Light、Height Fogを再利用する。通常時は曇天を意識した寒色主光を使い、AtmosphereはMie scattering scale .012 / anisotropy .65を設定する。Fogは開始距離1200 cm、最大不透明度22%を基準に、外周の奥行きと中央の視認性を両立させる。HighQualityは同じGameplayへDX12 / SM6、Lumen GI・Reflections、Virtual Shadow Mapsを重ねる描画経路で、Legacyは維持する。設定と比較条件は [画質更新記録](VisualQualityUpgrade.md) を参照する。

突進中は既存Boss状態を読み、固定16個ずつの土埃・小石ISMを動かす。寿命は .65秒で、ageは1秒にclampし、突進終了後に全粒子が期限切れなら更新を早期終了する。新しい岩衝突ルール、Damage、カメラ制御は加えない。森の環境音と地響きは任意の音源差し替えslotを用意し、現在は `ASSET_REQUIRED`。

鎮静後は既存の `SetCalmPresentation` を通して霧を薄く、光を暖色にし、土埃・小石・地響きを即時resetする。環境側は浄化、Nushi進行、Victory、Campaignを決定せず、3禍根浄化から共有Calm lifecycleへ至る既存結果を受け取る。Retryは既存状態と鎮静前の霧・寒色光・音量へ戻す責任を維持する。

背景は `OnConstruction` 内で生成し、再構築の前に `ClearGeneratedComponents` で既存Componentを破棄する。Retryで盆地をSpawnし直す経路は追加しない。新しい背景Componentも同じ所有配列へ登録し、破棄・再構築の境界を揃える。

## 起動と検証

```powershell
.\Tools\Prototype.ps1 -Action Build
.\Tools\Prototype.ps1 -Action Play -Basin
.\Tools\Prototype.ps1 -Action Test -BasinScenario -Basin -Capture -SkipBuild
.\Tools\Prototype.ps1 -Action Test -Climbing -Capture -Basin -SkipBuild
.\Tools\Prototype.ps1 -Action Test -Camera -Capture -Basin -SkipBuild
.\Tools\Prototype.ps1 -Action Play -Basin -HighQuality
```

`-Action Play` で手動確認する。`-BasinScenario` は既存の部品検証とは別に、通常開始位置からAIを停止せず、距離と状態を観測しながら通常入力経路で接近、突進回避、Recover中のcounter、Kneelからの取り付き、最初の休息棚、飛び降り、床への着地、地上移動・回避、Retry、再取り付きを検証する。段階別タイムアウト時にはRunIdと両者の位置、取り付き距離、Boss・登攀・Movement状態を記録する。`-Capture` 併用時は `Saved/Screenshots/Basin/<RunId>/` に開始、突進、取り付き直前、休息棚、着地後、Retry後を保存し、カメラtransform/FOV・解像度・Actor状態をmetadataへ残す。

`-Climbing` と `-Camera` は従来どおり、既存の11地点・休息・しがみつき・3浄化・勝利・単独落下・Retryと、壁際/Boss遮蔽カメラを盆地配置で再実行し、`Saved/Screenshots/Climbing/<RunId>/` と `Saved/Screenshots/Prototype/<RunId>/` に実画像を保存する。既存テストは取り付き配置やBoss停止を使う部品検証として維持し、盆地シナリオの代用にはしない。

30/60FPS、Legacy/HighQuality、入口からの全身視認、初期埋まり、回避余地、Grab地点、登攀時の白面と禍根、背景へのカメラ埋まり、鎮静後の見た目、Retry後のComponent数を確認する。画像比較は同じカメラtransform/FOV、解像度、quality、露出と状態で行う。実施結果とBefore/After保存先は今回の検証記録に記載する。

この文書は検証手順と制作契約であり、今回の実機成功を示すものではない。実行したコマンドの `PASS` / `FAIL`、静的検査の `STATIC_PASS`、未実施の `NOT_RUN` を分ける。静的検査からGameplay feel、Lumen/VSM品質、FPS、human first play、実画面の改善を完了扱いしない。


今回の実行結果は [2026-10-09制作・検証記録](KoshiBasinValidation-2026-10-09.md)、実画像は [8場面Before/After](KoshiBasinComparison.md) を参照。
