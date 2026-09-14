# Tripo AI Reference Set

『禍祓い』の本番3D生成に使う技術リファレンス画像セット。

本番3Dの主要生成元は Tripo AI とし、このフォルダのSVGは、生成前にシルエット・素材・Gameplay上変更してはいけない領域を固定するための入力/監修資料として使う。

## Reference sheets

- `Shirotsura_TripoReference.svg`
  - 白面の祓い手。
  - 白面、濃紺〜墨色の山仕事/祓い装束、穢れた左腕、境断ち、腰縄、札、鈴を固定。
  - 既存 `Art/Characters/Shirotsura/References/Front.png` / `Turnaround.png` は削除せず、併用する。

- `Ishibashiri_TripoReference.svg`
  - 第一の主「石走り」。
  - 巨大猪 + 岩山 + 苔 + 根 + 局所的な祭祀物。
  - 既存11 Route、Grab面、3禍根、CollisionをVisualより優先する。

- `Fuchimatoi_TripoReference.svg`
  - 第二の主「淵纏い」。
  - 巨大な渓谷大蛇。
  - Bite / Head Grab / Coiling / Snake→Rock→Snake / 3禍根の成立を優先する。

- `Minedaki_TripoReference.svg`
  - 第三の主「峰抱き」。
  - 巨大山猿。
  - 左脚→背中→左腕橋→反対肩→首→頭頂の身体Routeを優先する。

- `Magatsune_TripoReference.svg`
  - 最終局面「禍津根」。
  - 第四の主ではなく、生きて動く地形。
  - 黒い根、岩柱、腐食樹木、中央核を部品として生成し、既存Root transform / Route / Safe Pointを基準にUE5で再構成する。

## Tripo AIでの使い方

1. 対象のSVGをPNGへ書き出すか、対応環境ではそのまま参照する。
2. 白面は既存Front/Turnaround画像と今回の技術シートを併用する。
3. 1回の生成で全部を決めようとせず、外形を優先したBase Meshを生成する。
4. 生成後はBlenderでScale、Topology、UV、Material、Rig、Weightを調整する。
5. UE5にImportし、既存のGrab / Climbing / Cling / Kakon / Collision契約と合わせる。
6. Gameplay Regression後、High Quality環境で採用判断する。

## 共通禁止事項

- 見た目を理由に既存Kakon位置、Route、Grab地点、Boss Phase、Retry、Campaign進行を安易に変更しない。
- 禍根以外を過度に発光させない。
- 西洋ドラゴン、重装騎士、SF、機械装甲へ寄せない。
- 主を討伐対象の悪役として死体化・爆散・消滅させない。
- 禍津根へ顔や人格を与えない。

## Pipeline

`ChatGPT reference → Tripo AI → Blender cleanup / rig / weights → UE5 import → Gameplay regression → High Quality review`

このSVG群は最終コンセプトアートそのものではなく、Tripo AIへの入力と生成物レビューで形状契約を崩さないための技術リファレンスである。
