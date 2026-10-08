# 古祀の盆地：背景制作と差し替え契約

## 制作範囲と素材の現況

基準mainは `98e73a4942cc1ed45b0de00a18dcf7389156539f`。分類はPresentation / Production tooling / Documentationで、石走りの既存戦闘を維持したまま `ABasinPrototypeArena` の背景を更新する。山奥の古い祭祀場を、風化した岩盤、針葉樹林、倒木、石走りの足跡、少数の無人の祭祀遺構で伝える。参照作品の背景、配置、固有意匠は複製しない。

既存採用素材は [2026-10-04の素材記録](../../Art/Environment/Ishibashiri/PolyHaven/README.md) と [UE import report](../../Art/Environment/Ishibashiri/PolyHaven/ue-import-report.json) に記録されている。Poly HavenのCC0岩・針葉樹・倒木・シダ・PBR地面、Blender procedural境界石・祭杭・縄・踏石・足跡/亀裂Decalを再利用する。`OldCedar` は `fir_tree_01` を使った針葉樹の代用であり、専用の老杉モデル完成を意味しない。

`manifest.json` と既存Promptは当初のTripo制作仕様として維持する。今回Tripoは **NOT_RUN**。以下の専用Mesh名は任意の差し替え先であり、その名のモデルを生成・import済みと主張しない。実機・静的検証の結果と画像保存先は今回の検証記録を参照する。

## 遺構Asset一覧

単位はcm、配置は盆地Actorのlocal座標、推奨サイズは専用Meshに指定する配置時の目標外形。Xは東、Yは北、Z=0が戦闘床上面。`H = ClearingHalfExtent = 4000`。値は `BasinPrototypeArena.cpp` の `AddRitualRemnants` に対応する。`Fit` は極端な軸方向の引き伸ばしを抑えるため、代用Meshの実外形が目標値と完全に一致するとは限らない。専用6点に加え、既存境界石を掲載する。

すべて **NoCollision**。差し替え先の共通prefixは `/Game/Environment/Ishibashiri/` で、UE Content Browserでは `Content/Environment/Ishibashiri/`。差し替えMeshが存在するときだけ採用し、欠けた場合は既存素材または簡素な仮形状へ戻る。灯籠だけは専用Meshがない間は配置しない。差し替えのためにAI、Collision、Grab、Routeを動かさない。

| Asset / 用途 | 推奨サイズcm | 配置cm | 既存代用・状態 | Tripo必要度 | UE差し替え先 |
| --- | --- | --- | --- | --- | --- |
| 崩れた巨大鳥居：かつての祭祀場の入口 | 1050×380×650 | `(-4200,1750,0)`、Yaw -18° | `FallenCedar_A` 横木＋`RitualPost_A` 柱。既存部材の組み合わせ | 任意。鳥居としての接合部・破断面の品質を上げる候補 | `SM_Ishibashiri_CollapsedTorii_A` |
| 注連縄石柱：山の主を敬う境界 | 180×160×420 | `(1650,4180,0)`、Yaw -20° | `BoundaryStone_A` を主柱・相方柱にし、`OldRope_A` を柱間へ渡す。柱巻き縄は専用形状待ち | 任意。柱に巻いた縄と破損部を一体で表現する場合の優先候補 | `SM_Ishibashiri_ShimenawaPillar_A` |
| 古い境界石：無銘の小さな目印 | 79×53×191を基準 | 入口側の既存 `(-3900,700,0)`、Yaw 80° | `BoundaryStone_A` が実在。文字や大きなsymbolを増やさない | 不要。既存採用品を維持 | `SM_Ishibashiri_BoundaryStone_A`（既存） |
| 苔の小祠：人の不在を示す小さな遺構 | 240×210×290 | `(4140,-1350,0)`、Yaw 180° | 踏石の台座＋Cube本体/屋根2枚。小祠候補の仮形状 | 任意。屋根・柱・朽ちた面の専用品質は素材待ち | `SM_Ishibashiri_SmallShrine_A` |
| 巨大祭祀石：自然の巨岩を敬っていた痕跡 | 700×520×780 | `(2950,-4150,0)`、Yaw 37° | `Rock_B` を再利用。刻像や新しい攻略地点を作らない | 低。既存岩が十分なら制作不要 | `SM_Ishibashiri_SacredStone_A` |
| 石灯籠：祭祀域の小さな人工物 | 110×110×185 | `(-1650,-4120,0)`、Yaw 10°、専用Mesh導入時に1点 | 代用配置なし。現在は素材待ち | 任意。専用外形が必要なら制作。発光・Gameplay機能は不要 | `SM_Ishibashiri_StoneLantern_A` |
| 石碑：風化した無銘の石 | 80×55×190 | `(-2650,4160,0)`、Yaw 24° / Roll -18° | 傾いた `BoundaryStone_A` を再利用。文章・Questの起点を持たせない | 低。既存境界石で代用可能 | `SM_Ishibashiri_Stele_A` |

