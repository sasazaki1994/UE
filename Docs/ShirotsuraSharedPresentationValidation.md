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

- 顔・首・左腕の Early / Advanced 二段階状態を実マテリアルスロットへ接続する。既存 API と章対応は維持しているが、マテリアル切替は今回の必須範囲外であり未接続のままである。
