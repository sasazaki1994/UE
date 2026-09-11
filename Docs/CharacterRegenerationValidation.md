# Character Regeneration Validation

## Latest run — 2026-09-11, local UE 5.6.1 / Blender 3.6.23

This section records the run performed from upstream `main` commit
`8872159ba04df30aa069a8cc447980acc8a15593` on branch
`codex/work_character-regeneration-20260911`. The previous blocked run below is
historical; its “no assets changed” conclusion does not apply to this run.

Blender 3.6.23 was verified against the official SHA-256 and executed from the
repository root. Before assets and previews were copied to the ignored local
artifact directory `Artifacts/CharacterRegeneration/20260911-local-8872159/Before/`
with SHA-256 records. The generated assets are present in the tracked `Art/` and
`Content/` paths.

### Stage results

| Stage | Result | Evidence |
|---|---|---|
| CreateCharacterModels | PASS | `logs/CreateCharacterModels.log` |
| RefineCharacterModels | PASS | `logs/RefineCharacterModels.log` |
| RigCharacterModels | PASS | `logs/RigCharacterModels.log`, `RIG_EXPORT_PASS` for 18/20 bones |
| VerifyRiggedCharacters | PASS | `logs/VerifyRiggedCharacters.log`, `RIG_ANIMATION_VALIDATION_PASS` |
| PBR fixture bake | PASS | `PBRBakeValidation/validation-result.json` |
| External CC0 PBR | NOT APPLIED | manifest is unverified; procedural fallback was used |
| UE import | PASS | `logs/UE-Import.log`, `RIGGED_CHARACTERS_IMPORT_PASS` |
| UE material apply | PASS | `logs/UE-Materials-fixed.log`, `BAKED_MATERIALS_APPLIED` |
| UE asset audit / repeat apply | PASS | `ue-asset-validation.json`, `REGEN_UE_ASSET_VALIDATION_PASS` |
| UE build | PASS | `logs/Build.log`, UE 5.6.1 Editor target succeeded |
| High Quality capture | UNVERIFIED | D3D12/SM6 capture was not run after execution approval quota was exhausted |

### Before / after metrics

Metrics were measured by opening the `.blend` files in Blender, counting
triangles/vertices, and checking finite coordinates and weights. Dimensions are
XYZ metres. The rigged mesh object count is one after the authored meshes are
joined for export; the source-scene object counts remain in `model-info.json`.

| Metric | Shirotsura Before | Shirotsura After | Ishibashiri Before | Ishibashiri After |
|---|---:|---:|---:|---:|
| triangles | 74,626 | 75,394 (+768) | 205,172 | 205,442 (+270) |
| vertices | 38,040 | 38,536 (+496) | 103,834 | 103,975 (+141) |
| dimensions XYZ (m) | 1.306174 × 0.330974 × 1.716839 | 1.306174 × 0.330974 × 1.716839 | 7.062849 × 12.178528 × 9.397869 | 7.062849 × 12.178528 × 9.399366 |
| rig bones | 18 | 18 | 20 | 20 |
| materials | 13 | 14 | 14 | 15 |
| texture bytes | 9,307,136 (BC+N) | 9,254,423 (BC+N+R) | 8,568,082 (BC+N) | 8,858,879 (BC+N+R) |
| roughness atlas | absent | 2048² PNG | absent | 2048² PNG |

All 15 requested gameplay clips exist with the expected lengths. Finite animated
mesh sampling passed for every clip, and unweighted, unnormalised, and invalid
weights were all zero for both characters. Runtime route, three Kakon positions,
Grab marker, ledges, collision, character height, and skeleton origins were not
changed by this run.

### UE material and import checks

The normal atlas is authored in Blender’s OpenGL convention and is imported with
UE Green Channel flip enabled. BaseColor uses sRGB; Normal uses `TC_NORMALMAP`;
Roughness uses `TC_MASKS`. The audit found all expected skeletal meshes,
skeletons, 15 animations, material slots, and three atlas references. A second
material application produced identical node counts and references.

### Preview and regression results

