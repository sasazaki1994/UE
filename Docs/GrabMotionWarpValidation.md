# 白面 Grab Motion Warping Vertical Slice

## Scope / 調査結果

対象は **地上 → Grab成立 → 石走り前脚の既存Route node 0** のみ。11点Route、前脚→肩岩のFBIK、Camera、Stamina、Shake/Cling、Retry、Legacy/High Quality renderingは置換しない。

変更前のroute Grabは、`GrabPressed()`がnode 0との距離240 cm以内かつ非Chargeを確認し、CharacterMovementを即時停止して `SetActorLocation(GetClimbPosition(0))` を呼ぶ方式だった。補間、Attach、Root Motionはなく、その後のtickもroute位置へ直接移動する。汎用`UGrabComponent`は別fixtureであり、開始位置のboss-relative transformを保存し、PostPhysics tickで`SetActorTransform(...TeleportPhysics)`する。Player animationは10本の`UAnimSequence`を`AnimationSingleNode`で直接再生し、既存Grab/Climb/Grip clipにRoot Motion/Montage/warp notifyの契約はない。このため既存clipへMotion Warpingを無理に適用していない。

## Plugin / module

UE 5.6標準`MotionWarping` pluginとRuntime moduleだけを追加した。既存`PythonScriptPlugin`、`ControlRig`、`FullBodyIK`は維持する。外部pluginはない。

## Asset contract（Editorで作成が必要）

このLinux checkoutではbinary `.uasset`を生成していない。UE 5.6 Editorで以下を作成するまで見た目は **UNVERIFIED** である。

1. `SK_Shirotsura`用に、地上poseから一歩踏み込み両手が前脚へ触れる直前までroot boneが前進する短い専用sequenceを作る。既存10 clipは変更しない。
2. Root Motionを有効にし、`AM_Shirotsura_Grab_Ishibashiri` Montageへ入れる。
3. 接近開始〜手の接触直前（推奨0.18〜0.72秒、実clipに合わせる）だけ`Motion Warping` notify stateを置く。Warp Target Name=`IshibashiriGrab`、Translation/Rotation warpを有効、Ignore Zは無効にする。開始poseや接触後へwindowを広げない。
4. Default Slotを持ち、Root Motion Mode=`Root Motion from Montages Only`の最小Anim Blueprintを作る。
5. `APrototypePlayer` defaultsの`GrabMotionWarpMontage`と`GrabMotionWarpAnimClass`へ割り当てる。

Asset未設定時は警告を記録して従来route mountへfallbackする。これはGameplay regression/soft lockを避ける安全経路で、Motion Warp成功とは扱わない。

## Target / limits / moving boss

Target名は`IshibashiriGrab`。新しいhold座標は追加せず、位置は毎frame `AIshibashiriBoss::GetClimbPosition(0)`、向きはその位置からboss actor中心への水平方向とする。`GetClimbPosition`自体がCreature component localの既存Route[0]をcomponent transformでworldへ変換するため、歩行、旋回、軽い上下動に追従し、Grab開始時world座標へ固定されない。

2026-10-01の前脚Grab補助ではGrabRange=300 cm、MaximumWarpDistance=300 cm、MaximumWarpAngle=120度を使用する。距離はboss中心ではなく既存node 0へのworld距離、角度は既存の取り付き姿勢とのyaw差である。完了許容は位置35 cm/角度18度を維持し、超過時はnodeへsnapせずcancelする。180度吸着をしない。開始distance/angle/durationと完了errorはlogへ記録する。実機での操作感は未検証である。

## Gameplay hand-off / FBIK

Gameplay成立条件を通過後にだけtarget/montageを開始する。Warp中はRouteの`Boss`をまだ設定しないため`IsIKVerticalSlice()`はfalse、IK Weightはgroundの0を維持する。完了許容内でのみ既存node 0へ制御を渡し、それ以降は従来の0.22秒FBIK blend-inが担当する。Motion Warpingはroot、FBIKは四肢/pelvis/spine接触補正という責務を分離する。PR #30のFBIK Runtimeは引き続き **UNVERIFIED**。

## Failure / reset / camera

Target消失、Charge遷移、死亡、fall、Detach、Retryは共通cancelでwarp targetとmontageを除去し、速度/forceをclearしてMovementをFallingへ戻す。既存どおりGrab releaseはbrace入力だけを解除し、開始済みGrab自体は中断しない。Retryは続けてspawnへresetしてWalkingとなる。完了誤差超過も同じ経路で、Attach残留はない。既存spring-arm/climbing cameraには変更せず、warp中はground cameraを継続する。

Development/Testで`-GrabWarpDebug`を指定するとtarget frame、current root、start root、root-target線を表示する。Shippingではcompileされない。logの`GRAB_WARP_START` / `COMPLETE` / `CANCEL`でstateと数値を確認する。

