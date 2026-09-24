# 淵纏い Production Polish（2026-09-24）

Status: **SOURCE IMPLEMENTED / UE RUNTIME NOT_RUN**

## 調査基準

checkout の基準は `a30ca20`（PR #64 merge、直前の実装PRは #64内の`cb424f1`）。`origin/main` の取得はネットワークの CONNECT 403 で失敗したため、これより新しいremote状態は未確認である。石走りのProduction Vertical Slice、淵纏いのsource / README / Docs / Acceptance / Python contractを比較した。Campaign、Story、Save、Sense、浄化条件、Victory、Retry、Encounter progressionは変更しない。

| 観点 | 石走りで成立 | 変更前の淵纏い | 今回 |
|---|---|---|---|
| Encounter開始 / Boss予告 | Chaseから予告へ連続 | 即Submerged周期 | 契約維持。新規introは追加しない |
| movement | 巨体actorとclip、状態別motion | 頭の直線移動、身体は固定点列 | 頭の補間、位相差の小さい胴体うねり |
| attack telegraph / reaction | 身体色、向き、Telegraph、横回避 | Lightとdebug円。頭自体の予備動作が弱い | 頭を後方・下方へ溜め、追視、段階的local light。円/矢印はdebug限定 |
| Bite / Charge / Rock Catch | 固定方向ChargeとRecover | Bite target固定、指定岩Snag | 契約維持。Windup後にtarget lock、Snag窓を維持 |
| 攻撃後の隙 | Recover 1.65秒 | miss時に頭が瞬間帰還 | 1.1秒の退避を含む安全な間と補間帰還 |
| Grab / Cling / Stamina | 共通Grab/Stamina、取り付き補間 | 共通Grab/Stamina、専用route | 条件・消費・routeは変更なし |
| Kakon / Purification | 収束tail、色、Calm | 即時marker更新とCoiling | 共通の非authority pulse envelopeを追加し両主で利用 |
| Calm / Victory | Calm animation、静かな色、既存完了 | Calm色、既存完了 | authorityと遷移を維持。pulseは進行を待たせない |
| Camera | 遮蔽回避、登攀lag、控えめFOV | 遮蔽sweepのみ | 頭を弱くframingするlag、最大+3度の予告FOV、浄化+1.5度。回転shakeなし |
| Sense / Retry | read-only Sense、presentation reset | 同じ契約 | 変更なし。追加camera値だけRetry reset |

## 実装境界

`ANushiBase` に0〜1の短い浄化envelopeとCalm補間を置いた。これはKakon登録、浄化判定、Nushi state、完了通知を所有せず、各主が光・material・cameraへ任意に写すための小さな共通部品である。淵纏いが利用し、石走りの既存tailと同じ非authority原則を共通APIにした。Gameplay Contractで固定された石走りsourceは変更せず、峰抱き・禍津根も後続PRで重複tailを置き換えられる。大規模presentation frameworkにはしていない。

淵纏い固有として、地を這う身体の位相差、頭を引くBite anticipation、target lock後の直線lunge、岩へのSnag、Coiling routeを残した。身体うねりはcollisionやroute authorityを動かさないvisual offsetで、攻撃を高速化していない。通常時はwarning lightを消し、常時発光やHUD警告を追加していない。

## Validation / ASSET_REQUIRED / NOT_RUN

Python source contracts、GameplayContract、NarrativeContract、`git diff --check` をLinuxで実行する。結果はコミット時点の最終報告に記録する。

**ASSET_REQUIRED**: 淵纏いの完成Skeletal Mesh、蛇行/Bite anticipation/Bite recovery/Rock Catch/Calm animation、AnimBP、専用Material parameter、Niagara、Soundはcheckoutに存在しない。今回のPrimitive補間・local lightを完成assetとして扱わない。Camera shake assetもなく、静けさと酔い回避のため擬似回転shakeは追加していない。

**NOT_RUN**: このLinux環境にWindows、UE 5.6.1 Editor、MSVC、描画RHIがないため、Development Editor/Game build、Fuchimatoi 30/60 fixed timestep、実描画性能、Camera、Bite/Charge telegraph、Rock Catch、Kakon、Calm、Retry、Campaign regression、6場面captureは未実行。固定timestep結果と実GPU frame-timeは別のgateとして記録すること。

## 残る負債と次の1PR

Primitive bodyは物理的な長さを保存せず、visual waveとroute anchorの表面接触も一致しない。Head trackingの補間はsimulation step内の固定値を使うため、完成animationと置換する際にbone-spaceへ移す必要がある。共通envelopeは峰抱き・禍津根へ未接続である。

次の1PRは **Windows UE 5.6.1で今回のsourceをbuildし、Encounter開始 / Bite予告 / Charge（lunge） / Rock Catch / Kakon浄化 / Calmの同一camera captureと30/60 fixed timestepを取得して、初見回避率に基づきWindup・Recovery・camera biasだけを調整する**。新systemやassetの存在を仮定しない。