Studio previews were regenerated with the existing camera/light setup:
`Art/Characters/Previews/Shirotsura.png`, `Shirotsura_Back.png`,
`Shirotsura_Rigged.png`, `Ishibashiri.png`, `Ishibashiri_Back.png`,
`Ishibashiri_Top.png`, `Ishibashiri_Rigged.png`, and `ScaleComparison.png`.
Pose samples for every clip are in the ignored run artifact under
`After/Poses/`.

The six legacy runs were recorded before and after. Grab and Camera passed in
both runs. Climbing at 60 FPS, Climbing at 30 FPS, gamepad climbing, and the
Basin scenario failed in both runs at the existing interaction/setup checks;
these failures are retained as regression evidence and are not attributed to
the regenerated geometry. Gameplay compatibility therefore remains
**UNVERIFIED**, and no gameplay or route change was introduced.

Visual assessment of the tracked Studio previews: Shirotsura **PARTIAL**
improvement (mask, cloth repairs, hand detail, and subdued emissive roots are
clear; animation deformation needs in-game confirmation); Ishibashiri
**PARTIAL** improvement (rock/body/root integration and moss layering read more
as a single mountain mass; climbing readability needs in-game confirmation).
The generated assets are retained as a review candidate, not declared fully
adopted, until the UE High Quality capture and the pre-existing climbing
regressions are resolved.

## 判定

**UNVERIFIED / BLOCKED — 生成物は不採用。** 2026-09-11 UTC の実行環境には Blender と
Windows UE 5.6 がなく、Blender 3.6.23 公式配布物の取得も HTTP 403 で失敗した。したがって
ソースの存在を再生成成功とはせず、既存バイナリ、UE asset、preview は変更していない。

| 項目 | 値 |
|---|---|
| source commit | `4a250f92ec79416400cf66e5dfb461092e2d45be` (`work`) |
| upstream | remote 未設定のため GitHub の最新 `main` と照合不能 |
| Blender | **UNAVAILABLE**（目標: 3.6.23） |
| UE | **UNAVAILABLE**（目標: Windows UE 5.6 / D3D12） |
| run id | `20260911T073811Z` |
| backup | `Artifacts/CharacterRegeneration/20260911T073811Z/Before/`（Git 対象外） |
| adoption | **REJECTED**（候補自体を生成できていないため） |

`Specs/Acceptance/CharacterVisualRefinement.feature` は、造形、既存 skeleton/scale、11地点、
3禍根、PBR fallback、atlas、反復 material apply を既に acceptance criteria として含む。
今回の不足は spec ではなく実行環境なので、機能 spec は変更していない。

## 作業前保護

指定された5ディレクトリを、生成コマンドより前に上記 `Before` へ再帰コピーした。
`SHA256SUMS.txt` も同所へ保存したため、手修正 asset と preview は後続環境で照合できる。
この約284 MiBの複製は `.gitignore` 対象であり commit しない。

## Before baseline

値は既存 `model-info.json`、`rig-info.json`、`climb-layout.json` と Animated GLB の JSON chunk、
PNG headerから採取した。Blenderで scene を開いて再計測した値ではないため、既存メタデータと
実ファイルの静的監査という位置付けである。GLBは object を統合しており mesh は各1、制作 scene
の mesh object 数は model-info の値を記載する。GLBに exporter の rig action が各1件あるが、
ゲーム用 animation 数からは除外した。

| 指標 | 白面 Before | 石走り Before |
|---|---:|---:|
| triangles | 74,626 | 205,172 |
| vertices（`rig-info.json`） | 38,040 | 103,834 |
| GLB POSITION entries（primitive合計） | 71,459 | 156,589 |
| mesh objects（制作 scene metadata） | 174 | 537 |
| dimensions XYZ (m) | 1.306174 × 0.330974 × 1.716839 | 7.062849 × 12.178528 × 9.397868 |
| bones | 18 | 20 |
| gameplay animations | 10 | 5 |
| materials | 13 | 14 |
| BaseColor | 2048² / 4,444,361 bytes | 2048² / 4,164,399 bytes |
| Normal | 2048² / 4,862,775 bytes | 2048² / 4,403,683 bytes |
| Roughness atlas | **なし** | **なし** |
| texture total | 9,307,136 bytes | 8,568,082 bytes |
| recorded weight validation | pass（今回の実変形再検証は未実施） | pass（今回の実変形再検証は未実施） |

