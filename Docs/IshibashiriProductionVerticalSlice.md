# 石走り Production Vertical Slice（2026-09-24）

Status: **SOURCE IMPLEMENTED / UE RUNTIME NOT_RUN**

## 調査と守る契約

石走りの既存loopは、突進を横回避し、無傷のRecover中だけ禍の隆起へ反撃して3段階の体勢を崩し、Kneel中に前脚のroute node 0へ取り付き、11点routeを登る。ShakeではGrabを保持してStaminaを管理し、休息nodeで回復しながら3つの禍根を順に浄化する。3/3は`NushiProgressComponent`から既存のCalm、Encounter Completed、Victoryへ到達し、Retryが全状態とSenseをresetする。HPやDeathは主に存在せず、このauthority、route座標、入力、各timing、浄化数、Senseのread-only責務は変更していない。

通常HUDはStamina/Kakon、押下中のSense、短いcontext feedbackを中心とし、Player HP、Posture、内部state/timer、操作一覧は`-DebugGuidance`だけに残る。登攀中のみCling/浄化のcontext hintを表示する既存方針も維持した。

## 実装

### 取り付き

専用Montage/AnimBPはcheckoutに存在しない。従って生成済みとは扱わず、既存Climb clipとC++ fallbackだけを改善した。入力後、開始transformを石走りrelativeで記録し、約0.48秒の間も移動するtargetと一緒に運ぶ。身体の向きを先に揃えて「手を伸ばす」読みを作り、位置は遅れて前脚へ収束する。許容誤差を通過して初めてGrab成立とClimbingへのhandoffを行うため、終端snapで成功を偽装しない。Grab開始時は88度へ穏やかに寄り、成立後は100度へ開く。

### 接触と動く地形

`CR_Shirotsura_Climbing.uasset`は存在しない。Control Rig componentと4 effectorのsource hookは存在するが、現状は安全なno-opである。今回target供給をnode 0〜3限定から全11 node/edgeへ広げ、前脚、肩、背、休息点、Shake中のClingまでCreature component frameで更新する。石走りの歩行・旋回・Buck rotationへ追従するが、共通offsetは皮膚surfaceの実測値ではない。Control Rig作成後はEditorで各edgeの手足contact、joint limit、20 cm超の連続誤差をcapture比較する必要がある。

Climbing cameraはplayerを追う従来の安全sweepを保ちつつ、位置に低周波補間を入れた。巨大な並進は残し、animation由来の細かい振動だけを減らす。Shake時は画角を最大3度だけ広げ、回転camera shakeは追加していない。

### 浄化とCalm

既存の禍根はHP破壊ではなく`Exposed -> Purified`である。通常表示は現在対象だけが暗い赤黒でpulseし、浄化時は既存0.35秒の収束tailとcore bone消灯を使う。3/3は既存`AN_Ishibashiri_Calmed`とBasinの霧/照明緩和へ進む。爆発、死、消滅、HP、ragdollは追加していない。Gameplay authorityを遅延させないため、Encounter Completed/Victory前の新しい待機timerも追加していない。

## Editor作業（assetを作成する場合）

1. `SK_Shirotsura`から`CR_Shirotsura_Climbing`を作り、既存sourceが要求する`IK_Hand_L/R`、`IK_Foot_L/R`、`IK_Weight`を公開する。
2. 既存Climb/Hang/GripをBase Poseとし、pelvisをFBIK root、実在する`hand_L/R`と`foot_L/R`をeffectorにする。まず前脚→肩、肩→背、各休息点、Buck/Gripの順で調整する。
3. `-ClimbingIKDebug`でtarget-to-bone線を確認し、各edgeで浮きと貫通を同時に比較する。routeやGameplay判定をIKへ移さない。
4. 専用Grab animationを制作する場合だけRoot Motion MontageとAnimBPを割り当て、`IshibashiriGrab` Motion Warp notifyを接近部分へ置く。既存clipを専用clipとして改名しない。
5. 取り付き前、接近、接触、前脚、肩、背、休息、Shake/Cling、各禍根、3/3、CalmをLegacy/HighQualityの同一cameraでcaptureする。

## 未検証と不足

Linux環境にはWindows、UE 5.6.1 Editor、MSVC、描画RHIがないため、Build、UHT、Runtime、30/60固定FPS gate、実描画性能、camera酔い、collision、IK誤差、captureはすべて`NOT_RUN`である。固定FPS testが将来PASSしても、GPU/frame-timeを測る実描画性能PASSとは区別する。

最大の不足は`CR_Shirotsura_Climbing`と専用Grab Montage/AnimBPが未制作なことである。source fallbackは連続性を改善するが、指先・足裏をmesh surfaceへ保証しない。次に着手すべき1作業は、**UE 5.6.1 EditorでControl Rigを作成し、全route edgeの接触誤差をcapture付きで調整・検証すること**である。
