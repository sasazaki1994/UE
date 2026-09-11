# language: ja
機能: 前脚から最初の肩岩までの接触補正
  プレイヤーとして
  既存の登攀アニメーションを保ちながら白面の四肢を岩へ接触させたい
  なぜならルート登攀のプロトタイプ感を最小変更で減らしたいから

  背景:
    前提 実際の白面Skeletonのroot、pelvis、spine、chest、左右の腕脚Boneを使う
    かつ IK Targetは石走りのSkeletal Mesh Local Spaceで定義されている

  シナリオ: 代表区間だけFull Body IKを適用する
    もし 白面が通常入力で前脚をGrabして最初の肩岩まで登る
    ならば Node 0から3だけでIK Weightが滑らかに有効になる
    かつ hand_L、hand_R、foot_L、foot_Rは対応Targetへ補正される
    かつ 既存Climb AnimationはBase Poseとして再生される

  シナリオ: 動く石走りとの接触を維持する
    前提 白面が代表区間で登攀中である
    もし 石走りが歩行、旋回、またはShakeする
    ならば 四つのTargetは石走りと同じComponent Transformで移動する
    かつ Clingに成功した白面のIKは維持される

  シナリオアウトライン: IKを安全に解除する
    前提 白面のIK Weightがゼロより大きい
    もし <遷移> が発生する
    ならば IK Weightは瞬間移動せずゼロへ向かう
    かつ RetryではTargetとWeightが完全に初期化される

    例:
      | 遷移 |
      | Jump Off |
      | Cling失敗によるFall |
      | Retry |

  シナリオ: 開発ビルドで接触誤差を観察する
    もし ClimbingIKDebugを指定して代表区間を実行する
    ならば Target、Bone位置、両者を結ぶ線が表示される
    かつ 誤差8cm以下は緑、20cm以下は黄、それ以外は赤である
    かつ Shippingでは表示されない
