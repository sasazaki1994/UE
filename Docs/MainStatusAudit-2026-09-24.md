# main 開発状況監査（2026-09-24）

## 監査基準と制約

- 監査対象は checkout と `.git/FETCH_HEAD` がともに示す `c4bacbd17dcdf6dc8f42842ac108ab9ee5689d5f`（PR #76 統合コミット）。開始時の作業ツリーは clean だった。
- `git ls-remote https://github.com/sasazaki1994/UE.git refs/heads/main` で再取得を試みたが、proxy の CONNECT が HTTP 403 を返した。このため、これは **ローカルに記録された main** の監査であり、2026-09-24 時点の GitHub remote tip を独立に最新確認できたという主張ではない。
- Unreal Engine、画像生成、外部音源サイトは使用していない。現在 SHA では UE build、Automation、入力攻略、描画、実機音声確認を実行していない。静的検査の PASS は以下の「UE 実機検証」には含めない。
- 日付・基準 SHA 付きの既存記録は当時の履歴として残す。たとえば `MergeMainValidation.json` の UE 成功は 2026-09-09 / `6d08e29` の結果であり、現在 SHA の証明ではない。

## 状況一覧

「素材」は `git ls-files` でこの main に追跡される実ファイルだけを数える。コードが参照する予定名、生成手順、別 PR のファイルは素材に含めない。

| 対象 | 実装済みのコード | main に実在する素材 | 実行済みの検証 | 残る作業 |
|---|---|---|---|---|
| Campaign | `UCampaignGameInstance` の固定章遷移、各 GameMode の完了通知、`CampaignPlayerController` の入力 E2E coordinator がある。全章は既存 `L_Prototype_01` と GameMode 切替を使う。 | 専用 Campaign map、完成島、ムービーはない。既存マップ・4戦の下記素材を再利用する。 | 現在 SHA では静的 source-contract のみ。`Docs/CampaignE2EValidation.md` にある Campaign UE matrix は全て NOT_RUN。 | Windows UE 5.6.1 build、60/30 FPS keyboard、gamepad、Retry・Sense reset・Ending を1プロセスで確認し capture を保存する。 |
| 4戦（石走り・淵纏い・峰抱き・禍津根） | 4つの GameMode / Player / Boss / HUD と共通 `NushiEncounterManager`、各入力 driver があり、Campaign の Completed に接続されている。 | 石走りと白面の rigged FBX/GLB/texture、`Content/Characters/Rigged/` の UE asset、各戦がコード生成する primitive presentation はある。完成した淵纏い・峰抱き・禍津根の production character/audio/map asset はない。 | 2026-09-09 の旧 SHA には石走り中心の UE 記録があるが、現 SHA の4戦 Campaign 実機証明はない。各 validation JSON の過去結果は再利用しない。 | 4戦を単体と Campaign の両方で攻略し、Kakon 3/3 → Calm → Completed → Victory、落下復帰、Retry、collision、camera、frame-rate 差を確認する。 |
| 白面の二段階外見 | 章から `Early` / `Advanced` を解決し、`UShirotsuraVisualComponent` が監査済み parameter のある slot だけへ値を設定する。PR #69 は顔・首 provenance mask の生成・安全な接続手順も追加した。 | 既存白面 mesh、animation、base/normal/roughness と UE asset はある。一方 `T_Shirotsura_FaceNeckMask.png` はなく、チェックイン済み FBX / `.uasset` はその手順で再生成されていない。したがって **顔・首の実素材は未生成**。 | PR #69 の Python source test は手順と fail-safe を検査できるだけ。Blender bake、UE build、mask 表示、Early/Advanced 比較は NOT_RUN。 | Blender で mask を生成し、UV/bleed と右手・仮面が黒であることを人が確認後、UE material を再生成する。同一カメラ比較と Retry 回帰を行う。 |
| HUD | 4戦の Canvas HUD は通常表示と `-DebugGuidance` を分離し、表示行数から背景高を計算する。Campaign card HUD と Sense 表示もコードにある。 | 独立した UI art / Widget asset はなく、フォントと Canvas 描画によるコード表示。 | HUD source-contract は実行可能だが、解像度、Japanese glyph、重なり、画面外 recovery 行、gamepad 表記の UE 描画確認は現 SHA で NOT_RUN。 | 4戦を通常 / debug、複数解像度で capture し、背景、可読性、marker と camera の干渉を目視する。 |
| セーブ | `UCampaignSaveGame` が version と章境界を `MagabaraiCampaign` slot に保存する。Continue、破損/未知 version 拒否、上書き二段確認、取消、完了時削除、E2E の persistence 無効化がある。戦闘途中の値は保存しない。 | SaveGame class は C++。事前生成した save binary や fixture asset は収録しない。 | source-contract と C++ Automation のテストコードはあるが、現在 SHA で C++ Automation と終了・再起動を伴う実保存は NOT_RUN。 | Windows で新規、各章再開、Retry、上書き取消/確定、削除失敗、未知 version、通常 save を E2E が汚さないことを確認する。 |
| 物語カード | `CampaignHUD.cpp` に Title / Prologue / 3 Interlude / Ending / Completed の本文と折返しがあり、`NarrativeContract.json` と順序・枚数・本文を静的照合する。 | 物語カード専用の静止画像は main にない。黒背景、文字、prompt は Canvas 描画。島概念図 SVG は設計資料でありカード実素材ではない。 | narrative validator と Python tests は本文契約を検証する。日本語 glyph、viewport、送り入力、実表示 capture は現 SHA で NOT_RUN。 | UE で各カード、狭い viewport、keyboard/gamepad の送り、上書き警告、Ending/Completed を描画確認する。 |
| 共通効果音 | PR #70 はライセンス情報と形式を必須にする manifest validator / test / acceptance gate を追加した。ゲーム内 playback 実装や音源そのものではない。 | `Audio/CommonSfx/manifest.json` の6役は **全件 `pending`**、delivery は `blocked`。WAV 6個も preview WAV も存在しない。 | manifest の pending 状態に対する静的検査のみ。試聴 `not_performed`、UE import `not_performed`。 | 外部接続可能な別工程で候補を試聴・ライセンス確認し、48 kHz / 16-bit / mono を納品して validator、聴感、UE import、定位・音量を確認する。 |

