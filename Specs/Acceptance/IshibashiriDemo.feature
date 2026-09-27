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

  シナリオ: 接近路の警告字幕は画面幅に応じて読める
    前提 プレイヤーが接近路の境界を越える
    もし 「ここから先へ入るな」の警告が表示される
    ならば 字幕は実際の文字幅で画面中央に配置される
    かつ 暗い半透明背景と控えめな境界線で地形から分離される
    かつ 小さなビューポートでも左右の安全領域内に収まる

  シナリオ: 初見プレイは自動E2Eと分離して評価する
    前提 開発に参加していないプレイヤーが操作説明なしでPackageを開始する
    もし Titleから石走り編の結末を経てTitleへ戻るまでを観察する
    ならば 各主要beatの理解時刻、失敗、Retry、介入を入力機器とsource SHAに結び付けて記録する
    かつ プレイヤーが失敗原因と石走りを鎮めた結末を説明できるか記録する
    かつ 人間の記録がない場合は自動E2EやCaptureがPASSでも初見プレイをNOT_RUNとする

  シナリオ: 初見プレイ記録を機械検査する
    前提 初見プレイ記録はまだNOT_RUNである
    もし 記録validatorを実行する
    ならば 人間によるPASSを作らずNOT_RUNと報告する
    かつ PASSまたはFAILを記録する場合はsource SHA、PackageのSHA-256、非開発参加者、実入力機器、同意範囲、全主要beatを必須にする
    かつ 観察したbeatの理解時刻はセッションの開始から終了までの範囲内にする
    かつ PASSは無介入の完走、結末後のTitle復帰、鎮静の理解をすべて必要とする
