# 白面共通表示・物語カード検証記録

## ソースに実装した内容

- 物語カードの計測値と表示領域幅を分離し、描画倍率を割り返した幅で、日本語を含む文字列を一文字単位で折り返す。空文字と極端に狭い表示幅も安全に扱い、本文領域と送り表示の間から縮尺上限を決める。
- `UShirotsuraVisualComponent` に既存 `SK_Shirotsura`、Idle / Walk / Run / Slash / Dodge / Climb / Hang / Grip / Jump / Death、ProductionVisuals 選択、簡易形状 fallback、武器骨表示、Retry 初期化を集約した。
- Prototype、淵纏い、峰抱き、禍津根の Player は固有の移動・Grab・Cling・攻撃・浄化ロジックを変更せず、ゲーム状態を共通表示へ通知する。アニメーションからゲーム処理を発火しない。
- Rig またはクリップが不足した場合は SkeletalMesh を非表示にして簡易形状だけを表示する。利用可能な場合は逆に簡易形状を非表示にし、二重表示を防ぐ。

## 今回、実際に実行して確認した内容

- Python のソース契約検査で、カード幅の分離、全 Player の共通 Component 接続、fallback の排他表示、Retry 初期化、ProductionVisuals 再利用を確認した。
- Narrative contract と既存 Campaign / 登攀 / ProductionVisuals を含む Python 検査一式を実行した。結果は PR の検証欄に記録する。

## UE 環境で確認が必要な内容（NOT_RUN）

- Unreal Editor / UnrealBuildTool がこの作業環境にないため、C++ ビルドと Automation（`Campaign.CardTextWrapping`、`ProductionVisuals.*`）は **NOT_RUN**。
- 日本語フォントの glyph 表示、狭い実画面でのカード本文と送り表示の間隔は **NOT_RUN**。ソース上の折り返し成立を実表示確認済みとは扱わない。
- 4 Encounter のメッシュ位置・向き、Idle / 移動 / 登攀 / しがみつき / 浄化・攻撃遷移、武器骨、Retry 後の姿勢、カメラ内の見切れは実機で確認する。
- 入力、HP、スタミナ、Sense、Collision、Grab 距離、石走りの体勢崩し、禍津根の浄化から Encounter 完了、Campaign 章遷移の操作回帰は実機で確認する。

## 次工程（今回未実装）

- この節は初回の共通表示実装時点の記録である。左腕の接続、および顔・首マスクの再生成手順については以下の追記を正とする。

## 禍祓い二段階表示（2026-09-18）

### Spec / acceptance

- 外見値は `Early` / `Advanced` の二つだけとし、Campaign は現在章を正本、単体起動は Encounter 種別を既定値とする。Interlude2 のカード画面では Player を生成せず、次の Encounter の BeginPlay で適用する。
- ProductionVisuals が候補メッシュを最終決定した後に一度だけ適用する。Retry の表示 Reset は同じ Dynamic Material Instance を保持し、Tick では生成・設定しない。
- 段階を解決できた状態と、監査済み素材へ値を適用できた状態を別フラグ・別ログにした。スロットまたはパラメーターがない Production 候補は既存素材を維持する。

### 素材・部位監査

調査元は `RebuildReferenceShirotsura.py` / `ReferenceShirotsuraHead.py` / `RigCharacterModels.py`、生成済み FBX・atlas・`rig-info.json`、および UE の `SK_Shirotsura` / `M_Baked_*` 資産である。Blender / UE Editor がないため、Editor 上の UV オーバーレイと描画確認は未実施である。

| 部位 | 形状・リグ | 素材 / atlas の実態 | 今回の対応 |
|---|---|---|---|
| 左腕 | `Corrupted_left_arm`、黒根・亀裂形状、左腕 bone weight | 専用 `09 • petrified corruption` section。濃い状態が BaseColor に bake 済み | 対応。BaseColor と薄い色を scalar で補間し Early=.28 / Advanced=1.0 |
| 首 | `Neck_anatomy`、head weight | `05 • exposed right hand` の skin atlas を右手・素顔と共有 | 未対応。右手まで変色させずに領域を分離する UV mask がない |
| 顔 | `Head_under_mask` は存在し、仮面 `Reference_mask_carved_porcelain` と別形状 | skin atlas は首・右手と共有。仮面は `07 • aged whitewood mask` で別 section | 未対応。仮面を黒くする代替は行わない |
| 左目 | facial rig なし。面の eye recess は仮面形状 | 独立した skin/eye 切替パラメーターなし | 未対応（既存素材だけでは簡単に安全対応できない） |
| 衣服・右腕・刀・仮面 | 個別形状 / material label あり | 左腕 section とは別 | 変更なし |

