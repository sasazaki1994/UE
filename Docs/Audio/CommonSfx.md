# 共通効果音 作業記録

2026-09-30。6つのイベントへのC++接続、再現可能な加工スクリプト、UE取込スクリプトを追加した。WAV、プレビュー、SoundWaveの生成と人による試聴は未実施。`Audio/CommonSfx/manifest.json` は未生成状態を示す。

素材候補はKenney [Impact Sounds](https://kenney.nl/assets/impact-sounds)、[RPG Audio](https://kenney.nl/assets/rpg-audio)、Freesoundの[水音](https://freesound.org/people/qubodup/sounds/212143/)と[風音](https://freesound.org/people/florianreichelt/sounds/459977/)。ダウンロードした元音声はリポジトリ外に置き、`Tools/PrepareCommonSfx.py` が切り出し、EQ、フェード、音量を適用する。出典、作者、ライセンス、原音ハッシュ、加工値を生成時のmanifestへ記録する設計。実際の生成と出力検証が完了するまではライセンス・音質の採用判定をしない。

```powershell
python Tools/PrepareCommonSfx.py --source-root <元音声の保存先>
python Tools/ValidateCommonSfx.py
```

その後、UE Pythonから `Tools/ImportCommonSfx.py` を実行して6つのSoundWaveを保存し、実機で定位・音量・警告としての聞き取りやすさを確認する。プレビューと6音源をヘッドフォンと小型スピーカーで人が聴く必要がある。BGMとの重なりも未確認。
