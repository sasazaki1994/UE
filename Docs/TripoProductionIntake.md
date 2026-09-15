# Tripo Production Intake — 白面・石走り

基準: `dac60cb41979254388955b75b97c939c90695701`（2026-09-14に取得した `origin/main`）。
現在の両者の状態は **BLOCKED — TRIPO SOURCE REQUIRED**。既存Blenderモデル、Primitive、`CharacterModels.zip` は比較用Fallbackであり、Tripo生成物として登録していない。

## 入力と成果物

```text
Art/Characters/Production/
  Shirotsura/                       # Ishibashiri/ も同じ構造
    manifest.json
    TripoSource/                    # 原本GLB/GLTF/FBX/BLEND + 全sidecar、加工しない
    Blender/                        # *_Clean.blend、*_Rigged.blend
    Export/                         # SK_<name>.fbx
    Textures/                       # T_<name>_{BaseColor,Normal,Roughness}.png
    Validation/                     # preflight/cleanup/rig/export/ue-import/adoption.json
      rig-review.template.json      # artistによる実物レビューの雛形
      review.template.json          # 回帰、同条件画像、性能、8項目採点の雛形
```

`manifest.source_file` は `TripoSource/` からの相対パス。空欄/nullは「未提供」、指定したファイルがない場合、空ファイル、未登録の入力、壊れたファイルは「不正な入力」。`--allow-missing` がCIで許容するのは前者だけ。生成者/Tripo task IDは `generated_by`、実際の生成日時は `generation_date` に記入する。出所を推測して埋めない。

原本寸法はBlenderで評価したXYZメートル。元のunitも検査レポートに残る。実物がない値はnull。`adopted=false` はImport成功後も維持され、最終レビューを通すまで変更しない。ファイルだけでなく `TripoSource/` 内の全sidecarをSHA-256で追跡する。入力変更で後段の検証は無効になる。不要な資料は原本フォルダに混ぜない。

## 実行環境と最初のコマンド

Python 3.10+、Blender **3.6.23**、Windows UE **5.6.1** で確認。UE PythonはEditor内で実行する。以下はリポジトリ直下からのPowerShell。通常の `python` / `blender` がPATHにあれば代用できる。

```powershell
$IntakePython = "$env:USERPROFILE\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
$IntakeBlender = "$PWD\.cache\blender-3.6.23-windows-x64\blender.exe"
$IntakeEditor = "$env:USERPROFILE\UnrealEngine\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$IntakeProject = "$PWD\IshibashiriPrototype.uproject"

# 現状のSource missingを正しく確認。実物がある不正な入力は成功扱いにならない。
& $IntakePython -X utf8 Tools/ValidateProductionCharacter.py --character Shirotsura --allow-missing
& $IntakePython -X utf8 Tools/ValidateProductionCharacter.py --character Ishibashiri --allow-missing
```

実物を受領したら、白面は `Shirotsura/TripoSource/Shirotsura.glb`、石走りは `Ishibashiri/TripoSource/Ishibashiri.glb` へ置く。各manifestの `source_file` にそのファイル名、`generated_by` にTripo task/export ID、`generation_date` に実際のISO日時を記入する。FBX/GLTFの場合は拡張子も更新し、.binとTextureを元の相対パスで同梱する。GLBのTexture埋め込みを推奨する。

```powershell
& $IntakePython -X utf8 Tools/ValidateProductionCharacter.py --character Shirotsura --blender $IntakeBlender
& $IntakePython -X utf8 Tools/ValidateProductionCharacter.py --character Ishibashiri --blender $IntakeBlender
```

Exit code: `0` preflight pass/明示的に許容したmissing、`2` invalid、`3` source missing、`4` Blender unavailable。Blender内部例外は `--python-exit-code 2` で失敗させる。`pass` はその工程だけの成功であり、本番採用ではない。

## Preflightとcleanup

検査対象: 実ファイル/サイズ、Object/Mesh/Vertex/Triangle/Material/Texture数、画像参照とサイズ、全体Bounds/Dimensions、Origin/Rotation/Scale、UVの面積、評価済みNormals、骨名/骨数、Animation、loose vertex、Non-manifold edge、孤立した連結成分、退化三角形、不正座標。GLB/GLTFは原本のNORMAL attributeの欠落も確認。FBX/BLENDはImporterが補った法線と元法線を区別できないため、その制限をWarningに残す。

