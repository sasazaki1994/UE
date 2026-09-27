# 石走り編 初見プレイ検証

Status: **READY / HUMAN PLAYTEST NOT_RUN**

## 次に行う1作業

Windows UE 5.6.1でReview Gateを再度広げたり、新しい演出を先に足したりせず、現在のPackageを開発に参加していない1人へ渡し、Titleから石走り編の結末2枚を経てTitleへ戻るまでを無説明で観察する。2026-09-27の自動Review Gateは完走、Retry、Sense、保存隔離、描画Captureを確認したが、初見の人が「観察 → 回避 → 登攀 → 鎮静」を理解できることは証明していない。この不確実性を最初に解消する。

これは統計的なユーザーテストではなく、最初の導線阻害を発見するための1セッションである。結果を一般化せず、修正後は別の初見参加者で再確認する。

## 守る契約

- `-IshibashiriDemo` の `Title → Prologue → IshibashiriApproach → Ishibashiri → 結末2枚 → Title` を使用する。
- 通常Campaign、`MagabaraiCampaign`、3禍根、Calm / Encounter Completedのauthorityを変更しない。
- 観察者は開始前に操作、正解ルート、Sense、回避、Grab、Cling、浄化を説明しない。安全・機器トラブル以外では介入しない。
- 物理ゲームパッドまたはkeyboard/mouseのどちらを使ったかを記録する。模擬gamepad結果を物理機器の証拠にしない。
- 録画または時刻付き観察メモへの同意を得る。個人名、音声、映像は同意した範囲だけで保存し、リポジトリへcommitしない。
- テレメトリや観察結果がない段階で、Route座標、戦闘timing、カメラ、字幕を推測だけで調整しない。

## セッション手順

1. Review Gateを通したものと同じSHAのPackage、画面解像度、入力機器、開始・終了時刻を記録する。通常Campaignの保存はDemoから隔離されるが、参加者には保存操作を要求しない。
2. 「石走り編を最後まで遊んでください。考えていることは話しても話さなくても構いません」だけを伝えてTitleから開始する。
3. 次のbeatについて、最初に意味を理解した時刻、失敗回数、介入の有無を記録する。
   - Title開始とPrologue送り
   - 接近路の痕跡、境界警告、盆地への到達
   - 突進予告の認識、横回避、Recover中の反撃
   - Kneel中の前脚Grab
   - Route移動、ShakeでのCling、休息によるStamina回復
   - 3つの禍根の浄化、Calm、結末2枚、Title復帰
4. Retryした場合は、失敗原因を参加者が説明できたか、Retry後に同じ地点を自力で越えたかを記録する。明確な進展が10分ない、体調不良、または参加者が中止を望む場合は終了する。
5. 終了後だけ「迷った場所」「役に立った手掛かり」「石走りを倒したのか鎮めたのか」を短く質問する。

## 記録テンプレート

機械検査する正本は `Docs/IshibashiriFirstPlaytestValidation.json` である。セッション前にリポジトリ外へコピーし、個人名を入れずに記録した後、次を実行する。リポジトリ内の正本は実セッションの証拠が追加されるまで`NOT_RUN`のまま維持する。

```powershell
python Tools/ValidateIshibashiriFirstPlaytest.py <記録JSONのパス>
```

validatorは自動E2Eを実行せず、`NOT_RUN`をPASSへ昇格させない。PASS / FAILではfull source SHA、配布したPackageのSHA-256、非開発参加者、実入力機器、同意、時刻、全beatの記録、リポジトリ外のEvidence保存先を検査する。観察したbeatの`firstUnderstoodAt`はセッション時間内でなければならない。中止または未到達のbeatは`observed: false`、`firstUnderstoodAt: null`と理由を記録し、PASSだけは全beatの観察を必須にする。物理gamepadの場合は機器名も必須である。

`packageSha256`は、参加者へ渡したzipなど単一の配布物に対して`Get-FileHash -Algorithm SHA256 <Package>`で取得する。実行ファイルだけでなく隣接Contentも必要なため、展開済みフォルダー内のexeだけを配布物全体のhashとして記録しない。

```text
Source SHA / Package / Package SHA-256:
Date / observer:
Input device (physical model or keyboard/mouse):
Viewport / quality mode:
Consent scope (notes / screen / audio):
Start / end / completion time:
Completed without coaching: YES / NO / NOT_RUN
Returned to Title after two ending cards: YES / NO / NOT_RUN
Understood calm rather than death: YES / NO / UNCLEAR / NOT_RUN

Beat | first-understood time | failures | coaching | observation
Title / Prologue |
Approach / boundary |
Charge / dodge |
Recover / counter |
Kneel / Grab |
Route / Cling / rest |
Kakon 1 / 2 / 3 |
Calm / ending / Title |

Participant-reported blockers:
Observer interpretation (keep separate from observations):
Proposed smallest change:
Evidence location (outside git):
```

## 判定と次の変更境界

最初のセッションの合格条件は、開発者の攻略説明なしで結末2枚後のTitle復帰まで到達し、失敗またはRetryがあっても原因を自分の言葉で説明でき、石走りを「死亡させた」のではなく「鎮めた」と理解できることである。目標所要時間は既存方針どおり10〜20分だが、1人の超過だけで難易度を一括変更しない。

不合格時は最初に詰まった1地点だけを対象にする。修正優先順は、既存手掛かりの配置または表示時間 → 短い文言 → camera framing → gameplay timingである。新能力、常時marker、Route authority変更、敵HP、追加チュートリアル画面はこの最小対応に含めない。変更ごとにAcceptance specとsource-contractを更新し、自動Demo Review Gateを再実行した後、別の初見参加者で確認する。

人間の記録が作成されるまでは結果を **NOT_RUN** とし、自動E2Eや14枚CaptureからPASSを推定しない。
