# 白面・石走り CC0 PBR 素材ライブラリ

## 2026-09-10 着手時確認

作業ツリーは `9d1b147`（PR #18 のマージ）でクリーンだった。ローカルには
`work` ブランチしかなく remote は設定されていないため、GitHub 上の最新
`main` と関連 PR は照合できなかった。GitHub CLI も未認証だった。
`feat/cc0-pbr-character-materials` をこのコミットから独立して作成した。

指定された外観・登攀資料、6本の Blender/UE スクリプト、既存 acceptance
spec を確認した。記録どおり、先行改良のソースはある一方、生成済みモデルの
更新や比較描画は行われていなかった。この環境には Blender と UnrealEditor
の実行ファイルがない。

## 素材選定と取得結果

対象は肩岩の風化石、木面の古い白木、上着の粗い織布の3種類だけである。
Poly Haven と ambientCG の公式サイト/API を 2026-09-10 に HTTPS で確認し
ようとしたが、環境のプロキシが `Tunnel connection failed: 403 Forbidden` を
返した。検索サービスも `401 Unauthorized` だった。このため、公式ページ、
CC0表記、実在ID、取得URL、物理寸法を確認できず、ファイルを取得できないので
SHA-256も算出できなかった。

架空の値を導入済みとしない要件に従い、`Art/Materials/CC0PBR/manifest.json`
には3つの用途だけを `unresolved_network_blocked` として記録した。したがって
**素材3種類は未導入**であり、モデル、atlas、UE asset、比較画像は更新して
いない。既存バイナリを変更後成果物として扱わない。

## 実装した再開可能な経路

接続可能な環境では、各用途について公式 CC0 ページでIDとライセンスを確認し、
2K の BaseColor、OpenGL Normal、Roughness を `Art/Materials/CC0PBR/files/` に
保存する。manifest の各 map を `url`、repository-relative `path`、`sha256`
で埋め、license の検証URLと `verified: true`、素材の実寸が公開されていれば
`physical_size_m` を記録する。次で不足・改変を検出する。

```bash
python Tools/ManageCharacterPBR.py verify
```

Blender は検証済み manifest の場合だけ3用途へ素材を重ねる。色は素材色の直貼り
ではなく、風化石、白木、藍布のデザイン色で乗算する。Normal 強度は 0.32 に抑え、
既存の形状由来の傷・しわを残す。粒度の初期値は岩4、木7、布18 repeat とし、実寸
情報と同一カメラ近景を見て調整する。素材用UVを読み取った後、ベイク専用
`AtlasUV` を別途作成し、2048px BaseColor/Normal/Roughness を出力する。
未検証キャッシュでは既存の手続き型外観へ明示的にフォールバックする。

UE は BaseColor だけを sRGB、Normal と Roughness を linear とし、Roughness は
Masks 圧縮で接続する。入力は OpenGL Normal に固定し、UE import の
`flip_green_channel=true` の一回だけで DirectX 規約へ変換する。再適用では種類と
ordinalで既存 texture sample を再利用する。マテリアルの粗さ分類は引き続き slot
label（用途名）を見ており、配列順で材質を判断しない。

## 再生成・取り込み・比較手順

manifest 検証成功後、既存コミットの preview と atlas を `Artifacts/Before/` に
退避し、次を実行する。先行造形改良も初めて生成されるため、まず外部PBRを無効に
した生成を `AfterGeometry/`、有効にした生成を `AfterPBR/` に保存して差を分離する。

```bash
blender --background --factory-startup --python Tools/CreateCharacterModels.py
blender --background --factory-startup --python Tools/RefineCharacterModels.py
blender --background --factory-startup --python Tools/RigCharacterModels.py
blender --background --factory-startup --python Tools/VerifyRiggedCharacters.py
UnrealEditor-Cmd IshibashiriPrototype.uproject -run=pythonscript -script=Tools/ImportRiggedCharacters.py -unattended
UnrealEditor-Cmd IshibashiriPrototype.uproject -run=pythonscript -script=Tools/ApplyRiggedMaterials.py -unattended
UnrealEditor-Cmd IshibashiriPrototype.uproject -run=pythonscript -script=Tools/ApplyRiggedMaterials.py -unattended
UnrealEditor-Cmd IshibashiriPrototype.uproject -run=pythonscript -script=Tools/VerifyRiggedCharacters.py -unattended
```

同一カメラ・3灯・露出で木面、上着、肩岩を撮影する。法線の凹凸方向、UV seam、
粒度、欠落、アニメーション中の模様の追従を確認する。再適用前後の mesh material
数、各 material expression 数、texture reference が同じことを記録する。最後に
`Docs/ClimbingValidation.md` の描画あり60 FPS試験を実行し、11地点、棚、3禍根、
浄化と視認性を画像付きで確認する。

## 容量・検証結果・残作業

変更前の PNG は BaseColor/Normal の2枚ずつ（白面 9,307,136 bytes、石走り
8,568,082 bytes）。UE texture は白面 9,044,307 bytes、石走り 8,269,746 bytes。
現時点の material asset は白面13、石走り14である。Roughness生成前なので変更後
容量は未計測で、見込み値を実測値として記載しない。

Python構文検査と差分検査は成功した。キャッシュ検査は意図どおり失敗し、全9 map、
公式ID/URL、ライセンス、ハッシュが未解決であることを報告した。Blender生成、UE
取り込み、二度適用、描画比較、60 FPS登攀はツール不在のため未実施である。この
PRはこれらが完了するまで Draft とする。

## 2026-09-10 素材判定・適用前検証の修正

Blender素材名を単純な部分文字列ではなく、Unicode正規化後の英単語として照合する
ようにした。先頭番号、`•`、空白、underscore、大文字小文字を無視して、実在する
`01 • indigo work cloth`、`05 • aged whitewood mask`、`03 • granite N` をそれぞれ
藍布、白木、石へ分類する。紙、縄、苔、皮膚、刃などは対象にしない。

CLIとBlenderは同じmanifest loader/validatorを使用する。JSON構造、license、3用途が
各1件であること、素材とmapの必須項目、BaseColor/OpenGL Normal/Roughness、ファイル
存在、SHA-256を全件検証してから、画像またはnodeに触れる。白面は白木・藍布だけ、
石走りは石だけを適用先として要求し、結果には実際の素材名と用途別件数を記録する。

通常生成は未取得時にも従来の手続き型素材を維持する `fallback` であり、理由付きの
`not_applied` を `rig-info.json` に保存する。PBR導入確認では、素材不足、検証不合格、
適用先不足のいずれでも停止する `strict` を明示する。

```bash
python -m unittest discover -s Tests -p 'test_*.py' -v
python Tools/ManageCharacterPBR.py verify
blender --background --factory-startup --python Tools/RigCharacterModels.py -- --pbr-mode strict
```

標準Pythonの自動テストでは、分類、誤判定防止、一時ファイルの実SHA-256、欠落・改変・
重複・不正JSON・必須項目不足、strict/fallback、事前検証中の副作用防止、キャラクター別
適用件数を検証する。正式manifestは引き続き未取得・license未検証であり、上記verifyが
未取得を報告する状態を維持する。素材取得、UV・法線の実画像確認、Blender生成・描画、
UE取り込み・描画・登攀検証は未実施であり、完了扱いにしない。