白面高さ1.55–1.90m（目標1.72m）、石走り高さ7.5–10.5m・全長10–14mを入口の範囲とする。実際の合否は既存Meshと全軸Bounds/Originの比較、足場・Grab・身体への一致で決める。5cm/100mの白面、1kmの石走り、Scale .001/負値、欠損Texture、Geometryなしを拒否する。異なる姿勢/向きで範囲外なら原本に手を加えずcleanupの作業コピーで修正する。

```powershell
& $IntakeBlender --background --factory-startup --disable-autoexec --python-exit-code 2 --python Tools/ProductionCharacterBlender.py -- cleanup --character Shirotsura
& $IntakeBlender --background --factory-startup --disable-autoexec --python-exit-code 2 --python Tools/ProductionCharacterBlender.py -- cleanup --character Ishibashiri
```

Blender座標はメートル、Z up、**-Y forward**、床/足裏をZ=0とする。`--scale 0.01` は原本unit換算後の明示的な一様縮尺、`--yaw 180` はZ軸回転、`--offset X Y Z` はメートルの原点補正。自動で身長に合わせたり、石走りRouteを移動したりしない。正面が+Z等ならBlenderで手動で正しく立てたunrigged作業ファイルを別の原本として登録し、元の出所をnotesで追跡する。

cleanupは評価Meshを作業コピーへ取り出し、親/TransformをGeometryへ適用、unitを1mへ統一、1e-7m以内の重複頂点と孤立頂点を除去、面法線再計算、未使用/重複参照Material slot整理、Object命名、Camera/Light等の不要Object除去、画像packを行う。Non-manifold/離れた成分は札・衣・岩等の意図的構造もあるので警告を見て手動判断。UVを自動で上書きしない。**Decimateは実行しない**。

原本にArmatureがある場合はpreflight可能だが、cleanupは停止する。元Rigを `TripoSource` に保存したまま、unrigged rest-pose exportを用意して登録する。Tripo Rigを自動でGameplay Rigとして採用しない。

## Rig / Weight / Export

```powershell
& $IntakeBlender --background --factory-startup --disable-autoexec --python-exit-code 2 --python Tools/ProductionCharacterBlender.py -- rig --character Shirotsura
& $IntakeBlender --background --factory-startup --disable-autoexec --python-exit-code 2 --python Tools/ProductionCharacterBlender.py -- rig --character Ishibashiri
```

既存 `Art/Characters/<name>/Rigged/<name>_Rigged.blend` からArmatureとActionを再利用し、既存Meshから新Geometryへ近傍面補間で仮ウェイトを転送する。転送元Geometryは候補から除去する。白面の18骨・10Action、石走りの20骨・5Action、bone名/親/rest poseは維持する。自動転送は出発点であり、衣裳・武器・手足・岩・根の適切なWeightを保証しない。

Blenderで `*_Rigged.blend` を開き、肩/肘/手首/股関節/膝/足首、武器と左手、全Actionを確認しWeightを修正。骨位置を動かす必要がある場合は、既存SkeletonへRetargetしてから戻す。Gameplay Animation、突進/Buck/Calmの秒数、Root Motionへ合わせてGameplayを作り直さない。

`Validation/rig-review.template.json` を `rig-review.json` にコピーし、実物を検査したレビュー担当者・`rigged_sha256`・各checksを記入する。Hashは次のコマンドで取得する。自動でtrueにする手順はない。

```powershell
(Get-FileHash Art/Characters/Production/Shirotsura/Blender/Shirotsura_Rigged.blend -Algorithm SHA256).Hash.ToLower()
# レビュー完了後
& $IntakeBlender --background --factory-startup --disable-autoexec --python-exit-code 2 --python Tools/ProductionCharacterBlender.py -- export --character Shirotsura
& $IntakeBlender --background --factory-startup --disable-autoexec --python-exit-code 2 --python Tools/ProductionCharacterBlender.py -- export --character Ishibashiri
```

