# Tripo Intake 検証・引き渡し記録

検証日: 2026-09-15。基準は `origin/main` の `a7d69870914b8dbdb048aa7713f624946e79df35`。
白面・石走りともTripo原本は未提供。今回の成果は素材受入パイプラインであり、Production CandidateのImport・品質・採用が成功したことを示すものではない。

## 実行した検証

| 検証 | 結果 | 証拠（ローカル、Git対象外） |
|---|---|---|
| UE 5.6 Editor Build | 成功 | `Saved/ProductionIntake/build-pr-console.log` |
| Editor Automation `IshibashiriPrototype` | 34成功、警告付き成功0、失敗0、未実行0 | `Saved/ProductionIntake/Automation-pr/index.json` |
| Climbing入力回帰、60 FPS、ProductionVisuals=1 | 成功。両者の候補は存在せずfallbackを選択 | `Saved/Logs/ClimbingTest-60.log`、Run ID `e11166dc2ea443f6bdbcfa1c9ff75d34`、`CLIMB_TEST_PASS` 72.40秒（固定ステップのゲーム時間） |
| Python source / intake tests | 68成功、10 subtests成功 | `python -X utf8 -m pytest Tests -q` 相当 |
| Blender 3.6.23 fixture | GLB/GLTF/FBX/BLEND読込、負例、cleanup、仮ウェイト転送、10 Action変形、32px Bake/FBX Export成功 | `Saved/ProductionIntake/blender-smoke.json`、`blender-pr-console.log` |
| UE Python API smoke | 両fallbackで骨・Bounds・Import設定・Material APIの読取成功。Importなし | `Saved/ProductionIntake/ue-api-smoke.json`、`Saved/Logs/ProductionIntake-Api-PR.log` |
| 実Intake preflight、両者 | `missing_source`、`--allow-missing` 指定時のみexit 0 | 各 `Validation/preflight.json` |

Blender fixtureは一時ディレクトリの `TEST_FIXTURE_NOT_TRIPO` という検査用cube。UEのVisual切替テストもTransientなfallback複製を使い、偽のProduction Candidateを保存しない。これらを実素材の変形品質や見た目の証拠には数えない。

SourceテストはUTF-8で実行する。既存UEコードが実行時に `Config/DefaultInput.ini` を再保存するため、検証後に今回生成された設定差分を元に戻してからソーステストを再実行した。入力設定の変更はPRに含めない。

UE 5.6で既存コードのビルドを通すため、Editor依存、欠けていた型include、Unity翻訳単位の名前衝突、TObjectPtrのループ型、ログマクロの条件分岐括弧を最小修正した。未提供の任意Control Rig参照を存在確認で保護し、Sense/Minedakiのテストfixtureも実際に位置・距離条件を満たすよう修正した。戦闘・登攀の状態遷移やRoute座標は変更していない。

## 25項目の引き渡し

