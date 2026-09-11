# 白面・石走り 生成・描画検証（2026-09-11）

## 対象と事前確認

- 作業開始時の HEAD は `bffe196`（ローカル `work`）で、作業ツリーはクリーンだった。
  `.git/config` に remote がなく、GitHub CLI は未認証、GitHub API はプロキシから
  HTTP 403 となったため、GitHub 上の最新 `main` と未マージ PR は照合できなかった。
  ローカル履歴では PR #22 のマージ `b5dab85` と修正 `7036d53` を含む。
- `Docs/CharacterMaterialLibrary.md`、`Docs/CharacterVisualRefinement.md`、外観・登攀の
  acceptance spec、Create / Refine / Rig / Verify、および PBR bake 検証スクリプトを確認した。
  既存 acceptance spec は11地点のルート、棚、3禍根、浄化、再挑戦を既に要求しており、
  今回は実行環境の不足だけが阻害要因なので変更していない。
- Linux `6.18.35`、Python `3.12.13`。`blender`、`bpy`、`UnrealEditor` / 
  `UnrealEditor-Cmd` は存在しない。`Art/Materials/CC0PBR/files/` も存在せず、正式
  manifest は3用途9 mapすべて未解決のままである。

## 実行結果

要求された最初の実描画ゲートを次で実行した。

```bash
blender --background --factory-startup --python Tools/ValidateCharacterPBRBake.py
```

終了コードは `127`、結果は `blender: command not found` だった。これは bake 中の
Python例外ではなく実行系自体の欠如である。このゲートを通過していないため、順序を
守って Create → Refine → Rig → Verify は実行せず、モデル、テクスチャ、UE assetを
上書きしていない。従って退避対象となる変更後生成物もなく、既存の手修正成果物は保持
されている。

ホスト上で実行できる検査は次の結果だった。

| コマンド | 結果 |
|---|---|
| `python -m unittest discover -s Tests -p 'test_*.py' -v` | 10件成功 |
| `python Tools/ValidateCharacterRefinement.py` | `SOURCE_CONTRACT_PASS` |
| `python Tools/ManageCharacterPBR.py verify` | 終了1、`PBR_CACHE_INVALID`（3用途9 map、公式ID/URL、license、hashが未取得） |
| `python -m py_compile Tools/*.py` | 成功。ただし描画検証ではない |

## 生成物と比較画像

新しいモデル、atlas、UE asset、比較画像は生成していない。既存 preview を変更後画像と
偽装せず、外部PBR導入成功とも造形改良生成成功とも扱わない。対象の `.blend` / `.fbx`
と preview の最終更新はローカル履歴上 `0220cf3`（2026-09-09）で、造形改良
`346e176` より前である。このため、現在コミットされている画像は今回の改良を反映した
比較証跡ではない。

## 未実施事項と再開コマンド

Blender不在のため、チェック柄と凹凸方向、`PBRDetailUV` / `AtlasUV`、法線合成、二度
適用時のノード数、同一条件での白面3カット・石走り3カット、15クリップ中の変形を未確認
とした。UE不在のため、再取り込み、マテリアル二度適用、描画ありの11地点・棚・3禍根・
浄化・再挑戦も未確認である。ネットワーク遮断により外部PBRは未取得なので、Blenderが
利用可能になった時点ではまず人工fixtureの bake を完走し、その後は正式PBR成功と区別
して手続き型マテリアルによる**造形改良**を生成する。

```bash
blender --background --factory-startup --python Tools/ValidateCharacterPBRBake.py
blender --background --factory-startup --python Tools/CreateCharacterModels.py
blender --background --factory-startup --python Tools/RefineCharacterModels.py
blender --background --factory-startup --python Tools/RigCharacterModels.py
blender --background --factory-startup --python Tools/VerifyRiggedCharacters.py
```

UE 5.6.1 がある環境では、その生成物を退避・比較した後に既存の import / apply を二度
実行し、`Docs/ClimbingValidation.md` の描画あり60 FPS検証を実施する。ツール不足を補う
ための別の検証基盤や人工の正式素材は追加していない。