鳥居代用の立柱は `(-4200,1330,0)` / 50×50×520、折れ柱は `(-4200,2170,0)` / 50×50×280、倒れた横木は `(-4400,1280,0)` / 1100×100×100。石柱代用は主柱 `(1650,4180,0)` / 140×110×380、相方柱 `(2550,4180,0)` / 90×70×230、縄の端Aは `(1650,4180,215)`。縄は既存16m segmentをlocal X方向0.56倍で使う弛んだ柱間の縄で、巻き縄の完成品ではない。

新しい専用Meshは根元のground contactをpivot、XY center、local Z-upで制作する。CollapsedToriiは壊れた全体を含む一体のboundsと安定したground pivot、縄を含む石柱は縄をbounds内に収める。Surfaceは既存の苔岩・風化木・縄Materialへ合わせ、原則1–2 slots、shared 1k–2k texturesを優先する。複雑なCollisionやcomplex-as-simpleは不要で、import済みMeshにも配置時のNoCollisionを明示する。

## 地面、岩壁、植生

地面の `M_Ishibashiri_Ground` は既存のforest ground PBR、world XY / 300cm mapping。新しい凹凸Collisionを作らず、足跡と湿った土の色・roughnessで踏み荒らされた帯を読む。Basin足跡は専用の平面matte `D_Ishibashiri_BasinFootprint` を使い、既存1024² `T_D_Ishibashiri_Footprint_A_BaseColor` のalphaだけを再利用する。足跡Normalは接続せず、roughness .94 / specular .04で盛り上がった光沢形状に見えるのを抑える。Approach用の元Material `D_Ishibashiri_Footprint_A` とtextureは変更しない。亀裂は既存 `D_Ishibashiri_CorruptionCrack_A` のBaseColor+alpha / Normalを再利用し、明るいemissive、ダメージ、浄化対象、勝利条件を追加しない。

`AddGroundHistory` は10枚の足跡（幅290 / 長さ480）、外周の18枚の苔/湿土（幅700–1200 / 長さ1040）、6枚の黒い亀裂（幅380 / 長さ520）、踏み荒らされた5枚の湿土（幅620 / 長さ1400）を浅く投影する。投影depthのhalf-sizeは4cm。新しい `D_Ishibashiri_BasinWetSoil` と `D_Ishibashiri_BasinMoss` はtexture-freeの色・roughness・soft opacity Materialで、既存texturesを再importしない。生成ツールは [CreateKoshiBasinMaterials.py](../../Tools/CreateKoshiBasinMaterials.py)。専用足跡・湿土・苔・土埃の新Materialは計4点、新textureは0点。

岩壁は不可視境界と外観を分けた既存構造を維持する。苔岩A/Bの回転、部分的な重なり、低い崩落塊で地層と不規則な輪郭を作る。大きく引き伸ばすほどtextureとsilhouetteが荒れるため、専用岩盤が必要かは実画面で判断する。必要な場合もプレイ境界ではなく外観Meshを差し替える。

`AddWallDressing` は外周のNoCollision崩落岩64個とシダ64株をISMで足し、戦闘領域外に16個の低い遠景岩盤を置く。岩の色と地面のTintはBasin ComponentのDynamic Materialで抑え、共有Material assetそのものは書き換えない。

シダ・小岩・遠景針葉樹は既存Instanced Static Meshを使い、近景の高密度杉を大量追加しない。現在の杉LOD0はA 434068 / B 321395 triangles、最終LODはA 40582 / B 24534で、3 Material slots。これらは旧Tripo manifestのbudgetとは異なる現行採用値であり、フレーム時間とAlpha overdrawは実機計測する。新規PCGや外部素材の大量追加を前提にしない。

元の内周杉6本は `Trees[I].GetSafeNormal2D() * (H + 850.f)` へ移し、各方向と高さを保って半径4850cmの外周に置く。外周20本・遠景72本は増やさず、杉の総本数を維持して中央からの見通しを作る。位置変更はNoCollisionの外観だけで、床・不可視境界・Boss/Player開始位置は変更しない。

