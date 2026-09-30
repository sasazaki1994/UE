# 石走りデモ：環境と共通効果音の作業記録

2026-09-30。最新 `main` のHUD、落下復帰、揺れ・浄化タイミングの実装を前提にした追加作業。

## 変更内容

- 古杉・岩・境界石のBlender生成器とUE取込スクリプトを用意。Approach/Basinは取込済み素材を表示し、素材がない場合は既存の仮素材へ戻る。通路と戦闘に使う衝突は従来のものを使う。
- 石走りデモのApproachからBasinへ移る際に専用アリーナを選択。通常Campaignの進行形式はそのまま。
- 6つの共通効果音をSense、予告、取り付き、揺れ、浄化、鎮静のイベントに接続。音の有無は進行条件にしない。音声制作・取込手順は [CommonSfx](Audio/CommonSfx.md)。
- 同じ位置・視点からApproachで5枚、Basinで2枚を撮る描画比較モードを追加。

## 状態と検証

Blenderの候補生成は実施したが、生成物はこのPRに収録していない。UEへの環境素材取込と実行画面の確認は未実施。6つのWAVも未生成で、UE取込・ゲーム内再生・試聴は未実施。最終C++ビルド、スクリーンショット比較、初見プレイも未実施。従ってこの変更はレビュー用のドラフトであり、完成判定ではない。

描画確認用のコマンド（実行結果ではなく手順）:

```powershell
.\Tools\Prototype.ps1 -Action Test -EnvironmentReview -Approach -Capture
.\Tools\Prototype.ps1 -Action Test -EnvironmentReview -Approach -Capture -PrimitiveEnvironment
.\Tools\Prototype.ps1 -Action Test -EnvironmentReview -Basin -Capture
.\Tools\Prototype.ps1 -Action Test -EnvironmentReview -Basin -Capture -PrimitiveEnvironment
```

`EnvironmentReview` は視点を固定した描画確認で、入力による攻略や衝突確認の証拠にはならない。素材取込後は上記の撮影、既存デモE2E、登攀・カメラの実行確認、物理コントローラーとヘッドフォンでの確認が必要。
