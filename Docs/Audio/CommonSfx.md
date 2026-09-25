# 共通効果音 試作記録

## 結論（2026-09-25）

素材サイトへの外部接続が HTTP 403 で遮断され、音源ページ、音声本体、作者、個別ライセンスを確認できなかった。したがって、試聴済みまたはライセンス確認済みとは報告せず、WAV とプレビューは収録していない。無関係なローカル音声や生成音で6役を埋めることもしない。候補と不足理由は `Audio/CommonSfx/manifest.json` に記録した。

確認を試みた場所:

- Kenney Interface Sounds（素材候補）: https://kenney.nl/assets/interface-sounds
- Kenney Impact Sounds（素材候補）: https://kenney.nl/assets/impact-sounds
- Freesound（CC0フィルターで風・石・水・呼吸を探索する候補）: https://freesound.org/
- GitHub `sasazaki1994/UE` main: https://github.com/sasazaki1994/UE

この状態は「適切な素材が見つかった」状態ではなく、調査環境の制約による **blocked** である。

2026-09-25 に再調査したが、`git fetch origin main` は CONNECT tunnel の HTTP 403、外部ページの直接確認も利用可能なWeb経路で HTTP 401 となった。このため最新 `main` の取得、元ページの確認、音源取得のいずれも完了していない。既存checkoutの内容だけを確認し、出典を推測せず、6役はすべて `pending` のままとした。

## Validator結果

validatorは、blocked時にも次を別々に表示する。

- provenance: `BLOCKED`
- WAV technical validation: `NOT_RUN`
- preview: `NOT_RUN`
- human listening: `NOT_RUN`
- UE import: `NOT_RUN`

したがってPythonの実行成功は、人による試聴やゲーム内確認の完了を意味しない。

## 再開手順

1. 各候補をヘッドフォンと小型スピーカーで実際に聴き、用途、ノイズ、耳に刺さる高域、強い低域を評価する。
2. 元ページ上で作者、ライセンス名・URL、原音単体の再配布可否を確認する。CC0を優先し、不明または単体再配布不可なら棄却する。
3. 原音はリポジトリへ残さず、必要部分だけを切り出し、fade、EQ、gainを適用する。加工値と時刻範囲を manifest に記録する。
4. `BoundaryReading.wav` から `EncounterCalmed.wav` までの6個と、仕様順の `CommonSfxPreview.wav` を配置する。
5. `python Tools/ValidateCommonSfx.py` とテストを実行し、その後に人がプレビューを聴いて manifest の `listening_review` を更新する。

UEへの取り込み、ゲーム内の音量・定位・BGMとのマスキング確認、ゲーム内再生は未実施である。BGM、主ごとの専用音、ゲーム進行ロジックには変更を加えていない。
