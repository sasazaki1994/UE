# language: ja
機能: 四主戦の通常HUDと開発表示の分離
  通常プレイに必要な判断材料を残し、実装詳細は明示的なデバッグ起動時だけ表示する。

  シナリオアウトライン: 通常HUDは攻略に必要な情報だけを表示する
    前提 <encounter> を -DebugGuidance なしで開始している
    ならば HUDは危険の予告、GrabまたはCling操作、Stamina、禍根浄化数、落下Recovery案内、Retryを状況に応じて表示する
    かつ primitive encounter、内部State、Phase、Route Node番号、倍率、計測値を表示しない
    かつ 峰抱きの経路案内は内部Node番号ではなく次の行動を短文で示す

    例:
      | encounter |
      | 石走り    |
      | 淵纏い    |
      | 峰抱き    |
      | 禍津根    |

  シナリオアウトライン: 開発表示は明示的に復元できる
    前提 <encounter> を -DebugGuidance 付きで開始している
    ならば HUDは通常プレイ情報に加えて内部State、Phase、Route Nodeまたはテレメトリーを表示する

    例:
      | encounter |
      | 石走り    |
      | 淵纏い    |
      | 峰抱き    |
      | 禍津根    |

  シナリオ: HUD背景は表示行を収める
    前提 Sense表示または-DebugGuidanceによってHUD行が増えている
    ならば HUD背景の高さは実際の表示行数と描画Scaleから決まる
    かつ 長い通常案内は一行に収まる短文を優先する

  シナリオ: HUD整理はゲーム進行を変更しない
    ならば 入力条件、当たり判定、Boss状態、Campaign進行、Sense、勝利判定は既存実装のままである