## Validation / capture

Windows UE 5.6でasset割当後、`./Tools/Prototype.ps1 -Action Test -GrabMotionWarp -Capture -Basin -SkipBuild -TestFPS 60`を実行する。専用testは通常PlayerStartからW入力で接近し、E入力からWarp、Attach、Climb開始まで進め、途中teleportを使わない。保存先は`Saved/Screenshots/GrabMotionWarp/<RunId>/`で、`01-BeforeGrab`、`02-WarpStart`、`03-Approach`、`04-BeforeContact`、`05-Attached`、`06-ClimbStart`を同一camera/FPSで取得する。Before比較はassetを外した別runへ保存し、自動testのAfterを上書きしない。

Regression matrix: Build、Camera、Grab、Climbing 60/30 FPS、ClimbingGamepad、ClimbingIK、BasinScenario、Retry、Legacy、High Quality。本環境にはWindows UE 5.6、MSVC、RHIと必要Montage/AnimBP assetがないため、Editor Build、Montage生成、Motion Warping Runtime、FBIK Runtime、FPS、screenshots、位置/角度誤差、見た目改善はすべて **UNVERIFIED**。Static Python validationだけを結果として記録する。

## 2026-09-24 source update: asset missing fallback

Montage / AnimBPが無い通常プレイでも、Grab成立直後にnode 0へ瞬間移動せず、約0.48秒で移動する。開始transformは石走りrelativeで保持するため、接近中の歩行・旋回にも運ばれる。向きは移動より早く揃え、既存Climb clipによる手を伸ばす読みを先行させ、最後の22%で接触へ収束してからGrabを成立させる。Grab中は画角を88度へ穏やかに寄せ、成立後は100度へ戻して巨体と経路を見せる。移動中に対象が無効になった場合とRetryではMovementと衝突無視を解除する。これはRoot Motionを使うMotion Warpingや専用Grab animationではなく、既存assetとC++だけの安全な簡易表示である。専用`-GrabMotionWarp`テストは引き続き本物のMontageを要求し、このfallbackを合格扱いにしない。Windows UEでの描画・衝突・カメラ検証は未実行。

## Known issues

- 専用Root Motion clip、Montage、AnimBPはartist/editor作業待ち。存在しないassetを生成済みとは扱わない。
- 35 cm/18度とnotify時刻はcaptureで足滑り、手の貫通、camera clippingを見て確定する。
- 今回はnode 1以降、自由登攀、全身Skeleton/Camera redesignへ展開しない。

## 2026-10-02 source update: foreleg Grab assistance

対象は前脚の既存node 0だけである。膝つき中かつnode 0から300 cm以内の失敗したE / RB入力は、同じ石走りを対象に0.25秒だけ保持する。向き直りや動作終了によってその間に開始条件を満たせば、再度押さずに既存の接近動作へ進む。残り時間を減らしてから再判定するため、期限切れの入力で遅れて取り付くことはない。ボタンを離してもしがみつき状態だけが解除され、この短い取り付き入力は取り消さない。

範囲外や膝つき前の押下は予約しない。保持中に対象が交代・消失した場合、膝つきが終了した場合、範囲を離れた場合、Encounterが終了した場合、死亡、Detach、Resetでは予約を破棄する。予約が次の膝つき、再挑戦、新しい対象に持ち越されることはない。短い失敗理由を表示し、膝つき中の操作ヒントはboss中心ではなくnode 0から400 cm以内で表示する。400 cmは案内を始める距離であり、300 cmの取り付き範囲を広げない。

**角・頭からの第二入口は未実装。** 現在の生成モデルの角上の点`(-261,-531,185)` cmとRoute[0]の`(-270,-215,65)` cmは、同じモデル座標系で約338 cm離れている。その点を開始地点にすると300 cmの接近上限を超える。第二の印だけを追加して取り付けると案内せず、頭の姿勢または途中の接近動作を設計・実機確認する後続作業として残す。11地点Route、休息点、戦闘タイミングは今回の対象外である。

Windows UE 5.6 Editorビルド、`IshibashiriPrototype.Climbing.GrabAssist` Automation（300 cm/120度の境界、0.25秒の期限、Reset/死亡/対象変更、30/60/120 FPS fixture）、入力駆動の登攀E2E（30/60 FPS）、Python回帰185件は **PASS**。60 FPS描画Captureでは前脚の操作案内と安全棚への復帰場面を目視した。ログは `Saved/Logs/GrabAssistAutomation-20261002-rerun.log` と `Saved/Logs/ClimbingTest-{30,60}.log`、画像は `Saved/Screenshots/Climbing/c63fd77aee13484190818c4acd69078f/` にある。自動入力と固定fixtureの証拠であり、物理gamepad・初見プレイ・専用Montage/Root Motionの接触品質は **NOT_RUN**。
