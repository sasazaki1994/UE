# 古祀の盆地 制作・検証記録

記録日: 2026-10-09（Asia/Tokyo）。基準main: `98e73a4942cc1ed45b0de00a18dcf7389156539f`。作業開始時とPR準備時にremote main・未マージPRを確認し、mainの更新なし、open PRなし。分類はPresentation / Production tooling / Documentation。

既存の約80m四方の床と8枚の不可視境界を維持し、見通し、外周岩盤、植生、祭祀の痕跡、足跡、突進と鎮静の表示を更新した。本番候補としてレビューする変更で、専用モデル・音・曇天の最終lookを完成扱いしない。

## 保護した契約と変更

- 床上面Z=0、ClearingHalfExtent=4000、RockWallHeight=1350、Player/Boss開始位置を維持。AI・操作・攻撃/回避・Stamina・Grab・11登攀地点・3禍根・共有Calm/Victory・Retry・Campaign・他の主のproduction sourceは変更していない。
- 中央を遮っていた既存杉6本を半径4850cmの外周へ移し、本数は維持。崩落岩64・壁際シダ64をISMで追加し、16個の遠景岩で輪郭を繋いだ。装飾にCollisionや攻略責任を持たせない。
- 実画像で壁際の視点がNoCollision岩に入る問題を確認し、外観岩壁をH+850→H+1650、崩落岩の基準をH+500→H+1300へ8m外寄せした。不可視境界を動かさず、既存の最大7mのBossフレーミング退避に余地を作った。
- 既存PBR地面・岩・針葉樹・倒木・シダ・境界石・祭杭・縄・踏石・Decal textureを再利用。地面に10枚の平面足跡、苔/湿土18枚、外周亀裂6枚、湿土5枚を浅く投影。共有素材や既存mapは変更していない。
- 少数の鳥居・注連縄柱・小祠・祭祀石・碑を既存部材と明示した仮形状で構成。灯籠は専用Meshが存在するときだけ置く。サイズ・位置・NoCollision・差し替えpathは [制作契約](Art/KoshiBasinProduction.md) に記録。
- 土埃と小石は各16個の固定pool。Bossの状態・位置・方向を読むだけで、Actorを毎frame生成しない。後脚後方480cm・左右290cmから短い軌跡を出し、寿命.65秒、age上限1秒。Calm/Retryでreset、idleは更新を止める。
- Calm通知で霧density .012→.008、最大opacity .22→.16、主光3.2→3.55・暖色。Retryは元の寒色・霧・poolに戻る。2音源slotは素材待ち。

## 検証の読み方

**PASS**は実際のUE/commandlet実行成功、**STATIC_PASS**はPython/source-contract成功、**NOT_RUN**は未実行。30/60は固定simulation stepであり、実描画FPSやフレーム時間の測定ではない。30の非Capture実行はNullRHI。60の比較撮影はD3D12/SM6 HighQuality、CameraとPrimitiveはLegacy D3D11/SM5。

EnvironmentReviewはAIを止めた配置比較。Climbingは通常入力のground counter検証に加え、取り付き配置、Boss停止/回転を使う既存fixtureを含む。BasinScenarioは通常開始位置・通常AI・InputKey経路で進め、移動、姿勢、healthを検証側から与えない。

Retry検証は既存PlayerのRetry bindingを保存し、元delegateを一度実行した直後に状態を観測、Finish/EndPlayで復元する。InputKey送信直後の検査を改め、UEのWalking接地補正も考慮した。XY/回転/scaleとBoss transformを厳密確認し、Player ZはwalkableなBasinFloor・非貫通・capsule床距離1.9〜2.4cm（±.1）を検査する。92cmのspawn指定から90.15cmへの接地補正は正常で、production Retryを変更していない。

## 実行結果

UE 5.6.1 / Windows / VS2022 BuildTools。実行済み結果・RunId・pass marker・ログSHA256・ソースSHA256は [検証JSON](KoshiBasinValidation-2026-10-09.json)。各コマンドは `Tools/Prototype.ps1` を使用し、Testは `-Action Test -Basin -SkipBuild` を共通指定した。

