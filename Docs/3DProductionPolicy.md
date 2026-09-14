# 3D制作方針 — Tripo AI採用

## 方針

『禍祓い』の本番用3Dモデル制作では、**Tripo AIを主要な3D生成ツールとして使用する**。

対象は主人公「白面の祓い手」、第一の主「石走り」、第二の主「淵纏い」、第三の主「峰抱き」、最終局面「禍津根」、および必要な小物・環境用3Dアセットとする。

既存のBlender/Python生成モデルやPrimitiveモデルは、Gameplay検証、サイズ確認、登攀Route検証、Collision検証、比較用Baselineとして維持する。ただし、本番Visualの最終到達点としては扱わない。

## 基本パイプライン

本番3Dは原則として次の流れで制作する。

1. コンセプト画像・正面/側面/背面などのリファレンスを準備する。
2. **Tripo AIで3Dモデルを生成する。**
3. Blenderで形状確認、スケール調整、不要Geometry除去、Topology/UV/Material整理を行う。
4. 必要に応じてRig、Weight、Animation対応を行う。
5. UE5へImportし、PBR Material、Collision、Socket、Grab/Climbing用Anchorとの整合を確認する。
6. 既存Gameplayの寸法・登攀Route・禍根位置を壊していないことをRegressionで確認する。
7. High Quality（DX12 / SM6 / Lumen / VSM）環境で最終Visualを確認する。

## キャラクター・主ごとの扱い

### 白面の祓い手

Tripo AIで本番モデルを生成し、白面、濃色衣装、穢れた左腕、境断ち、腰縄という主要シルエットを維持する。

既存Skeleton、Animation、Grab / ClimbingのGameplay要件へ接続できるよう、Blender側で必要な調整を行う。

### 石走り

Tripo AIで「巨大猪 + 山 + 岩盤 + 苔 + 根 + 注連縄」の本番Visualを作る。

ただし、現在確立している11地点の登攀Route、Grab地点、3禍根、岩棚、Collision基準はGameplay契約として優先し、Visual側をGameplayへ合わせる。

### 淵纏い

現在のPrimitive Vertical SliceをGameplay基準とし、Tripo AIで巨大な大蛇の本番モデルを制作する。

噛みつき、頭部Grab、巻き付き、蛇と岩棚・岩柱を往復するRouteを壊さないことを優先する。

### 峰抱き

現在のPrimitive完成主戦をGameplay基準とし、Tripo AIで巨大な山猿の本番モデルを制作する。

脚、背中、腕橋、肩、首、頭頂までの身体変形RouteとCling判定を維持する。

### 禍津根

禍津根は第四の巨大生物ではなく「生きて動く地形」として扱う。

Tripo AIは黒い根、岩柱、腐食樹木、中央核などのVisual Asset生成に利用できるが、Gameplay上のRoot transform、Route、3禍根、Safe Pointは既存実装を基準とする。

## 既存Blender生成スクリプトの位置づけ

以下のような既存スクリプトは今後も削除しない。

- `Tools/CreateCharacterModels.py`
- `Tools/RefineCharacterModels.py`
- `Tools/RigCharacterModels.py`
- `Tools/VerifyRiggedCharacters.py`
- `Tools/CharacterPBR.py`
- `Tools/ImportRiggedCharacters.py`
- `Tools/ApplyRiggedMaterials.py`

これらは、

- Prototype / fallback生成
- Rig / Animation / Weight検証
- PBR検証
- UE Import検証
- Regression比較
- Tripo AI生成物を導入する前後のBaseline

として使用する。

**本番3Dの主要な生成元はTripo AI、Blender/Pythonは調整・検証・補助工程**という役割分担にする。

## 採用判定

Tripo AIで生成したモデルも、生成できたことだけでは採用しない。

以下を満たす場合のみ本番Assetとして採用する。

- コンセプトの主要シルエットを維持している。
- UE5で破綻なくImportできる。
- Skeleton / Weight / Animationが正常。
- Grab / Climbing / Cling / Staminaの既存Gameplayを壊さない。
- 既存の登攀Route、禍根位置、Collision基準を大きく変えない。
- Material / TextureがPBRとして正常。
- High Quality環境でVisualが既存Prototypeより明確に改善している。
- FPSやMemoryへ許容できないRegressionを起こさない。

## 開発上の優先順位

Tripo AIによる3D制作は、Gameplayを作り直すためではなく、**既に成立しているGameplayへ本番Visualを載せる工程**として扱う。

したがって、見た目のためにGrab地点、登攀Route、禍根位置、Boss Phase、Retry、Campaign進行を安易に変更しない。

現在の優先順位は、

**Gameplay完成済みPrimitive / 既存モデル → Tripo AI生成 → Blender調整 → UE Import → Gameplay Regression → High Quality Visual確認**

とする。