白面の clip は Idle / Walk / Run / Slash / Dodge / Climb / Hang / Grip / Jump / Death、
石走りは Idle / Walk / Charge / Buck / Calmed である。Animated GLBには skin 1件、18/20 joints、
上記 clip が実在することを確認した。ただし肩、肘、手首、股関節、膝、足首、前後脚、首、顎の
今回の目視変形確認や未weight頂点の再走査ではない。

### Gameplay anchors

既存制作座標（m）の3禍根は `(2.18,-1.55,6.11)`, `(0,0.60,7.84)`,
`(-1.22,3.05,6.88)`。runtime の cm 座標と一致する。runtime route は次の11地点であり、
node 0 に Z + 90 cm を加えた位置が Grab marker、node 3以降に Z - 18 cm を加えた位置が
ledge collision である。

```text
(-270,-215,65), (-292,-210,200), (-288,-202,340), (-213,-198,485),
(-178,-100,591), (-95,-48,685), (0,55,770), (36,218,714),
(-122,305,668), (218,-155,593), (140,-60,666)  [cm]
```

今回 After mesh がないため、3禍根、route、Grab、岩棚、collision、白面身長、skeleton origin
の Before/After 数値整合は **UNVERIFIED** である。

## 実行結果

最初の production stage を次の順序で開始した。

```bash
blender --background --factory-startup --python Tools/CreateCharacterModels.py
```

結果は exit 127、`blender: command not found`。失敗後に Refine / Rig / Verify へ進まず、後段を
成功扱いにしていない。また公式 archive の HTTPS download も 403 だった。そのため結果は以下。

| stage | 結果 |
|---|---|
| CreateCharacterModels | **BLOCKED** — executableなし |
| RefineCharacterModels | **NOT RUN** — Create失敗で停止 |
| RigCharacterModels | **NOT RUN** |
| VerifyRiggedCharacters | **NOT RUN** |
| Texture bake | **UNVERIFIED** |
| Studio preview | **UNVERIFIED**、既存画像をAfterとして再利用していない |

正式CC0 PBR cacheは存在せず、strict経路は使用できない。既存生成設計の fallback を用いるべき
状態だが、生成自体が開始できなかった。現行assetに BaseColor / OpenGL Normal はある一方、
Roughness atlasはない。したがって3 atlas生成、OpenGL法線、UEでのGreen Channel flipを今回の
成果として主張しない。

## After comparison と外観評価

Afterは存在しないため、全差分と5段階評価は **N/A (UNVERIFIED)** とする。旧previewより改善したか
は白面 **NO（新規成果物なし）**、石走り **NO（新規成果物なし）**。これは見た目の劣化判定では
なく比較対象を生成できなかったという判定であり、既存assetを保護するため採用しない。

| 指標 | Before | After | 差 |
|---|---:|---:|---:|
| 白面 triangles | 74,626 | N/A | N/A |
| 白面 vertices | 38,040 | N/A | N/A |
| 白面 dimensions | 1.306174 × 0.330974 × 1.716839 m | N/A | N/A |
| 白面 bones | 18 | N/A | N/A |
| 石走り triangles | 205,172 | N/A | N/A |
| 石走り vertices | 103,834 | N/A | N/A |
| 石走り dimensions | 7.062849 × 12.178528 × 9.397868 m | N/A | N/A |
| 石走り bones | 20 | N/A | N/A |
| Material数 | 13 / 14 | N/A | N/A |
| Texture容量 | 9,307,136 / 8,568,082 bytes | N/A | N/A |

白面の silhouette / mask / cloth / left-arm corruption / sword / material response / animation
deformation / game readability、および石走りの silhouette / scale impression / anatomy /
rock-body integration / fur / moss-weathering / Kakon readability / material response / climbing
readability は全て **N/A**。既存画像だけからAfter評価を捏造しない。