Exportはclean/source/baseline RigのHash、レビュー対象Hash、骨階層・rest pose・Weight合計・全Actionの変形を再確認する。Jump等の定常poseはフレーム間変化でなくrest poseとの差で検査する。既存 `RigCharacterModels.bake_surface` のUV/Atlas経路とFBX出力規約を使う。Tripo Materialを別のCC0 PBRで置換せず、2048px BaseColor/Normal/RoughnessへBakeする。既存 `CharacterPBR` の通常Fallback生成への適用は保持する。全ActionのGeometry変形は確認するが、美的な変形品質は人が判断する。

## UE ImportとVisual切替

実物のcleanup/rig/exportレビューが通った場合だけ実行する。未提供状態ではImportしない。

```powershell
$env:PRODUCTION_CHARACTER = 'Shirotsura'
& $IntakeEditor $IntakeProject -run=pythonscript "-script=$PWD\Tools\ImportProductionCharacter.py" -unattended -nop4 -nullrhi -UTF8Output
$env:PRODUCTION_CHARACTER = 'Ishibashiri'
& $IntakeEditor $IntakeProject -run=pythonscript "-script=$PWD\Tools\ImportProductionCharacter.py" -unattended -nop4 -nullrhi -UTF8Output
Remove-Item Env:PRODUCTION_CHARACTER
```

`ImportRiggedCharacters.run` と `ApplyRiggedMaterials.apply_character_materials` を共有する。既存Fallbackでの単体スクリプト起動も維持。候補は毎回固有の `/Game/Characters/Production/<name>/Intake_<timestamp>` へ新規Importし、既存Skeleton/Actionを共有する。Physics Assetも生成するが、実行時MeshはNoCollisionのまま。

SkeletalMesh、Skeleton、骨数/親、weapon/hand/foot/core骨、Physics Asset、全Materialの3マップ参照、TextureのsRGB/Normal compression、Bounds/Origin、Animationと長さを検査してから、Meshのみ次の固定パスへ移す。

- `/Game/Characters/Production/Shirotsura/SK_Shirotsura`
- `/Game/Characters/Production/Ishibashiri/SK_Ishibashiri`

Missing/未接続MaterialやTextureは拒否する。シェーダーコンパイルエラーによるPink表示は描画比較で拒否する。Importの静的検査だけでは描画成功を宣言しない。Import途中の失敗は固定パスを公開せず、調査用stagingを残す。既存の固定候補がある場合は上書きを拒否するので、Editorで旧候補を別名へ退避してから再実行する。

```powershell
.\Tools\Prototype.ps1 -Action Play -Basin -HighQuality -SkipBuild -ProductionVisuals 0
.\Tools\Prototype.ps1 -Action Play -Basin -HighQuality -SkipBuild -ProductionVisuals 1
```

起動引数 `-ProductionVisuals=0/1`、個別に `-ShirotsuraProductionVisuals=0/1`、`-IshibashiriProductionVisuals=0/1`。個別値が優先する。**起動時選択**であり、動作途中で切替えるConsole CVarではない。再起動して比較する。`PRODUCTION_VISUAL_SELECTED <name>` が実候補の使用証拠。`PRODUCTION_VISUAL_FALLBACK` は候補未使用なので候補Regressionの成功として数えない。

RuntimeでもSkeleton同一性、全bone階層/rest pose、Material欠損、各軸Bounds比0.75–1.25とOrigin差を再検査する。変更するのは同じComponentのSkeletal MeshとMaterial overrideだけ。Transform、子Actor、Animation選択、Collision、状態/Timerへ書き込まない。後半3戦は別のPlayer/Bossクラスであり、この切替対象ではない。

Rollbackは `ProductionVisuals=0` で再起動。通常の参照先は常に `/Game/Characters/Rigged/` のまま。配布パッケージで候補を使う際は検証済みProductionフォルダをCook対象へ明示追加し、Cook済み実機で候補選択ログを再確認する。このSTEPでは候補なしでCook設定を変更しない。

## Gameplay / Anchor契約

