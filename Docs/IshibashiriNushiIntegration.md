# 石走りの共通Nushi基盤と体勢戦

## 攻略契約

第一の主「石走り」は次の一方向の攻略フローを持つ。

`Chase → Telegraph → Charge（方向固定） → 横回避 → Recover → 禍の隆起へ反撃 → 体勢値0 → Kneel / Mount Window → 前脚・角からGrab → 11地点Route → 3禍根浄化 → Calm → Completed → Victory`

地上斬撃は本体へのダメージではない。通常時は硬質部位に弾かれ、Recover中に脚・角周辺へ現れるPrimitiveの禍の隆起だけが有効である。有効反撃はRecoverごとに一回、石走り専用の `Posture` を1減らす。初期値とRetry値は `MaxPosture=3` である。

## 膝つきと取り付き

体勢値0は `Kneel` へ遷移するだけであり、Calm、Completed、Victoryを発生させない。5秒のMount Window中は地上AIと突進を停止し、前脚のGrab Markerを表示する。通常時はRouteへの新規Grabを拒否する。

取り付きに成功した場合は既存Grab Motion Warpまたはfallbackから11地点Routeのnode 0へ接続し、Mount Window終了で強制解除しない。取り付かなければChaseへ戻して体勢値を最大へ戻す。登攀後に落下した場合も同じ地上フローから再度取り付ける。

## 唯一の勝利条件

勝利は登録済みの3つの `AKakonActor` がすべて一度ずつPurifiedになった場合だけである。独自の進行配列や地上戦からの完了経路は持たない。

`AKakonActor::Purify`
→ `UNushiProgressComponent::OnAllPurified`
→ `UNushiStateComponent::CalmNushi`
→ `ANushiEncounterManager::Completed`
→ `APrototypeGameMode::HandleEncounterCompleted`
→ Victory / Campaign `OnEncounterCompleted`

1/3と2/3はRunningのままである。3/3後も石走りは死亡・消滅せずCalmed animationを使う。

## 維持する契約

- 11地点Route、分岐、休息地点、Shake、Cling、Stamina、一度だけのPurify
- Telegraph終了時に固定されるCharge方向と、回避失敗時のPlayer Health減少
- 境断ちの「現在到達可能な未浄化禍根」だけの案内
- 左腕のTelegraph / Charge / Shake=`DANGER`、Recover / Kneel=`SAFE`
- 左腕使用中と解除後2秒のStamina回復50%
- Retry時のPlayer、Climbing、Motion Warp、Kakon、Nushi、Encounter、Sense、Camera、表示、位置のReset
- `OnEncounterCompleted` によるCampaign遷移

## 検証

静的/source-contract検証は `Tests/test_ishibashiri_posture_source_contract.py` が、旧本体Health経路の不在、体勢値、Kneel、Grab gate、Sense、唯一の浄化完了経路、Acceptanceと検証記録を確認する。Windows UE 5.6.1のBuild、Automation、入力駆動60/30 FPS、模擬Gamepad、Campaign E2EはこのLinux環境では実行せず `Docs/IshibashiriPostureValidation.json` に `NOT_RUN` と記録する。
