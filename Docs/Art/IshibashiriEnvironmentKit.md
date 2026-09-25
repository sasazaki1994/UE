# 石走り編 Minimal Production Environment Kit

Status: **SPECIFICATION COMPLETE / ASSET GENERATION NOT RUN**<br>
Baseline: `a0e87c3aba0ebff9619046f5c1aa956d205315c4`（PR #82 mergeを含むローカル先頭。`origin/main` の取得はHTTP 403のため freshness **UNVERIFIED**）<br>
Machine-readable authority: [`Art/Environment/Ishibashiri/manifest.json`](../../Art/Environment/Ishibashiri/manifest.json)

## 目的と範囲

約1,311mの一本道であるApproachと80m Basinに残るEngine Basic Shapesのうち、画面占有率が高い杉・岩と、導線の意味を持つ境界物だけを、反復配置できる8点で置換する。石走りDemoの10〜20分を成立させるためのkitであり、島全体、村、建築、別地域、後半Boss、PCG/Procedural forestを制作しない。

少数に限定する理由は、回転、0.65〜1.8程度の範囲内スケール（asset別の正本値はmanifest）、配置、共有Material/vertex variationで不均一さを作り、個別asset・draw call・texture memory・review量を増やさないためである。杉A/Bと岩A/Bだけにsilhouette差を持たせ、倒木・境界石・杭・縄は物語上必要な機能へ限定する。

「大歳ノ島」は湿気、静けさ、古い自然、無人の気配、巨大な存在の圧、抑えた色数という**品質・雰囲気だけ**の参照とする。固有の島、建築、民俗意匠、石像、symbol、構図、landmarkを複製しない。『禍祓い』の独自性は、古杉、湿った岩、無銘の境界石、石走りの痕跡、黒い禍に置く。

## Scale authorityと用途

白面の既存制作資料は身長約172cm、runtime capsuleはradius 42cm / half-height 88cmである。この値を比較基準とし、UE unitをcmとして寸法を定義した。生成物のbounds、地面接触、playerとの見え方は未検証なので、最終値はすべて **TBD — VERIFY IN UE** である。巨大感は環境物の装飾的巨大化ではなく、白面、約12.2m全長の石走り、杉、Basin外周の相対比較で作る。

| Priority | Asset | 主用途 | 制作寸法（cm） | LOD0 / LOD1 / LOD2 triangles |
|---:|---|---|---|---|
| 1 | `SM_Ishibashiri_OldCedar_A` | Approach主林、Basin外周 | H 1800–2400、幹径70–110 | 28–45k / 14–23k / 5–9k |
| 2 | `SM_Ishibashiri_Rock_A` | 山道肩、崖cluster、Basin | 280–420 × 200–340 × 180–300 | 12–22k / 6–11k / 2–4.5k |
| 3 | `SM_Ishibashiri_BoundaryStone_A` | Boundary clearing landmark | 55–80 × 40–65 × 170–220 | 10–18k / 5–9k / 1.8–3.5k |
| 4 | `SM_Ishibashiri_FallenCedar_A` | Damaged grove、通過痕 | L 1100–1500、幹径80–125 | 22–38k / 11–19k / 4–7.5k |
| 5 | `SM_Ishibashiri_OldCedar_B` | Aと異なる補助silhouette | H 1600–2200、幹径75–120 | 28–45k / 14–23k / 5–9k |
| 6 | `SM_Ishibashiri_Rock_B` | 縦長の補助岩 | 180–320 × 160–280 × 240–420 | 12–22k / 6–11k / 2–4.5k |
| 7 | `SM_Ishibashiri_RitualPost_A` | Approach/Basinの簡素な杭 | 18–28 × 18–28 × 150–210 | 5–9k / 2.5–4.5k / 0.8–1.8k |
| 8 | `SM_Ishibashiri_OldRope_A` | 石・杭・木を結ぶ補助segment | L 250–400、径5–9、sag 20–55 | 8–14k / 4–7k / 1.2–2.8k |

Budgetは非Nanite fallbackを成立させる入口値であり、LODごとに実meshを目視して調整する。自動Decimateを無条件に使わない。Naniteの採否は全assetとも **UE RUNTIME DECISION**。

## First Adoption Batch

最初に作るのは `OldCedar_A` → `Rock_A` → `BoundaryStone_A` の3点だけ。この3点でApproachの大きな画面領域、organic/masked候補、自然岩、人工landmarkという異なるriskを検証できる。