## UE 5.6 / High Quality

Windows UE 5.6、D3D12がないため、通常参照先 `/Game/Characters/Rigged/Shirotsura` と
`/Game/Characters/Rigged/Ishibashiri` への再import、material apply、asset/animation length、
compression、missing texture、pink material、material node冪等性、Build、gameplay regression、
FPS、High Quality起動は全て **UNVERIFIED**。D3D12 / SM6 / Lumen GI / Lumen Reflections /
VSM / Sky Atmosphere / Volumetric Fog / Bloom / Exposureもログ確認できていない。

Windows UE 5.6環境では repository root から次を順に実行する（`Prototype.ps1` の engine検出を
使用する）。import/applyに直接 `UnrealEditor-Cmd` を使う場合も project と script は絶対pathに
解決すること。

```powershell
$EngineRoot = if ($env:UE_ROOT) { $env:UE_ROOT } else { 'C:\Program Files\Epic Games\UE_5.6' }
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
& $Editor (Resolve-Path .\IshibashiriPrototype.uproject) -run=pythonscript -script="$((Resolve-Path .\Tools\ImportRiggedCharacters.py).Path)" -unattended -nop4 -UTF8Output
& $Editor (Resolve-Path .\IshibashiriPrototype.uproject) -run=pythonscript -script="$((Resolve-Path .\Tools\ApplyRiggedMaterials.py).Path)" -unattended -nop4 -UTF8Output
& $Editor (Resolve-Path .\IshibashiriPrototype.uproject) -run=pythonscript -script="$((Resolve-Path .\Tools\ApplyRiggedMaterials.py).Path)" -unattended -nop4 -UTF8Output

.\Tools\Prototype.ps1 -Action Build
.\Tools\Prototype.ps1 -Action Test -Climbing -Basin -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Climbing -Basin -SkipBuild -TestFPS 30
.\Tools\Prototype.ps1 -Action Test -ClimbingGamepad -Basin -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Grab -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Camera -Basin -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -BasinScenario -Basin -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Play -Basin -HighQuality
.\Tools\Prototype.ps1 -Action Test -Climbing -Capture -Basin -HighQuality -SkipBuild -TestFPS 60
```

注意: UEを標準外へ導入した場合は、`Prototype.ps1` と同様に `UE_ROOT` をそのengine rootへ設定する。
コマンドラインに `HighQuality` を渡しただけでは合格にせず、
実行logのRHI、feature level、GI/reflection method、shadow methodと実測FPSを証跡へ残す。

Studio previewの予定保存先は `Art/Characters/Previews/`、UE画面は
`Art/Characters/Previews/HighQuality/`。今回はどちらも更新していない。

## 再開可能なCI経路

手動workflow `.github/workflows/character-regeneration-validation.yml` を追加した。公式checksumで
Blender 3.6.23を検証し、baselineを先に退避して Create → Refine → Rig → Verify を fail-fastで
実行し、ログとBefore/After候補を14日Artifactとして保存する。生成物を自動commitしないため、
ダウンロード後に同一camera/lightのStudio比較、関節変形、寸法・anchor、Windows UE検証を人が
完了するまでmain assetは保護される。

## 未解決リスクと次の3D上の弱点

1. **Roughness情報** — 現行binaryには専用atlasがなく、木・布・皮膚・岩のmaterial responseを
   UE 5.6で分離評価できない。3 atlasの実生成とcompression確認が最優先。
2. **石走りの岩・身体・根の接続** — 新Refineを描画できておらず、「背中に岩を載せた猪」から
   脱したか不明。route/ledgeを固定した同一camera比較が必要。
3. **変形時の読みやすさ** — 白面の肩～手首と登攀姿勢、石走りの前後脚・首・顎について、全15
   gameplay clipのweight/変形を描画確認できていない。未weight 0とanchor不変を同時に検証する。

Gameplay code、Control Rig/FBIK plumbing、Motion Warping、route、collisionには変更を加えて
いない。ただし新assetを実gameplayで試せていないため、Gameplay互換性は「維持を確認」ではなく
**UNVERIFIED** である。
