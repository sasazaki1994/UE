# 禍津根 — Primitive Playable Vertical Slice

基準コミット: `e08d3de659638b9d9c622e599b5f58ade08704c3`。本変更は石走り・淵纏い・峰抱きおよび共通 `UGrabComponent` / `UStaminaComponent` / Nushi classesを変更せず、専用Actorから公開APIだけを再利用する。

## 世界観上の位置づけ

禍津根は第四の巨大生物や人格を持つラスボスではない。長年蓄積した禍が自然現象として地表へ噴出し、黒い根、岩柱、腐食樹木状の地形になったもの。内部実装ではLifecycle hostとして `ANushiBase` を継承するが、表示・説明は一貫して「生きて動く地形」である。目的は破壊やHP 0ではなく、三つの流れを断って中央核を一時的に鎮めること。

## Gameplay flow / Route

`Approach → Root Grab → Root A脈動を待つ/Cling → KAKON 1/3 → Root/岩柱/崩れた柱を往復 → KAKON 2/3 → Final Root Rise → Cling → 中央核KAKON 3/3 → Calming → Calm → Encounter Completed → Victory → Retry`。

| Phase | Node | 構成 | 禍根 / Safe point |
|---|---:|---|---|
| 1 地表根 | 0–3 | 地面 → Root A。4秒周期でTranslate/Rotateし、脈動頂点がCling判断になる | 禍根1=node 3。開始地面/node 3が休息点 |
| 2 環境往復 | 4–7 | Root → 岩柱 → Root → 崩れた石柱 → Root B | node 7が禍根2かつ安全回復点 |
| 3 中央核 | 8–11 | 持ち上がるRoot → 中央構造。最大Pitch 38° / Yaw -18° / Roll 31° | node 10が安全点、node 11が禍根3 |

進行前のRouteは非表示・移動不可。禍根1後はRoot全体が2.5秒で持ち上がってPhase 2を開き、禍根2後は大きく傾くFinal Riseを行う。Phaseごとに地面、岩/Root B終端、中央核手前の最低1安全地点でStaminaを回復する。

## Primitive構成 / Root変形

Engine基本Cube/Sphereだけで、中央核、9個の黒いRoot segment、Route marker、地面、岩柱、崩れた柱、6本の腐食樹木を構築する。Skeletal Mesh、Physics、Landscape、PCG、Destruction、Niagaraは使用しない。`LivingTerrainRoot` をDeltaSecondsでsmooth interpolationし、その子のGrab frame、禍根、Routeが同じTransformへ追従する。Phase 1は正弦波のZ移動±35 cm / Yaw±3° / Roll±7°、Phase 2はZ+120 cm / Pitch -8°、FinalはZ+390 cm / Pitch 38° / Roll 31°。30/60/120 FPSの相対追従Automationを定義した。

## Grab / Cling / Stamina

専用Playerは既存 `UGrabComponent` と `UStaminaComponent` をそのままDefault Subobjectとして持つ。Root親に付けた非scale Grab frameへ `TryGrab` し、Route移動は公開 `SetRelativeGrabTransform` を使う。このため移動中も世界座標へ置き去りにならない。E/RBはGrabとClingを兼ね、新Actionはない。通常移動はGrabだけ、Phase 1の周期Large PulseとFinal RiseはCling推奨で、非ClingまたはStamina不足なら落下する。Root上は消費、safe nodeと非Grab中は回復する。

## Recovery / Retry

落下はProgressをResetせず、0/3は基部、1/3はPhase 2入口、2/3はFinal入口に対応する安全地点へ移す。そこから通常移動とE/RBによる既存Grabで復帰し、再Grab直後2秒は消費猶予を与える。R/Y Retryは途中・Victory後とも、既存Actorを再spawnせずRoot transform、Route、Kakon、共通Progress/State/Encounter、Player、Stamina、Grab、Recovery、telemetryを初期化する。Automationとinput driverはActor数不変も検査する。

## Lifecycle / Common Nushi reuse

3個の `AKakonActor` を `ANushiBase::RegisterKakon` に登録し、独自Progress配列を持たない。`AKakonActor::Purify → UNushiProgressComponent 0/3..3/3 → UNushiStateComponent Calm → ANushiEncounterManager Completed` をそのまま利用する。専用PhaseはRoute表示/変形だけを担う。3個目後は2秒かけてRootの角度と高さを静かな姿勢へ戻し、消滅・爆発させない。

## HUD / Input

HUDはMAGATSUNE、KAKON、次Route、Grab可能、Large PulseのCling予告、Recovery方向、CALMED / ENCOUNTER COMPLETED / VICTORYを表示する。入力はWASD/Left Stick、Mouse/Right Stick、E/RB、Space/A、Left Click/X、R/Yだけである。境断ち探知、左腕能力、札は実装しない。

## Telemetry

軽量 `MAGATSUNE_TELEMETRY` logにRoot Grab attempts/success、Phase transitions、Kakon 1/2/3、Cling duration、Large pulse、Fall、Recovery、Stamina exhaustion、Calm/Completed/Victory、Retry、elapsed/clear timeを記録する。新Analytics基盤はない。

## Automation / Input Test

Automationは `MagatsuneLifecycle`、`RootTransformFollow`（30/60/120）、`RoutePhaseTransition`、`FallRecovery`、`RetryReset` を定義する。Integration driverはPlayerControllerのKeyboard/模擬Gamepad eventだけを使用し、攻略用にPlayer/Kakon/Phaseを直接書き換えない。Keyboard 60はRecoveryを含む2周（Victory後Retry→Victory）、Keyboard 30とGamepad 60は同じdriverを通す。

```powershell
.\Tools\Prototype.ps1 -Action Play -Magatsune
.\Tools\Prototype.ps1 -Action Test -Magatsune -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Magatsune -SkipBuild -TestFPS 30
.\Tools\Prototype.ps1 -Action Test -Magatsune -SkipBuild -Gamepad -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Magatsune -SkipBuild -Capture -TestFPS 60
```

## Screenshot定義

Capture runは `01-Arrival`、`02-RootMovement`、`03-FirstGrab`、`04-Kakon1`、`05-Phase2Opening`、`06-RootRockTransition`、`07-Kakon2`、`08-FinalRootRise`、`09-FinalCling`、`10-Kakon3`、`11-Calm`、`12-Victory` の12場面を要求する。描画可能なUEがないため画像は生成していない。

## Regression / Validation status

このLinux containerにはWindows、PowerShell、UE 5.6.1がなく、UHT/build、Automation、入力run、Capture、石走り（Climbing Basin/Grab/Camera）、淵纏い（Normal/Recovery）、峰抱き（Full Encounter/Recovery）はすべて `NOT_RUN`。実行していない結果をPASSにしない。静的check結果は `Docs/MagatsuneValidation.json` に記録する。

## 残る弱点

1. Rootは剛体segmentの親Transform変形で、節ごとの波や接地変形はまだ単純。
2. Phase 2はRoot/岩柱の視覚的往復をRoute nodeで表現するだけで、自由ジャンプ経路や分岐はない。
3. Recoveryは安全性優先の位置補正で、落下軌道から棚への連続的な着地演出と人間操作の距離調整が未検証。

次はWindows + UE 5.6.1でUHT/Editor build、5 Automation、Keyboard 60/30、Gamepad 60、Recovery、12 Captureの順に検証し、その後既存3 Encounterの指定Regressionを実行する。
