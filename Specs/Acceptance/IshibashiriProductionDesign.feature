# language: ja
機能: 石走り編 Production gameplay readability contract
  主を倒さず禍だけを祓う体験を、固定ルートと少ないHUDと落ち着いたCameraで初見にも理解可能にする。

  シナリオ: 初見プレイヤーが地上戦の三段階を理解する
    前提 石走りの体力ゲージと数値Postureは表示されていない
    もし 石走りが1.0秒予告して追尾しない突進を行う
    かつ プレイヤーが横回避する
    ならば 1.65秒のRecover中だけ反撃が有効になる
    かつ 三回の有効反撃で5.0秒の膝つきになる
    かつ 身体表現と初回だけの操作表示から取り付く機会だと理解できる

  シナリオ: Grabは精密な位置合わせを要求しない
    前提 石走りが膝つき中である
    もし プレイヤーが前脚または下がった角の300cm以内かつ120度以内でGrabを入力する
    ならば 二つの開始領域は同じ11地点Routeの先頭へ補助接続する
    かつ 数十cm単位の手動位置合わせを要求しない

  シナリオ: Mount Windowを逃しても学習済みの地上戦を全て反復しない
    前提 三回の有効反撃後に石走りが膝ついている
    もし 5.0秒以内にGrabしない
    ならば Chaseへ戻る
    かつ 次の一回の有効反撃で再び膝つきになる

  シナリオ: Staminaは安全地点と揺れへの備えを選ばせる
    前提 Stamina上限は100で通常登攀は毎秒5消費する
    もし 12秒周期の揺れに2秒の予告が出る
    ならば 安全棚は毎秒22回復する
    かつ Cling中の揺れは毎秒18消費する
    かつ Clingしない状態が0.70秒を超えるかStaminaが尽きた時だけ落下する
    かつ CameraはBoss transformへ完全追従しない

  シナリオ: 落下は理解済みの進行を破棄しない
    前提 プレイヤーが一つ以上の禍根を祓って登攀している
    もし ShakeまたはStamina切れで落下する
    ならば 最後に到達した安全棚または前脚Recovery地点へ戻る
    かつ 浄化済み禍根を維持してStamina 50と3秒のShake猶予を得る
    かつ 6秒間は再Grabできる
    かつ RまたはYの全Retryだけが全進行を初期化する

  シナリオ: 禍根はSenseと身体から順に発見する
    前提 常時赤い弱点Markerと常時禍根数は表示されていない
    もし 境断ちを保持する
    ならば 現在到達可能な未浄化禍根だけを粗い方向と強さで示す
    かつ 右肩、中央頂上、背面の順に一度ずつ浄化する
    かつ 浄化中と完了後1.5秒はShakeを開始しない
    かつ 各完了後だけ禍根1/3、2/3、3/3を1.5秒表示する

  シナリオ: Calmは勝利や死亡ではなく救済を伝える
    前提 三つの異なる禍根が浄化された
    ならば CalmとEncounter Completedだけが進行権限を持つ
    かつ Production画面にVictoryまたはBoss HPを表示しない
    かつ 石走りは死亡も消滅もせず最低2秒間呼吸が鎮まる
    かつ 二枚の短いEndingを確認した後にTitleへ戻る

  シナリオ: Chargeを視覚と身体音の両方で予告する
    前提 石走りが1.00秒のTelegraphへ入る
    ならば 重心移動と前脚周辺の局所Cueが突進方向を示す
    かつ 蹄の溜め、吸気、岩の軋みからなる固有の予告音が鳴る
    かつ 音源または本番VFXが未提供なら結果をPASSにせずASSET_REQUIREDとする

  シナリオ: ShakeとClingの瞬間を複数の手段で読む
    前提 Shake開始まで2.0秒である
    ならば 身体の予備動作、呼吸変化、局所Cue、縄または鈴の音で予告する
    かつ Shake直前の固有Cueを聞いてEまたはRBを入力できる
    かつ 音なし、色覚差、振動なしの各条件でも代替情報を残す

  シナリオ: DebugGuidanceなしで攻略情報を取得する
    前提 -DebugGuidance を指定せず本番表示で開始する
    ならば Charge、Grab、Stamina危険、Shake、Kakon、CalmをDebug円、矢印、状態文字なしで識別できる
    もし 境断ちを保持する
    ならば 到達可能な次の禍根の粗い方向と強度を刀身と限定Route反応から理解できる
    かつ 実音源、本番VFX、人間の初見記録がない項目はNOT_RUNまたはASSET_REQUIREDとする

  シナリオ: Calmは生きた主と自然の復帰で表現する
    前提 3個目の禍根の浄化が受理された
    ならば 禍の発光と戦闘音楽が止まり短い無音を置く
    かつ 遅い呼吸、鼻息、風、木、鳥と自然色が段階的に戻る
    かつ 死亡衝撃、消滅、爆発、戦利品音、Victory文字を表示しない

  シナリオ: 通常HUDとSenseとDebugGuidanceを分離する
    前提 通常プレイを開始している
    ならば 恒常的な文字Panel、数値Stamina、数値HP、Posture、State、Route番号を表示しない
    かつ Stamina、危険、Grab、Cling、浄化数、Retryは必要な状況だけ表示する
    もし Senseを保持する
    ならば 現在の粗い方向、危険または安全の反応だけを表示する
    もし -DebugGuidance で起動する
    ならば 内部値をDEBUGと明示して追加表示する
    かつ Debug表示は入力条件、進行、Camera collisionを変更しない

  シナリオ: Cameraは巨体より一段落ち着く
    前提 Ground Cameraは740cm、95度FOVを基準にしている
    もし Charge、Dodge、Grab、Climb、Shake、Fall、Calmを連続して通る
    ならば 一遷移のFOV差は5度以下で88度から103度に収まる
    かつ 回転Shakeは各軸0.35度以下かつRollは0.15度以下である
    かつ 頻繁なSnap、操作軸の反転、BossのPitch/Yaw/Rollへの完全追従がない
    かつ PlayerのCamera入力は緊急Collision退避以外のassistを中断できる

  シナリオ: Camera collisionは極端な接写を避ける
    前提 石走り、壁、岩、木、地面、背中の岩、禍根または狭所がCameraを遮る
    ならば 外側arc、raised shoulder、遮蔽物fade、emergency high shoulderの順で退避する
    かつ 通常はPlayerから300cm以上、緊急時にも220cm以上を保つ
    かつ 遮蔽解消後0.15秒待って0.35秒以上で通常Cameraへ戻る

  シナリオ: 初見合格はRuntime理解Evidenceを必要とする
    前提 Source contractと自動E2Eが成功している
    もし 開発に参加していないプレイヤーの記録が存在しない
    ならば 地上戦、Grab、Shake、禍根、Calmの理解をPASSと推定しない
    かつ Current SHAのCameraとHUDの視認性をNOT_RUNとして扱う
