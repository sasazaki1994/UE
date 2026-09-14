# language: ja
機能: 禍津根 Primitive Vertical Slice
  長年蓄積した禍を殺すのではなく、三つの流れを断って地形を一時的に鎮める。

  シナリオ: 三禍根から鎮静までを共通Lifecycleで進める
    前提 禍津根EncounterがKAKON 0/3で開始している
    もし プレイヤー入力だけでRoot A、Root B、中央核の禍根を順に浄化する
    ならば 共通Nushi Progressは1/3、2/3、3/3と進む
    かつ 状態はCalming、Calm、Encounter Completed、Victoryへ進む
    かつ 禍津根は消滅せず動きと黒い脈動だけが弱まる

  シナリオアウトライン: 動くRootへのGrab追従
    前提 プレイヤーが既存GrabでRootへ接続している
    もし Rootを<fps> FPS相当のDeltaSecondsでTranslateおよびRotateする
    ならば Grab対象との相対Transformは維持される
    例:
      | fps |
      | 30  |
      | 60  |
      | 120 |

  シナリオ: Phaseに応じてRouteが変わる
    もし 禍根1を浄化する
    ならば Rootと岩柱を往復するPhase 2 Routeが開く
    もし 禍根2を浄化する
    ならば 大規模脈動の後に中央核へのFinal Routeが開く

  シナリオ: 落下Recoveryは進行を保持する
    前提 共通Progressが1/3または2/3である
    もし プレイヤーが意図的に落下する
    ならば 対応する安全棚へRecoveryする
    かつ Progressを保持して通常Grab入力で復帰できる

  シナリオ: Retryは全状態を戻しActorを増やさない
    前提 Encounterが途中またはVictory後である
    もし RまたはYを入力する
    ならば Root、Route、Kakon、Progress、State、Encounter、Player、Stamina、Grab、Recoveryが初期状態へ戻る
    かつ Actor数は増えない
