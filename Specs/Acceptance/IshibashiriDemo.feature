# language: ja
機能: 保存から隔離された石走り編専用デモ
  シナリオ: デモは石走り編だけを完走する
    前提 アプリケーションをIshibashiri Demoモードで起動している
    もし プレイヤーがTitleからPrologueとIshibashiriApproachを進む
    かつ 3つの禍根をすべて祓い石走りを鎮める
    ならば 石走り編の短い結末2枚だけを表示する
    かつ Titleへ戻る
    かつ Fuchimatoiを開始しない
    かつ 正常なTitle復帰後にISHIBASHIRI_DEMO_COMPLETEを1回だけ記録する

  シナリオ: デモはCampaign persistenceを変更しない
    前提 MagabaraiCampaignに本編の続きが保存されている
    もし Ishibashiri Demoを開始して完走する
    ならば 既存保存を読まない
    かつ 既存保存を書かない
    かつ 既存保存を削除しない
    かつ Titleで保存上書き確認を表示しない

  シナリオ: 通常Campaignは変更されない
    前提 アプリケーションをCampaignモードで起動している
    もし 石走りを鎮める
    ならば Interlude1の3枚を従来どおり表示する
    かつ Fuchimatoi、Minedaki、Magatsune、Ending、Completedへ従来どおり進む
    かつ ContinueとMagabaraiCampaign保存を従来どおり使用する
