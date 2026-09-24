# language: ja
機能: 4体の主を接続する最小Campaign
  シナリオ: 固定された章順を完走する
    前提 CampaignがTitleにいる
    もし Prologueを進め各EncounterのCompletedを順に通知する
    ならば IshibashiriApproach、Ishibashiri、Fuchimatoi、Minedaki、Magatsune、Endingの順に進む
    かつ Ending後はCompletedになる

  シナリオ: 物語カードを機械可読契約どおり表示する
    前提 NarrativeContract.jsonのstory_cardsが章別の正本である
    もし CampaignHUDの物語カードをソース検査する
    ならば 各章の本文は正本と同じ順序かつ同じ枚数である
    かつ 本文の章移動、順序変更、重複、欠落は検査に失敗する

  シナリオ: 未完了またはRetryでは進まない
    前提 CampaignがEncounter中である
    もし 現在EncounterをRetryする
    ならば Campaign Chapterは変わらない
    かつ 別EncounterのCompleted通知は拒否される

  シナリオ: 終了後に章の先頭から再開する
    前提 Campaignが峰抱きの途中で終了した
    もし TitleでRまたはYを押してContinueする
    ならば 峰抱きの先頭から開始する
    かつ 禍根、スタミナ、Boss PhaseはGameModeの初期値になる
    かつ 穢れ段階はAdvancedのままである

  シナリオ: 自動入力テストは通常の保存領域を変更しない
    前提 続きから再開できる保存データがある
    もし Campaign E2Eを実行する
    ならば テストの進行は通常の保存データを上書きしない

  シナリオ: 保存があるTitleで新規開始を確認する
    前提 続きから再開できる保存データがある
    もし TitleでSpaceまたはXを1回押す
    ならば 上書き警告を表示し、ゲームを開始せず保存も変更しない
    もし 同じ開始入力をもう1回行う
    ならば Prologueを開始し、従来の保存失敗処理を使う

  シナリオ: 新規開始の確認を取り消す
    前提 保存があるTitleで上書き確認を表示している
    もし キーボードのEscまたはゲームパッドのBを押す
    ならば 確認を取り消し、Titleと保存を変更しない
    かつ 確認中でもRまたはYで保存章をContinueできる

  シナリオ: 保存がないTitleは従来どおり開始する
    前提 Continueできる保存データがない
    もし TitleでSpaceまたはXを1回押す
    ならば 確認を表示せずPrologueを開始する

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