| # | 項目 | 結果 |
|---|---|---|
| 1 | Source commit | `a7d69870914b8dbdb048aa7713f624946e79df35`。最新mainの探索区間とCampaign E2Eを保持 |
| 2 | 作成・変更ファイル | `Tools/ProductionCharacter*.py`、`ValidateProductionCharacter.py`、`ImportProductionCharacter.py`、`ReviewProductionCharacter.py`、既存Rig/Import/Material/launcher helper、`ProductionVisuals.{h,cpp}` とテスト、`Tests/production*`・`test_production_intake.py`、`Art/Characters/Production/`、本書・手順書・CI。UEビルド/fixture修正の全パスはPR差分を参照 |
| 3 | Intake directory | `Art/Characters/Production/{Shirotsura,Ishibashiri}/{TripoSource,Blender,Export,Textures,Validation}/` |
| 4 | Manifest | 両キャラクター直下に `manifest.json`。出所はTripo AI、実物の値はnull、`adopted=false` |
| 5 | Validator | `Tools/ValidateProductionCharacter.py`。未提供と不正な原本を区別。実物はBlenderで幾何・UV・Material・Texture・寸法・Transform・骨を検査 |
| 6 | Blender cleanup | `Tools/ProductionCharacterBlender.py` のcleanup→rig→手動review→export。原本保存、明示的な軸/単位補正、重複/孤立頂点除去、法線、Material整理。自動Decimateなし |
| 7 | UE import path | `/Game/Characters/Production/Shirotsura/SK_Shirotsura` と `/Game/Characters/Production/Ishibashiri/SK_Ishibashiri`。staging検査後のみ公開 |
| 8 | Visual switching | 起動引数 `-ProductionVisuals=0/1`、キャラクター別override。0が通常fallback。1でも欠損・非互換ならfallback。再起動でrollback |
| 9 | 白面Tripo source | **未提供**。既存Riggedモデル・ZIPはfallbackとして扱う |
| 10 | 石走りTripo source | **未提供**。既存Riggedモデル・ZIPはfallbackとして扱う |
| 11 | 白面validation | `missing_source`。実形状の合否は未評価 |
| 12 | 石走りvalidation | `missing_source`。実形状の合否は未評価 |
| 13 | UE import結果 | 両候補とも未実行、**BLOCKED**。Importerが必要とする公開UE APIは既存モデルで確認 |
| 14 | Animation結果 | 既存Actionを共有する構成。fixtureで白面10 Actionの変形を機械検査。両実候補のUE変形・視覚評価は**BLOCKED** |
| 15 | Sense結果 | UEのSense AutomationとVisual切替状態保持テスト成功。ソース契約維持。実候補の武器/左手への視覚整合は**BLOCKED** |
| 16 | 石走り11 Route | 座標・接続を変更せずsource契約成功。fallbackのClimbing入力回帰成功。実候補の身体/足場一致は**BLOCKED** |
| 17 | 3 Kakon | 3個の座標・既存Actor Authorityを保持。Kakon/NushiのAutomation成功。実候補の焼込み重複・視認性は**BLOCKED** |
| 18 | Calm / Victory / Retry | fallbackのClimbing回帰と既存ライフサイクルAutomationで確認。実候補では未実行 |
| 19 | Campaign regression | 最新mainの状態遷移契約とCampaign Automation成功。統合後の入力駆動・全章通しE2Eは未実行。候補での回帰は**BLOCKED** |
| 20 | Performance比較 | **未実行**。実候補が必要。両variant各3回以上、FPS/frame p50/p95/draw calls/Texture memory/成功率を記録するgateを用意 |
| 21 | High Quality比較 | **未実行**。DX12/SM6/Lumen/VSM、同一カメラ・光源条件の対画像が必要 |
| 22 | 白面採用判定 | **BLOCKED — TRIPO SOURCE REQUIRED**、8項目採点はnull、`adopted=false` |
| 23 | 石走り採用判定 | **BLOCKED — TRIPO SOURCE REQUIRED**、8項目採点はnull、`adopted=false` |
| 24 | BLOCKED項目 | 実物preflight/cleanup/手動Rigレビュー/Import、実候補Animation・Sense・Route・Kakon・全Campaign回帰、性能/HQ比較、採用 |
| 25 | 次に生成・配置するファイル | 白面: `Art/Characters/Production/Shirotsura/TripoSource/Shirotsura.glb`。石走り: `Art/Characters/Production/Ishibashiri/TripoSource/Ishibashiri.glb`。Texture埋込み、unrigged rest pose推奨。各manifestに実ファイル名・Tripo task/export ID・生成日時を記入。FBX/GLTFならTexture/.binを同じ相対構造で同梱 |

## 素材受領後

[TripoProductionIntake.md](TripoProductionIntake.md) のコマンドを順に実行し、実物に対する `rig-review.json`、対画像と回帰・性能値を含む `review.json` を記録する。`ReviewProductionCharacter.py --adopt` は現在の出力と全証拠のgateを通過した場合のみ採用を記録する。