白面: 約172cm、白い面、墨〜濃紺の山仕事/祓い装束、穢れた左腕、境断ち、腰縄、少量の札と鈴。`weapon` は境断ちとBoundary Sense、`hand_L` はCorruption Sense、`hand_L/R`・`foot_L/R` は登攀接触。既存Sense proxyは通常非表示、Sense中のみ反応し、新Meshに重複proxyを追加しない。地上移動/Jump/Dodge/Slash/Grab/Climb/Hang/Clingは現在の処理とAnimationを使う。

石走り: 「猪+岩山+苔+根+古い注連縄+局所的な禍」の歩く山。胴と岩と根を統合したシルエットにし、普通の猪に岩を載せただけにしない。大量の赤発光、SF、金属重装甲、ドラゴン、死亡崩壊表現は不可。

Gameplay AnchorはMeshの**骨ではなくComponent frame**を基準にしている。新GeometryへRouteを埋め込まない。新Meshの原点/寸法をこのframeに合わせる。

| Node | Mesh local cm (X,Y,Z) | 接続 |
|---|---|---|
| 0 | -270,-215,65 | 1 |
| 1 | -292,-210,200 | 2,0 |
| 2 | -288,-202,340 | 3,1 |
| 3 | -213,-198,485 | 4,2 |
| 4 | -178,-100,591 | 5,3 |
| 5 | -95,-48,685 | 6,4,10 |
| 6 | 0,55,770 | 7,5 |
| 7 | 36,218,714 | 8,6 |
| 8 | -122,305,668 | 7 |
| 9 | 218,-155,593 | 10 |
| 10 | 140,-60,666 | 9,5（上下/左右双方） |

禍根座標: `(218,-155,611)`, `(0,60,784)`, `(-122,305,688)` cm。Actor Capsule半径310/半高350cm、Creature位置(0,0,-350)、Yaw90°、主要ledgeはNode 3–10のZ-18、Box extent(65,65,18)を維持。Rest判定/Grab条件/Route followerも既存コードが唯一のAuthority。

`AKakonActor` とNushi ProgressがCovered/Exposed/Purified、3/3、Calmを所有。新Geometryに禍根本体を焼き込まない。`core_0..2` は互換性のため残すが、新しい進行状態を作らない。Purifyの短い0.35秒PresentationとCalm Action、Victory、Retryを維持する。石走りはshell health=0のためCoveredは通常進行で到達しない。専用のPresentation検査でCoveredを評価し、その場面を通常Gameplay達成の証拠にしない。

## 回帰とHigh Quality比較

```powershell
python -X utf8 -m pytest Tests -q
& $IntakeBlender --background --factory-startup --disable-autoexec --python-exit-code 2 --python Tests/production_blender_smoke.py
.\Tools\Prototype.ps1 -Action Build
& $IntakeEditor $IntakeProject -unattended -nop4 -nosound -nullrhi '-ExecCmds=Automation RunTests IshibashiriPrototype' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$PWD\Saved\ProductionIntake\Automation"

# 候補がある場合は0/1それぞれを実行し、実候補選択ログを必ず検査する。
.\Tools\Prototype.ps1 -Action Test -SkipBuild -ProductionVisuals 0 -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Climbing -SkipBuild -ProductionVisuals 0 -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Climbing -SkipBuild -ProductionVisuals 1 -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Climbing -SkipBuild -ProductionVisuals 1 -TestFPS 30
.\Tools\Prototype.ps1 -Action Test -Grab -SkipBuild -ProductionVisuals 1
.\Tools\Prototype.ps1 -Action Test -BasinScenario -Basin -SkipBuild -ProductionVisuals 1
.\Tools\Prototype.ps1 -Action Test -Climbing -Capture -HighQuality -SkipBuild -ProductionVisuals 0
.\Tools\Prototype.ps1 -Action Test -Climbing -Capture -HighQuality -SkipBuild -ProductionVisuals 1
```

Automation結果は終了コードだけでなく `index.json` のfailed/notRunと実行件数を検査する。UEはテスト失敗でも終了コード0の場合がある。`-Campaign` の既存launcherはEditorContextテストをgameモードで呼ぶ制限があるため、Campaign unit回帰は上記Editor Automationコマンドを使う。