## 保護する境界

- 約80m四方、床上面Z=0、8枚の不可視 `BlockAll` 境界、Player/Boss開始transformを維持する。
- 中央の接近・突進・回避空間に新たな遮蔽物を置かない。外周遺構の見た目もカメラの退避と高所からの白面・禍根の視認性を実画面で確認する。
- 白面の入力、石走りAI、攻撃/回避判定、Stamina、Grab、Climbing/Cling、11Route、3禍根とその位置、落下復帰、Calm/Victory、Retry、他の主を変更しない。
- 背景は既存Calm結果を表示へ反映する。地形やDecalはNushi/Kakon/Campaignの進行を決めない。
- Component生成は盆地Actorが所有し、再構築前の破棄へ含める。Retryで背景ActorやComponentを追加しない。
- Legacy、HighQuality、素材欠落時fallback、`-PrimitiveEnvironment` の比較経路を維持する。

## 素材待ちと実機判断が必要な事項

6専用Meshは制作待ちの差し替え候補。特に崩れた鳥居の接合/破断、小祠の屋根、柱巻き縄は既存素材の組み合わせでは仮形状の品質に留まる。Tripo未実行を生成済みと報告しない。生成する場合は元のexportとtask ID、寸法/pivot/slots、UE import結果を記録してから候補へ置換する。

## 突進と鎮静のPresentation

突進中の土埃は新しいtexture-free `M_Ishibashiri_BasinDust` とSphereを使い、小石は既存 `Rock_A` を小さく使う。各16個の固定ISM poolを生成し、Tickでは既存Bossの `Charge` 状態・位置・突進方向を読み取って視覚instanceだけを更新する。粒子にCollision、Damage、Bossの姿勢制御を持たせない。Niagaraや新しい岩衝突戦闘処理は追加していない。

粒子の可視寿命は .65秒、ageは1秒にclampする。非突進時に全粒子が期限切れならpoolを非表示へresetして早期returnし、idle時に不要なinstance更新を繰り返さない。この処理から実機FPSやフレーム時間の改善を推定しない。

`SetCalmPresentation(true)` は土埃・小石・地響き表示を即時resetし、Fog density .012→.008 / 最大不透明度 .22→.16、主光Intensity 3.2→3.55と暖色を適用する。`false` は元の霧・寒色光へ戻し、同じresetを行う。環境は既存Encounter完了とRetryの通知を受け、浄化と勝利の判定を所有しない。カメラ演出は既存石走り戦の処理を維持し、背景側に新しいカメラ制御を追加しない。

Sky AtmosphereのMie scattering scaleは .012、anisotropyは .65。Fog、Sky Light、曇天の寒色主光と合わせる設定値であり、Legacy/HighQualityでの空・霧・白面/石走りの明度は実画面で検証する。

森の音と地響きは `/Game/Environment/Ishibashiri/A_Ishibashiri_ForestAmbience`、`A_Ishibashiri_ChargeRumble` の任意 `USoundBase` slotを用意する。現状は **ASSET_REQUIRED**。環境音源や地響き音源が完成したとは扱わない。音源を導入した場合は、突進中の開始/停止、Calmでの森の音量回復、Retryでの元音量を実耳で確認する。

既存老杉代用、拡大岩の質感、霧/日光、登攀中の白面と禍根、土埃が操作を隠さないかは人が実画面で判断する。固定poolによる仮土埃・小石は専用VFXとしての最終品質を保証しない。

Before/Afterは実際のUE画面で、入口、初対峙、全景、突進、接近、高所、浄化、鎮静後を同じカメラ/FOV・解像度・描画設定で比較する。今回の実行コマンドと結果、保存先、未検証事項は検証記録へ残す。この制作契約から実機 `PASS` や品質承認を推定しない。


今回の実行結果は [2026-10-09制作・検証記録](../KoshiBasinValidation-2026-10-09.md)、実画像は [8場面Before/After](../KoshiBasinComparison.md) を参照。

壁際の実画像でcameraがNoCollision岩へ入る問題を受け、外観岩壁はH+1650、崩落岩はH+1300を基準へ8m外寄せした。不可視境界は元のH+125のまま。最大7mの既存Boss framing退避に余地を作り、実画像で再検証した。

土埃は後脚後方480cm・左右290cmから出し、Sphere scaleは1.1+age×3、soft opacity強度は.24。位置と見た目の調整であり、Bossの移動や足の判定は変更しない。