1. 各PromptをそのままTripoへ渡す。
2. 原本を保存し、作業copyだけをBlenderでcleanupする。
3. 寸法、origin、material、LOD、collision proxyを確認してUEへimportする。
4. Approachへ**仮配置**し、既存primitiveを残した比較可能な状態で同一cameraを撮る。
5. Acceptance 15項目とperformanceを評価する。
6. 3点が通った後だけ残り5点を量産する。不合格ならPrompt/cleanup/material規約を先に修正する。

次に実生成すべき1点は `SM_Ishibashiri_OldCedar_A`。画面占有率が最大で、foliage方式と遠距離silhouetteを最初に確定できる。

## Material / texture strategy

- 基本mapは `BaseColor`、`Normal`、`Roughness`、基本解像度は2048×2048。4Kを既定にしない。Opacity/Maskは葉cardまたは必要な縄frayだけに限定する。
- 杉A/B/Fallenはbarkとfoliageを共有候補にし、最大2 slots。岩A/Bは1つのshared rock materialを第一候補とし、苔/濡れ差はparameterまたはvertex colorで出す。境界石も同shader familyを使えるが、正面の可読領域を静かにする。
- 杭と縄は原則各1 slot。生成時に増えた意味のないslotをcleanupで統合する。
- **杉の推奨はhybrid**：幹・主要枝をgeometry、針葉と細枝を少量のmasked cardsとする。Full geometryはsilhouetteとshadowが良いが工数/triangleが高く、full cardsは安いが近距離で平面感が出る。hybridが最小工数とApproach近〜遠景の妥協点。最終方式はAlpha overdraw、shadow、Lumen/Legacy比較後の **UE RUNTIME DECISION**。
- 未生成のtextureはmanifestの`textures: []`のままにし、架空のPNG pathを登録しない。

## Collision / pivot contract

| Asset | Collision | Pivot / origin |
|---|---|---|
| Old Cedar A/B | trunkだけのsimple capsule（必要時は少数） | trunk base、XY center、ground Z=0 |
| Fallen Cedar | simple capsule/box chainまたはlow-poly custom。route沿いはreview後NoCollisionも可 | 重いroot側の論理ground contact、local Xを幹方向 |
| Rock A/B | 1個または少数のsimple convex。complex-as-simple禁止 | ground contact center |
| Boundary Stone | simple box/convex | ground center。傾きがあっても安定配置 |
| Ritual Post | simple capsule/box | base center |
| Old Rope | `NoCollision` | attachment A、local XをBへ向ける |

Collisionはvisual外形の精密再現ではなくplayerを不意に止めないことを優先する。既存Approach navigation corridor、Ishibashiri Grab/Climbing入口、Basinの回避空間を塞がない。Approach距離/path points、Basin size、Grab、Climbing、Kakon、camera、Boss AI、collision gameplay contractを変更しない。

## Tripo → Blender → UE workflow

### Tripo

`Art/Environment/Ishibashiri/Prompts/` の英語Production Promptとasset別Negative Promptを使用し、全体がframe内、背景/台座なしで生成する。task/export ID、日時、生成設定は実生成時に記録する。現時点は全て`source_file: null` / `tripo_generation_status: NOT_RUN`。

### Blender cleanup

全assetで、disconnected geometry除去、safeなhidden internal geometry除去、可能なnon-manifold修正、transform適用、origin設定、scale確認、UV inspection、material slot cleanup、normal/tangent validation、texture path validation、LOD準備、collision proxy準備、export validationを行う。asset固有手順は各Promptに記す。silhouetteを比較せずautomatic Decimateを適用しない。

Blenderはmeter/Z-up/-Y forwardという既存intake規約を用い、export時にUE cmとの換算を明示する。生成原本を上書きせずcleanup fileとexportを分ける。

### UE (future)

UE 5.6.1で命名、scale、pivot、normal、UV、material slot、texture、LOD screen transition、collisionを確認する。Approach/Basinへの配置は最初はtemporaryで、navigation/collision visualization、通常操作、Grab/Climbing、30/60 fixed-step regression、HighQualityとLegacy、frame p50/p95・draw calls・texture memoryを比較する。合格前にprimitive baselineを削除しない。

Old Ropeはまず固定segmentとする。繰り返し用途で変形が必要なら、同じ断面/UVを用いるstraightなSpline Mesh候補をcleanupで別exportするが、spline system実装は今回の範囲外。

## Decal / Material variation（3Dにしない）

