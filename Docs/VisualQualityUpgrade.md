# 石走り Vertical Slice 画質更新

## 方針と今回の最小単位

2026-09-11時点のPriority 1だけを実装した。既定のDX11検証経路は変更せず、明示的な
`-HighQuality` で同じマップ・同じGameplayへDX12 / SM6 / Lumen / VSMを重ねる。
一度に全Priorityを有効化して既存の11地点ルートを危険にさらさない。

関連仕様は `Specs/VisualQualityUpgrade.feature`。従来は品質プロファイルの切替、失敗時の
fallback、実機計測条件が仕様化されていなかったため、Legacy既定・明示選択・同一Gameplayでの
回帰というacceptance criteriaを追加した。

## 導入した技術

| 項目 | 状態 | 実装 |
|---|---|---|
| DX12 / SM6 | opt-in | `-HighQuality` が `-d3d12 -sm6` を渡す。DX12 SM6をcook対象へ追加 |
| Lumen GI / Reflections | opt-in | High Quality起動時だけ両CVarをLumenへ変更。mesh distance fieldsを生成可能に設定 |
| Virtual Shadow Maps | opt-in | High Quality起動時だけ有効化 |
| Atmosphere / Fog | Basinに導入 | Sky Atmosphereと控えめなvolumetric ground fog |
| Bloom / Exposure | opt-in | High QualityでBloom quality 4、Auto Exposure有効 |
| Nanite | 未導入 | Primitive岩を一括変換しない。制作済み高密度Static Meshを選別してから行う |

`DefaultEngine.ini` はDX11、Lumenなし、VSMなし、Bloomなし、固定露出のままである。
したがって通常起動と既存CIのコスト・見た目は維持される。High Quality用CVarはランチャーにのみあり、
設定ファイルの切替忘れによるリポジトリ汚染を避ける。

## Plugin / Module

今回追加したPluginとBuild Moduleは **なし**。Priority 1はEngineのRenderer機能だけを使う。
`PythonScriptPlugin`（Editor限定）とRuntime Module依存も変更していない。

次段階の候補はControl Rig、FullBodyIK、MotionWarping、Niagara、MetasoundEngine、PCG。
必要なPriorityに着手するときだけ `.uproject` と `Build.cs` へ追加し、各段階でbuildと回帰を行う。

## 切替方法（Windows UE 5.6）

```powershell
# Legacy（既定）
.\Tools\Prototype.ps1 -Action Play -Basin

# High Quality
.\Tools\Prototype.ps1 -Action Play -Basin -HighQuality

# 同一ルート・同一Cameraの描画比較
.\Tools\Prototype.ps1 -Action Test -Climbing -Capture -Basin -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Climbing -Capture -Basin -HighQuality -SkipBuild -TestFPS 60
```

High Qualityが起動しないGPU/driverではLegacyコマンドへ戻す。自動fallbackは、失敗を見えなくして
誤ってHigh Quality成功と記録するため実装していない。`Saved/Logs/ClimbingTest-60.log` の
RHI、Feature Level、Lumen警告を確認すること。

## Windows実機Validation手順

以下をUE 5.6があるWindows PowerShellでリポジトリ直下から順番に実行する。

```powershell
.\Tools\Prototype.ps1 -Action Build
.\Tools\Prototype.ps1 -Action Test -Camera -Capture -Basin -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Climbing -Capture -Basin -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Climbing -Basin -SkipBuild -TestFPS 30
.\Tools\Prototype.ps1 -Action Test -ClimbingGamepad -Basin -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Grab -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -BasinScenario -Basin -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Camera -Capture -Basin -HighQuality -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Climbing -Capture -Basin -HighQuality -SkipBuild -TestFPS 60
```

EditorのOutput Logで `stat fps`, `stat unit`, `stat gpu`, `rhi.DumpMemory` を実行し、LegacyとHigh
QualityそれぞれでBasin開始、接近、Grab、登攀、禍根、浄化、Victoryを同じ解像度・Camera位置で記録する。
現行自動captureはClimbingのGround / FirstLedge / ShoulderCore / Summit / Victory / ThrownOffを保存する。
接近・Grab・浄化を完全に同フレームで比較する専用capture拡張は次の小変更とする。

## Validation結果

| 検査 | 結果 |
|---|---|
| Python static/acceptance tests | この変更で実行。結果はコミット時の報告を参照 |
| JSON構文 | この変更で実行。結果はコミット時の報告を参照 |
| UE 5.6 Editor C++ Build | **UNVERIFIED**（このLinux環境にWindows UE 5.6 / MSVCなし） |
| Arena / Basin / Keyboard / Gamepad / Camera / Grab / Retry | **UNVERIFIED**（実機runtime未実行） |
| Climbing 60 / 30 FPS | **UNVERIFIED** |
| High Quality描画、Lumen、VSM | **UNVERIFIED** |
| High Quality FPS | **UNVERIFIED / 未計測** |
| Legacy FPS | **UNVERIFIED / 未計測**（過去記録を今回の結果として再利用しない） |
| スクリーンショット | **UNVERIFIED / 未取得** |

## 未導入技術とKnown Issues

- Priority 2〜7（Control Rig / Full Body IK、Motion Warping、Niagara、MetaSounds、PCG、Animation Blend改善）は未導入。
- High QualityはD3D12とSM6対応GPU/driverが必要。初回shader compilationは長い。
- `r.GenerateMeshDistanceFields=True` は両経路でDerived Dataとcook容量を増やし得るが、Legacy runtimeのGI方式は変更しない。
- BasinはEngine Primitiveで構成され、現時点でNanite化の画質利益が小さい。
- Volumetric FogはBasinの両profileに存在するが、LegacyではRenderer既定の `r.VolumetricFog` に従う。
- Auto Exposureの最終Min/Max、夕暮れの色温度、濡れた岩のroughnessは実機histogramと比較画像なしに確定しない。

## 次の推奨作業

まずPriority 1のWindows比較を完了し、GPU時間と7場面を記録する。重大な描画・回帰問題がなければ、
Priority 2として既存11地点のうち前脚→最初の肩岩だけにControl Rig / Full Body IKの手足targetを追加する。
targetはBoss local spaceに保持し、ルート座標・Skeleton・Animation Clip・Gameplay遷移は変更しない。
