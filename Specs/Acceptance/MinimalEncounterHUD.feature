# language: ja
機能: 4 Encounterの最小・文脈型HUD
  通常画面は現在の判断だけを支え、内部状態の検証情報はDebugGuidanceへ隔離する。

  シナリオ: 安全な通常時は常設情報を表示しない
    前提 Encounterが進行中である
    かつ プレイヤーは安全でSenseを使用していない
    かつ Staminaは十分である
    もし HUDを描画する
    ならば Boss名、Kakon数値、Stamina数値、Route、State、Telemetry、常時操作一覧を表示しない

  シナリオ: 最重要の操作を一つだけ表示する
    前提 Cling、Purify、Grab、Recoveryの文脈が既存Gameplay stateから読み取れる
    もし HUDを描画する
    ならば 最も優先度の高い操作Promptを一つだけ表示する
    かつ HUDはGameplay stateを変更しない

  シナリオ: 浄化直後だけ進行を知らせる
    もし 禍根の浄化数が増える
    ならば 「禍根 n/3」を1.5秒だけ表示する
    かつ 通常時は進行数値を隠す

  シナリオ: Staminaを必要時だけ量として示す
    前提 プレイヤーが登攀中でStaminaを消費したか危険に備えている
    もし HUDを描画する
    ならば 数値を含まないStaminaゲージを表示する
    かつ 安全な全快状態ではゲージを表示しない

  シナリオ: Senseは使用中だけ表示する
    もし Boundary Senseを使用中である
    ならば 距離強度だけを表示する
    もし Corruption Senseを使用中である
    ならば 危険区分だけを表示する
    かつ Senseを使用していない間はどちらも表示しない

  シナリオ: DebugGuidanceは検証情報を維持する
    前提 -DebugGuidanceで起動している
    もし HUDを描画する
    ならば 正確なKakonとStamina、StateまたはPhase、Route、Telemetry、操作一覧を表示する