`D_Ishibashiri_Footprint_A`、`D_Ishibashiri_CorruptionCrack_A`、`D_Ishibashiri_Gouge_A`、wet mud、small mossはDecal/Texture/Material variation backlogとする。Tripo meshにしない。PNGは未生成であり詳細は`Art/Environment/Ishibashiri/Decals/README.md`を正本とする。

## Before / After capture contract

同一build SHA、map、camera transform/FOV、解像度、時刻、露出、quality preset、asset placementで、primitive Beforeとcandidate Afterを対にする。

1. Approach entrance
2. Cedar enclosure
3. Boundary clearing
4. Damaged grove
5. Ishibashiri reveal gap
6. Basin entrance
7. Basin wide

各pairでplayerを同じ位置に置き、route clearanceと白面/石走りとのscaleも読めるframeを含める。現状は **NOT_RUN — WINDOWS UE 5.6.1 REQUIRED**。画像が存在する、または改善したとは主張しない。

## Acceptance criteria

候補は次をすべて満たした場合だけ採用する。

1. Primitiveより明確に自然物らしい。
2. 約172cmの白面および石走りとのscaleが合う。
3. 大歳ノ島または他作品の固有意匠を直接コピーしていない。
4. 『禍祓い』の暗く湿った山林へ色・roughness・agingが合う。
5. 遠距離でもsilhouetteが読める。
6. Approach routeを塞がない。
7. Grab入口を塞がない。
8. Ishibashiri戦の回避空間を狭めない。
9. Collisionが外形に対して過剰でなく、引っ掛かりを生まない。
10. Material slot数が上記上限内である。
11. Texture memoryが過剰でなく、原則2K/shared候補を使う。
12. UE 5.6.1へerrorなくimportでき、scale/pivot/normal/UV/LODが正しい。
13. HighQuality同条件表示でPrototypeより改善している。
14. Performance regressionが対象機で許容範囲内である（値はFirst Batch実測で承認）。
15. Rotation、scale、A/B、material variationによる反復が不自然に見えない。

## 未検証と非対象

Tripo生成、Blender cleanup、FBX/GLB、texture、UE Import、runtime scale、collision、navigation、Grab/Climbing、render、performance、HighQuality比較、captureはすべて**NOT_RUN**。Windows UE 5.6.1、Blender、Tripoが必要であり、source-only testはこれらを証明しない。

Gameplay、Approach距離/path point、Basin size、Grab/Climbing/Kakon、camera、Boss AI、白面/石走りmodel、後半Boss、村/建物、open world、PCG/procedural forest、Niagara、音源は変更・制作しない。

## Tripo未契約時のFree Fallback Pipeline

First Adoption Batchの3点だけは、外部texture、network access、有料AI、外部3D API、Marketplace素材を使わず、`Tools/CreateIshibashiriEnvironmentKit.py` によりBlender内のgeometryとprocedural materialから決定的に生成できる。`manifest.json` の寸法、LOD0 triangle budget、material slot上限、collision、pivot、Gameplay constraintを正本とし、Tripo向けpromptとstatusは変更しない。

```bash
blender --background --factory-startup --python Tools/CreateIshibashiriEnvironmentKit.py
python Tools/VerifyIshibashiriEnvironmentKit.py
```

生成物は `Art/Environment/Ishibashiri/Generated/{OldCedar_A,Rock_A,BoundaryStone_A}/` にGLB、FBX、report、neutralな3/4 previewとして出力する。LOD0を優先し、silhouetteを目視比較できないLOD1/LOD2は `NOT_GENERATED` と明記する。procedural nodeのGLB/FBX完全移植は保証せず、reportに **PROCEDURAL MATERIAL — UE FINAL MATERIAL / BAKE REQUIRED** と記録する。生成後もUE ImportとUE Visual Reviewは実施するまで `NOT_RUN` であり、成果物は **BLENDER PRODUCTION CANDIDATE** であってFINAL PRODUCTION ASSETではない。

採用関係は次の通りとする。

- Blender candidateが十分な品質なら、そのまま採用可能。
- Blender candidateが不足する場合だけ、将来のTripo版と比較する。
- Tripoを契約しても、自動的に全Assetを作り直す必要はない。

現在の環境にBlender executableがない場合はasset実体を偽造せず、manifestの`fallback_generation_status`を`NOT_RUN`に維持する。この場合の結果は **BLENDER GENERATION NOT_RUN — BLENDER EXECUTABLE NOT AVAILABLE** とする。
