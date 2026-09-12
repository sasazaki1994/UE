# 白面モデル再制作の作業保存

参考画像に沿った白面の再制作コードと、現在の途中生成物を保存するDraftです。
完成モデルの採用・検証を示す変更ではありません。

## 保存した内容

- `Art/Characters/Shirotsura/References/` に正面画像と四面図を保存。
- `RebuildReferenceShirotsura.py`、`ReferenceShirotsuraHead.py`、
  `ReferenceShirotsuraCostume.py` に、白い面・結い髪・黒い重ね衣・裂けた裾・
  縄帯・札・鈴・左腕の枝と亀裂・手足・刀の生成処理を追加。
- `PolishCharacterSurfaces.py` に素材別の手続き型表面を追加。
- リグ生成に部位指定、長い衣のウェイト、キャラクター選択、2048/4096pxの
  アトラス選択を追加。既定値は従来の両キャラクター・2048px。
- UE取り込み・マテリアル適用・監査に `CHARACTER_ASSET_FILTER` を追加。
  マテリアルの番号が再利用されても古い発光・金属値を残さないようにした。
- `VerifyCharacterQuality.py` と `VerifyCharacterQualityUE.py` に、
  骨格・ウェイト・ポーズ・画像・取り込み設定・再適用の検査を追加。

## 生成物の状態

この変更に含まれる `Shirotsura.blend` / `.fbx` / `.glb` と正面・背面プレビューは、
既存 `CreateCharacterModels.py` を実行した段階の途中生成物です。
**新しい参考画像用の生成コードの完成出力ではありません。**
`model-info.json` は旧モデル75,394三角形に対して途中生成物32,194三角形を記録しています。
この減少を品質向上や最適化の成果とは扱いません。

リグ済みモデル、ベイク済みテクスチャ、`Content/` のUEアセットは今回の変更では
更新していません。静的モデルとリグ済みモデルの制作段階は一致していません。

Tripo3Dで指定されたモデル
`984b85eb-d79c-4352-b34f-2aeafdfad927` は共有ページへのアクセスが自動レビューで
拒否されたため未取得・未取り込みです。GLBまたはFBXとテクスチャの受領後に、
実際のメッシュとリグの構成を確認して取り込みを再開します。

## 確認済みと未確認

- Pythonの既存ユニットテスト10件成功。
- `Tools/ValidateCharacterRefinement.py` が `SOURCE_CONTRACT_PASS`。
  追加した生成・検証スクリプトの構文検査を含む。
- 頭部と衣装の単独Blender試作は描画済み。全身統合、最終外観、アトラスの
  ベイク、全アニメーションの変形、UE再取り込みは未検証。
- 変更前のUEモデルでCamera描画テストは終了コード0
  （RunId `093e173a200a4434b7dad027cfb6dd89`）。
  Climbingは既存のphase 2で失敗し、終了時のシェーダー作業領域問題で終了コード3
  （RunId `36990ea465874c468ab6008ab964f7f0`）。
  どちらも新モデルの動作確認ではありません。

変更前ファイルのローカル退避先は
`Artifacts/CharacterQuality/20260911-polish/Before/`。
この退避データ、試作ログ、PythonキャッシュはPRに含めません。