| 場面/設定 | 結果 | RunId |
| --- | --- | --- |
| Before Environment | **PASS** | `a5369c296aa84f30b662572ac6b68db1` |
| Before Basin | **PASS** | `29d85fce236a467c9ac0f6246843241d` |
| Before Climbing | **PASS** | `4cb2cbc67955419596bcb45d8beec3b5` |
| After Environment | **PASS** | `43d4a7092d6648538a32e5cd53dddd51` |
| After Basin | **PASS** | `921d5db7810741fa8f491a305252b371` |
| After Climbing | **PASS** | `24d5ba6c2d744a6593a47a6c3a08c352` |
| After Camera | **PASS** | `d7e010500db746febe8f8d2333a87397` |
| After Basin30 | **PASS** | `d8de34a55e41468d94ab75383c00e4bf` |
| After Climbing30 | **PASS** | `849d90a97b514120a8ee909d7166cdef` |
| After Camera30 | **PASS** | `a698159337ec4357b3797253e2edf7fd` |
| After Primitive | **PASS** | `04d11e999a044e4b88c5cad41e6e15e2` |

- **PASS:** `-Action Build`。最終Arenaのコンパイル・link成功。
- **PASS:** UE Python commandletで `Tools/CreateKoshiBasinMaterials.py` 実行。4材保存、0新texture、0 errors / 0 warnings。
- **STATIC_PASS:** focused source contracts、その後 `python -m pytest Tests -q`（253 passed、18 subtests passed）。`Tools/GameplayContract.py` の全digest一致。保護JSONの更新なし。
- **PASS:** `git diff --check`。

最終の土埃位置/透過調整はBasin HQ Captureで再検証した。他suiteは最終の岩壁/Collisionと同じで、土埃の見た目調整前の実行結果。

BasinScenarioは突進回避・Recover counter・Kneel・Grab・休息・落下・床着地・地上回避・Retry・再Grabを確認。最終背景のRetry後もmesh/light/fog component数129/2/1、health・姿勢・入力・開始位置が復元された。Climbingは既存11-node経路で3禍根→共有Calm/Completed/Victoryを2回、Retryを2回、疲労とsafe ledge復帰を確認。Cameraは4壁・Boss sightline・aim lock・smooth return・再遮蔽・Retryの数値検査を実施した。

## 実画像と品質判断

[Before/After比較](KoshiBasinComparison.md) は8場面16枚のUE実画像。実PNGは888×500で、launch指定1280×800とは異なるため実寸を正本にした。8組すべて記録されたcamera座標/回転/FOVが一致。露出や未丸めtransformの精密一致までは主張しない。複数routeで上書きされるClimbing画像は最後の同名metadataと対応した。生成・リサイズ・加工は行っていない。

Beforeは基準mainのArena function bodiesを再ビルドしたもの。UHT layoutを揃えるためfinal headerと何もしないTick overrideを残したが、mainのconstructorはTickを有効化せず、新materialもmainの背景から参照されない。通常AIの経路と視覚fixtureは両側で同じ検証コードを使った。これは原型fallbackの画像ではなく、main背景との比較である。

目視した範囲では、入口/全景の中央杉遮蔽が減り、外周の岩盤と森林の層が読みやすくなった。接近時の白面とGrab印、高所の白面と赤い禍根、浄化後1/3、鎮静後の状態を実画像で確認した。壁際1方向の追加画像では、修正前の岩の入り込みが消えた。全camera姿勢の安全や人間の操作感はこの数枚から保証しない。

追加画像では土埃の輪郭が弱く、VFX視認性を品質合格にはしていない。固定poolと素材は実装済みだが、専用VFXの仕上げは残る。

追加画像: [突進演出](Images/KoshiBasin/After/09-ChargeEffects.png)、[壁際カメラ](Images/KoshiBasin/After/10-WallCamera.png)。突進開始frameの比較06だけでVFX品質を合格にせず、1.1秒後の追加観測も保存した。

