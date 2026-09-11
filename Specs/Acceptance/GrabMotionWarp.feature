# language: ja
機能: 地上から石走り前脚へ自然に取り付く
  プレイヤーとして
  Grab成立後に白面自身の踏み込みと手伸ばしで前脚へ接触したい
  なぜならRoute node 0への瞬間移動を見せたくないから

  背景:
    前提 UE 5.6標準MotionWarping Pluginが有効である
    かつ Grab Warp Targetは既存Route node 0から毎frame算出される

  シナリオ: 通常入力で地上から接触する
    前提 白面が通常PlayerStartから歩いて既存GrabRange内へ入る
    かつ 石走りはCharge中ではない
    もし KeyboardのEまたはGamepadのRBでGrabする
    ならば 接近から手の接触直前だけTranslationとRotationがWarpされる
    かつ Warp完了後にRoute Climbing node 0へ移行する
    かつ Warp中のIK Weightは0で完了後に既存制御へ渡される

  シナリオアウトライン: 危険な吸着を拒否する
    もし <条件> でGrab入力する
    ならば Motion Warpは開始されない
    かつ 入力とCharacterMovementはSoft Lockしない
    例:
      | 条件 |
      | 既存GrabRange外 |
      | Maximum Warp Angle外 |
      | 石走りがCharge中 |

  シナリオアウトライン: Warpを安全に中断する
    前提 Grab Motion Warp中である
    もし <中断> が発生する
    ならば Montage、Warp Target、Movement lock、IK Weightの残留がない
    例:
      | 中断 |
      | Target無効化 |
      | Charge遷移 |
      | Player死亡 |
      | Fall |
      | Retry |
