# language: ja
機能: 4体の主を接続する最小Campaign
  シナリオ: 固定された章順を完走する
    前提 CampaignがTitleにいる
    もし Prologueを進め各EncounterのCompletedを順に通知する
    ならば IshibashiriApproach、Ishibashiri、Fuchimatoi、Minedaki、Magatsune、Endingの順に進む
    かつ Ending後はCompletedになる

  シナリオ: 未完了またはRetryでは進まない
    前提 CampaignがEncounter中である
    もし 現在EncounterをRetryする
    ならば Campaign Chapterは変わらない
    かつ 別EncounterのCompleted通知は拒否される

  シナリオ: Chapter境界でSenseを破棄する
    前提 Boundary SenseまたはCorruption Senseを保持している
    もし 次ChapterへTravelする
    ならば 両SenseはOFFになる
    かつ Corruption risk tailは0になる

  @e2e @input-only
  シナリオ: Titleから4Encounterを通常入力で連続攻略する
    前提 CampaignがTitleにいる
    もし PlayerController入力でカードと各Encounterの既存攻略Driverを進める
    かつ 石走りで通常入力のRetry後に同じChapterを再攻略する
    ならば Title、Prologue、IshibashiriApproach、Ishibashiri、Interlude1、Fuchimatoi、Interlude2、Minedaki、Interlude3、Magatsune、Ending、Completedの順で通過する
    かつ 攻略目的の位置、Kakon、Phase、Stamina、Progress、Completed、Campaign Stateの直接書き換えを行わない
    かつ 各EncounterはKakon 3/3、Calm、Completed、Victoryを通過する
    かつ 禍津根は死亡せず鎮静する

  @approach @input-only
  シナリオ: 山道を歩いて初めて石走りと対面する
    前提 Prologueが終了しIshibashiriApproachが開始している
    もし Playerが一本道の山道を歩き境界石、痕跡、遠距離Revealを順に通過する
    ならば Revealは1回だけ2〜4秒表示されCombatを開始しない
    かつ Basin gate到達後にIshibashiriへ遷移する
    かつ ApproachのTimerとSenseは次Chapterへ漏れない

  シナリオ: 章から白面の穢れ段階を一意に導出する
    前提 新しいCampaignはPrologueにいる
    ならば 穢れ段階はEarlyである
    もし 淵纏いを鎮めFuchimatoiからInterlude2へ遷移する
    ならば カード送りを待たず穢れ段階はAdvancedになる
    かつ EndingまでAdvancedを保持する
    かつ TitleとCompletedには外見段階を適用しない

  シナリオ: EncounterのRetryで穢れ段階を保持する
    前提 CampaignがEarlyまたはAdvancedのEncounter中である
    もし 各GameModeのRetryEncounterが共通Retry通知を通ってEncounterをリセットする
    ならば Campaign Chapterと穢れ段階は変わらない

  シナリオ: 単体起動はEncounterから穢れ段階を決める
    前提 Campaignが無効でGameInstanceの章初期値がTitleである
    ならば 石走りと淵纏いはEarlyになる
    かつ 峰抱きと禍津根はAdvancedになる
