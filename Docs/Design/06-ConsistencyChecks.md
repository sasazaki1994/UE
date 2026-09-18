# 6. 設定とソースの矛盾を自動検査

## 実行

Python 3.11以降の標準ライブラリだけを使う。UE、Blender、pytestは不要。

```bash
python Tools/ValidateNarrativeContracts.py
python -m unittest discover -s Tests -p 'test_narrative_contracts.py' -v
```

[NarrativeContract.json](NarrativeContract.json) が制作ルールの機械可読表現であり、ランタイムへ読み込むデータではない。[専用workflow](../../.github/workflows/narrative-contracts.yml) はLinuxで上記2コマンドだけを実行する。エラー時は終了コード1。UE検証結果は出さない。

## 今回、自動で確認すること

| 項目 | 静的チェックの範囲 |
|---|---|
| 外見2パターン | Early / Advanced、Interlude2切替、各章の対応、共通資産、性能不変を契約JSONで確認 |
| 設定の禁止事項 | 仮面の封印機能、祓った主からの穢れ吸収、禍津根の第四主化が契約JSONで有効になっていないこと |
| Campaign | 現行enumの12状態と対応表が一致、淵纏い→Interlude2→峰抱きの接続が残っていること |
| 直接鎮静の再混入 | 本番 `.cpp/.h` の明示的な `CalmNushi()` 呼び出しを列挙し、共通の全浄化ハンドラ以外を拒否 |
| 完了通知の所有者 | 明示的な `OnEncounterCompleted.Broadcast()` と `FinishEncounter(true)` の位置を限定 |
| 石走りの旧HP経路 | TryReceiveCounterに体勢操作があること、同関数にHealth参照や直接完了がないこと |
| Senseの責務 | Sense内の明示的なPurify呼び出し・鎮静・完了通知を拒否。ログ関数名は誤検出しない |
| Retry・Travel | 現行4PlayerのResetForEncounterとCampaignの既知関数内にSense Resetがあること |
| 表示文言 | HUD/Playerの文字列中のBoss HP等、対象を絞った旧表現を検出。Playerの死亡やDeadTreeは許容 |

テストは壊したソース断片をメモリ上で与え、禁止呼出しの再導入や第三の外見パターンで失敗することも確認する。コメントや通常の文字列内の呼び出し例は実行コードと区別する。

## 検査の限界

これはC++コンパイラ、AST解析、全経路証明ではなく、現在の書き方に対するsource-contract lintである。間接呼び出し、関数ポインタ、別名、複雑なマクロ、Blueprint、バイナリアセットまでは保証しない。C++ raw stringは通常文字列と同じ精度では解析しない。

マークダウン全体の自然言語の意味も判定しない。「仮面は封印具ではない」「禍津根は第四の主ではない」という否定文を単語一致でエラーにしない。物語本文とJSONの意味の一致は人がレビューする。

特に以下は静的PASSの範囲外。

- 実際に各主へ3個のKakonが生成され、全て祓えること。契約上の数と実行時の登録数は別物。
- 3/3以外に未知の勝利経路が絶対に存在しないこと。
- RetryでActor数が増えない、Delegateが重複しないこと。
- 外見切替とRetry保持がゲーム内で動作すること。外見機能自体が未実装。
- Senseリスクが全Encounterで正しく適用されること。石走りの既存回復処理を含め別途確認が必要。
- 膝つきの視認性、Grab距離、反撃部位、カメラ、字幕の読みやすさ、音の聞こえ方。
- 主が生存して見えること、素顔と穢れの差分の品質、マテリアルやリグの成立。

`Kill`、`Dead`、`Destroy()` を全ファイルから機械的に禁止しない。Player死亡、腐食樹木、World破棄時のcleanup、テスト用State fixtureまで禁止すると正しいコードを壊す。

## 後日Windows UEで確認すること

1. ビルドと既存Automation。
2. 地上反撃だけでは石走りが鎮まらず、体勢崩しから登攀できること。
3. 4戦で1/3・2/3はRunning、3/3はCalm→Completed、Retry後は未浄化へ戻ること。
4. TitleからEndingまでの入力通し、再START、落下復帰、物理ゲームパッド。
5. 二段階外見の実装後にInterlude2切替、Retry保持、New GameでEarlyへ戻ること。
6. マーカーを減らしても攻略でき、主の鎮静と白面の変化が読み取れること。

未実行は `NOT_RUN`、資産なしは `NOT_IMPLEMENTED` と記録する。古い主戦のPASSを最新mainの検証結果として流用しない。