白面: Idle/Walk/Run/Jump/Slash/Dodge/Grab/Climb/Hang/Cling/Boundary Sense/Corruption Sense。石走り: Spawn/Charge/Recovery/Grab/11 Route/branch/rest/stamina/shake/buck/cling/fall/3 Kakon/Covered検査/Exposed/Purified/3of3/Calm/Victory/Retry。既存テストは内部APIで状態を進める検査と入力模擬を含むため、人間の操作や全経路の攻略と区別する。

Campaignは `-Action Play -Campaign` で石走り→淵纏い→峰抱き→禍津根を通し、各戦終了とRetryを確認。Automationの状態機械テストだけで全マップ遷移・実操作のCampaignが成功したとみなさない。後半3戦のVisualはそのまま。候補で成功率が下がれば失敗とする。

High QualityはWindows UE5.6 DX12/SM6/Lumen/VSM。両variantで同じMap、カメラTransform、FOV、光源、時刻、露出設定、解像度、固定動作時刻、GPU/Driverを記録する。白面6視点（Idle/Back/Run/Slash/Sense/Climb）、石走り8視点（front 3/4/side/back/Charge/Grab/Climb/Kakon/Calm）を `Validation/Captures/` 以下に対で保存。自動Climbing撮影で不足する視点は同じ固定Cameraで追加する。

## 性能・採用

preflight/exportのVertex/Triangle/Material数、画像サイズからの概算非圧縮Texture bytesを保存する。これはGPU常駐Texture memoryではない。UEの `stat unit`、`stat RHI`、`stat streaming`、`stat startfile` / `stat stopfile`、Unreal Insightsで実測する。ウォームアップ後、同一60秒シナリオを3回ずつ実行し、中央値FPS、frame time p50/p95、draw calls、texture memory、成功数/試行数を記録する。計測時はFPS cap/VSyncを同一設定にし、性能測定と固定FPSのGameplay検査を分ける。

入口のGeometry予算: 白面100k triangles、石走り350k、Materialは16以下。自動採用せず、frame p50/p95が10%以上悪化、Texture memory25%以上増、draw calls20%以上増ならREJECT。対象機材で予算を変える際も変更理由と再測定が必要。元のmainの数値は静的メタデータ（白面32,194 triangles、石走り205,442 triangles）であり、今回の新しいUE実測Beforeとして扱わない。

`review.template.json` を `review.json` にコピーし、実測・画像Hash・回帰結果・レビュアーと各1–5点を記入する。8項目: silhouette/material quality/texture quality/animation deformation/game readability/gameplay compatibility/performance/overall production readiness。

```powershell
& $IntakePython -X utf8 Tools/ReviewProductionCharacter.py --character Shirotsura
& $IntakePython -X utf8 Tools/ReviewProductionCharacter.py --character Ishibashiri
# 全ゲートを通過し、採用を記録する場合だけ --adopt を追加。
```

- **BLOCKED**: Source/Import/回帰/画像/計測/レビュー不足。未評価点はnull。
- **REJECT**: Gameplay低下、変形破綻、性能予算超過、最低品質不足。
- **CONDITIONAL**: Gameplayと性能は通過したが品質に3点項目がある。修正後再評価。
- **ADOPT**: 全証拠が現在のexportに対応、全品質4以上、Gameplay compatibility=5、全回帰成功、成功率維持、性能予算内。`--adopt` なしではadoptedフラグも切り替えない。

## 制限・今回の検証

実物へのRig調整とRetarget、美的判断、UE描画でのPink/欠損検査、実候補Gameplay、実候補性能比較、High Quality比較は実Source受領後。異種Skeletonの自動Retargetは提供しない。UDIM/特殊Shaderは標準3マップAtlasへ整理してから進める。編集でclean/rig/sourceが変わったら該当工程を再実行する。

既存 `CreateCharacterModels` / `RefineCharacterModels` / `VerifyRiggedCharacters` / `CharacterPBR` は保持する。Blender fixtureは明示的なcubeを一時フォルダに作り、format読み込み・cleanup・weight転送・変形・32pxのテストBake・FBX Exportを検査する。これはTripo候補でもVisual品質証拠でもない。

最新の実行結果、source有無、25項目の引き渡し記録は [TripoProductionIntakeValidation.md](TripoProductionIntakeValidation.md) を参照。