## 使用素材・変更ファイル

新規素材は `Content/Environment/Ishibashiri/` の `D_Ishibashiri_BasinFootprint`、`D_Ishibashiri_BasinMoss`、`D_Ishibashiri_BasinWetSoil`、`M_Ishibashiri_BasinDust` の4 uasset。Footprintは既存alpha maskのみ使用、roughness .94 / specular .04、Normalなし。新texture・専用3Dモデル・音源の追加はない。

変更対象はArena cpp/h、BasinPlaythroughTest cpp/h、ClimbingIntegrationTest、EnvironmentReviewCapture、Prototype.ps1、2 source-contract test、素材生成script、Basin/Art Docs、本記録、比較JSON/Markdownと実画像。元のcheckoutにあった未コミットのcharacter/art修正は隔離worktreeへ持ち込まず、PRにも含めていない。

## 残件と未検証

| 対象 | 状態・残る作業 |
| --- | --- |
| Tripo専用6Mesh | **NOT_RUN**。鳥居、小祠、柱巻き縄は仮形状。祭祀石・碑は既存岩/境界石。灯籠は未配置。差し替え先は用意済み |
| 森の音・地響き2音源 | **ASSET_REQUIRED / NOT_RUN**。slotと開始/停止/音量復元のみ。聴取・balance未検証 |
| 曇天の本番look | 実画像は青い空と強い直射影が残る。薄暗い曇天の最終照明品質には未到達 |
| 岩・植物への禍の亀裂 | 今回は床Decalのみ。岩面・植物への展開は未追加 |
| 岩衝突固有VFX・新規突進camera | 未追加。既存Charge/Recoverと既存cameraを維持 |
| 遺構・植生・土埃の最終品質 | 専用老杉、岩の拡大による質感、遺構の読みやすさ、専用VFXの仕上げと人による品質承認が必要 |
| 実30/60fps維持・GPU profile | **NOT_RUN**。固定step PASSから性能を推定しない |
| Human first play・物理gamepad・contact/animation feel | **NOT_RUN**。自動入力と実画像は人の操作・接触品質の代用ではない |
| 全日本語glyph・音balance・Lumen/VSM最終品質 | **NOT_RUN**。実撮影/利用設定の確認と全項目の品質承認を区別する |
| Package・全4Encounter Campaign E2E | **NOT_RUN**。既存production gameplay digest一致のみ |

## 修正前の失敗と解決

初期Basin/Climbing検証はworld originへ向かう旧baitが盆地中央を越えたBossの背後へ反転しFAILした。検証driverをPlayer側の既存counter経路へ揃えた。Retryは入力dispatch前の検査と接地Zの誤った完全一致でFAILし、上記の観測方法へ修理した。いずれもproduction gameplayを修理・迂回したものではない。

作業中のBuildでは実行中UEによるDLL lock、撮影用ソース切替のtimestampで再コンパイルが省略される問題もあった。UE終了後の逐次Build、明示したArena再コンパイルで解決し、その後の実行結果を採用した。素材commandletも最初の相対project path実行はFAILし、絶対pathで再実行してPASSした。

Cameraの数値PASSだけではNoCollision外観への入り込みを検出できず、実画像の目視で発見した。外観だけを外寄せし、再撮影・回帰を実施した。過去のFAILや未検証品質を最終PASSに混ぜていない。


## PR・CIとPlay起動

Draft PR [#119](https://github.com/sasazaki1994/UE/pull/119) を作成し、Narrative source contracts / Production intake contracts のGitHub CIは成功。CIは静的検査であり、UE実機結果とは別。

Playの初回SkipBuild呼び出しは、GUIを起動後に既存launcherの未初期化LASTEXITCODEでFAILを返した。実際にはゲームが動いており、続く再BuildがDLL lockでFAILした。起動したPlay個体を閉じてBuildを再実行しPASS。通常Playの設定を使い、終了コード変数を初期化した別の起動を行う。人の初回プレイの品質承認はNOT_RUNのまま。