左右の腕は形状・rig region・material section で区別できる。一方、顔・首・右手は同じ baked atlas のため、UV を実測せず座標 mask を仮定する実装はしていない。したがって本変更を「顔を含む二段階外見の完成」とは扱わない。

### 生成・接続の状況

- `Tools/ApplyRiggedMaterials.py` は白面の監査済み左腕 section だけに `ShirotsuraCorruptionIntensity` の Lerp graph を作る。同じ `desc` tag の node を再利用するため再実行で node を増殖させず、通常の baked material 再適用と同時に設定を復元する。石走りにはこの graph を追加しない。
- スクリプト実装: **IMPLEMENTED**。実際の `.uasset` 再生成: **NOT_RUN**（Unreal Editor 不在）。従って、リポジトリ内の現在の material asset はまだ parameter を持たず、ランタイムは安全に `UNSUPPORTED` として既存表示を保つ。
- ランタイム接続: **IMPLEMENTED_IN_CPP**。素材適用成功: **NOT_RUN / asset generation pending**。顔・首・左目: **UNSUPPORTED**。
- UE build / Automation / 同一カメラでの Early・Advanced 描画比較 / 入力操作回帰: **NOT_RUN**（Unreal Editor / UnrealBuildTool 不在）。架空の比較画像は作成していない。

## 顔・首マスク制作経路（2026-09-24）

### 実素材と UV の再監査

- GitHub の `main` はネットワークの CONNECT 403 により取得できず、ローカルの基準点は merge commit `45d9d83` のままである。この制約を「最新 main 確認済み」とは扱わない。
- 共有画像 `T_Shirotsura_BaseColor.png` と生成元 `Shirotsura_Rigged.blend` / `SK_Shirotsura.fbx`、生成コードを照合した。生成処理は全オブジェクトを join して一つの `AtlasUV` を再展開し、顔 `Head_under_mask`、首 `Neck_anatomy`、右手を同じ skin material に集約する。従って、既存画像の色または矩形範囲だけから三者を安全に復元することはできない。
- 一方、join **前**には顔と首が明示的なオブジェクト名を持ち、仮面は `Reference_mask_carved_porcelain`、右手は別名である。この制作 provenance なら対象 polygon を推測せず区別できる。`ShirotsuraFaceNeckMask` corner color を全メッシュへ作り、顔・首だけ白、それ以外（右手・仮面を含む）を黒にしてから join し、同じ `AtlasUV` へ `T_Shirotsura_FaceNeckMask.png` をベイクする手順を追加した。

### 安全な接続と現在の素材状態

1. Blender 3.6 で `Tools/RigCharacterModels.py` を白面に対して再実行し、BaseColor 等と同じ解像度の mask PNG を生成する。
2. UV オーバーレイまたは mask 描画で顔・首が白、右手・仮面が黒、島の bleed がないことを確認する。確認できなければ PNG と `.uasset` を採用しない。
3. Unreal Editor で `CHARACTER_ASSET_FILTER=Shirotsura` として `Tools/ApplyRiggedMaterials.py` を実行する。PNG がある場合だけ共有 skin material に mask × `ShirotsuraCorruptionIntensity` の graph を作る。PNG がなければ skin material は変更しない。
4. ランタイムは左腕と共有 skin の各 slot を独立に検査し、同じ既存 `Early=.28` / `Advanced=1.0` 値を、parameter が実在する slot にだけ設定する。このため未再生成 asset、別 mesh、slot 欠落でも右手や仮面へ無条件の色変更は行わない。

現在チェックイン済みの PNG / FBX / `.uasset` は再生成していないため、顔・首用 mask 素材は **PENDING_ASSET_REGENERATION**、顔・首の見た目は **NOT_COMPLETE** である。モデル、skeleton、animation clip、能力、Campaign 章順は変更していない。Blender ベイク、UE C++ build、Automation、Early / Advanced の同一カメラ描画比較はこの環境では **NOT_RUN** であり、実装完了の根拠にはしていない。
