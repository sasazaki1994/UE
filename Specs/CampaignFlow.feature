# language: ja
機能: 4体の主を接続する最小Campaign
  シナリオ: 固定された章順を完走する
    前提 CampaignがTitleにいる
    もし Prologueを進め各EncounterのCompletedを順に通知する
    ならば Ishibashiri、Fuchimatoi、Minedaki、Magatsune、Endingの順に進む
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