## PR #69、#70、#71 の境界

- **PR #69（main に merge 済み）**: `0ce6b45` / `532405c`。顔・首 mask を provenance から生成するコードと、mask がなければ shared skin を変更しない契約を収録した。生成コマンドを実行した成果物ではないため、顔・首用 PNG / 更新 `.uasset` / 描画比較は未納品である。
- **PR #70（main に merge 済み）**: `ddffef3` / `dee7054`。6音を納品した PR ではなく intake gate を収録した PR。manifest の6件は pending で、音源サイト未確認、WAVなし、試聴なし、UE importなしである。
- **PR #71（未マージ）**: main の merge 履歴は #70 の次が #72 で、#71 の merge commit はない。PR #71 側だけにある画像はこの監査の main 素材には含めない。なお main に元から追跡される `Art/References/Tripo/Fuchimatoi/{Front,Back,Left,Right}.jpg`（commit `99eb3e7`）とは別扱いであり、それらの存在から未マージ PR #71 の画像が入ったとは推定しない。GitHub API/PR画面は接続制約により再確認できず、PR #71 の画像名・内容・head SHA は **確認不能**。

## UE 環境復旧後の優先検証

同じ build 成果物を使い、失敗時は先へ進まずログと RunId を保存する。

1. **compile と Automation の土台** — `./Tools/Prototype.ps1 -Action Build`。続いて Editor の Automation で `IshibashiriPrototype.Campaign`、`Campaign.CardTextWrapping`、`ProductionVisuals.*` を実行する。
2. **Campaign 最重要 E2E** — `./Tools/Prototype.ps1 -Action Test -Campaign -SkipBuild -TestFPS 60 -Capture`。章順、4戦、Retry、Sense reset、カード、Ending と capture を一度に確認する。
3. **固定刻みと入力差** — `./Tools/Prototype.ps1 -Action Test -Campaign -SkipBuild -TestFPS 30`、次に `./Tools/Prototype.ps1 -Action Test -Campaign -SkipBuild -Gamepad -TestFPS 60`。模擬 gamepad の PASS を物理 controller 確認とは呼ばない。
4. **各戦の切り分け** — `-Action Test -Climbing`、`-Fuchimatoi`、`-Minedaki`、`-Magatsune` を 60/30 FPS と必要な recovery/capture option で個別実行し、Campaign 失敗を局所化する。
5. **見た目と HUD** — mask asset をレビュー・生成した後だけ、Early/Advanced の同一 camera capture、通常 / `-DebugGuidance`、複数 viewport、日本語カードを比較する。素材未生成のまま「二段階外見 PASS」にしない。
6. **保存の再起動試験** — `-Action Play -Campaign` で各章境界からプロセスを終了・再開し、上書き確認、取消、Completed の削除、破損/未知 version、E2E 隔離を確認する。
7. **音声は納品後** — 6 WAV と preview が揃って `python Tools/ValidateCommonSfx.py` を通った後にだけ UE import、イベント発火、音量、定位、masking を確認する。現在は実行対象の音がない。

## 今回確認できなかった項目

GitHub remote の最新 tip、PR #71 の head と画像内容、Windows UE build/UHT、C++ Automation、Campaign/4戦の入力攻略、実機 save、カード/HUD/二段階外見の描画、顔・首 mask bake、物理 controller、音源試聴・ライセンス・WAV・UE import。これらを静的テスト結果から補完しない。
