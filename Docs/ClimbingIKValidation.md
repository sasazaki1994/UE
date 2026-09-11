# 白面 Climbing IK Vertical Slice

## Scope / acceptance

Priority 2 は既存 Climb / Hang / Grip animation と11点 Route Climbing を Base Pose / body placement として維持し、前脚の node 0 から最初の肩岩（休息点 node 3）だけを補正する。受け入れ仕様は `Specs/Acceptance/ClimbingIK.feature`。自由登攀、trace による経路置換、残り8点、描画profileには変更を加えていない。

## Plugin / module

UE 5.6標準の `ControlRig` と `FullBodyIK` pluginだけを有効化した。Runtime C++が `UControlRigComponent` を生成してcontrol値を渡すため Build.cs には `ControlRig` moduleだけが必要である。FBIK solverはControl Rig asset内のunitとしてFullBodyIK pluginからロードされ、ゲームmoduleはsolver C++ APIを直接includeしないため `FullBodyIK` moduleを推測で追加していない。外部pluginはない。

## Skeleton / Base Pose

`Tools/RigCharacterModels.py` の生成元を確認した実名であり推測名ではない。階層は `root -> pelvis -> spine -> chest`、腕は `chest -> upperarm_L/R -> forearm_L/R -> hand_L/R`、脚は `pelvis -> thigh_L/R -> shin_L/R -> foot_L/R`。Climb animation (`AN_Shirotsura_Climb`) は置換せず、従来のSingle Node再生をBase Poseとする。

Control Rig asset `/Game/Characters/Rigged/Shirotsura/CR_Shirotsura_Climbing` は、このSkeletonから作成し、`IK_Hand_L`, `IK_Hand_R`, `IK_Foot_L`, `IK_Foot_R` (Transform) と `IK_Weight` (Float) を公開する。FBIK root=`pelvis`、effectors=4本、spine/chestをbody chainに含める。UE Editor上で推奨する初期値は Position Alpha=`IK_Weight`、Rotation Alpha=0.35×weight、Pull Chain Alpha=0.65、pelvis stiffness=0.75、spine/chest stiffness=0.82、arm/leg stiffness=0.55。thigh/upperarmのpreferred angleを既存Climb pose方向へ設定し、肘・膝hingeは±5度の横ぶれと0〜150度の屈曲、pelvis twistは±35度に制限する。これらはWindows UE 5.6で見た目を確認して確定する暫定値である。

> このLinux checkoutにはUE Editorも生成済み `.uasset` もない。UE Python APIは5.6でもRigVM graph / pinの安定した公開生成APIを保証しないため、動作未確認の生成scriptを追加して成功を装っていない。上記assetをUE 5.6 Editorで作成・保存するまではControl Rig componentはclass未設定で安全にno-opとなる。従ってFBIK Runtimeは **UNVERIFIED** である。

## Effectors / moving boss

4 targetはroute pointを変更せず、現在nodeとdestinationの補間anchorへ左右・上下offsetを加える。offsetは石走りの`RiggedIshibashiri` component local座標で保持し、毎frameそのcomponent transformでWorld transformへ変換する。このためActorの歩行・旋回だけでなくCreature componentのShake回転にも追従する。World固定targetではない。短距離traceはrouteを不安定にし得るため、このsliceでは採用しない。

## Weight / reset

node 0〜3（destinationも3以下）だけが対象。Grab後は0から0.22秒で1へ、Climb中は1、node 3 Restは0.72、それ以外・Jump Off・Fallは0.16秒で0へ補間する。Shake中はCling成功なら対象範囲のweightを維持し、失敗してDetachするとblend outする。Retryは`Reset()`でweight=0、4 targetをidentityへ即時クリアして次encounterへの残留を防ぐ。

## Debug / numerical evaluation

Development/Testで `-ClimbingIKDebug` を指定すると4 targetにsphere、実boneからtargetへlineを描画する。誤差8cm以下=緑、20cm以下=黄、それ以上=赤。`#if !UE_BUILD_SHIPPING`なのでShippingには含まれない。Windows validationでは各線の距離をログへ追加計測し、通常接触で20cm超が連続しないことを目標にする。現環境ではTarget/Bone誤差は **UNVERIFIED / 未計測**。

## Validation commands (Windows UE 5.6)

```powershell
.\Tools\Prototype.ps1 -Action Build
.\Tools\Prototype.ps1 -Action Test -Camera -Capture -Basin -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Climbing -Capture -Basin -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Climbing -Basin -SkipBuild -TestFPS 30
.\Tools\Prototype.ps1 -Action Test -ClimbingGamepad -Basin -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -Grab -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -BasinScenario -Basin -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -ClimbingIK -Capture -Basin -SkipBuild -TestFPS 60
.\Tools\Prototype.ps1 -Action Test -ClimbingIK -Capture -Basin -HighQuality -SkipBuild -TestFPS 60
```

ClimbingIKは通常Gameplay入力でGrabから全route regressionまで進み、route攻略中のteleportを使わない。After captureは `Saved/Screenshots/ClimbingIK/<RunId>/After/` のGrab、前脚、手接触、足接触、肩岩、Boss移動、Shake/Clingの7枚。BeforeはIK assetを一時的に外して同じ固定FPS/runを行い `Before/` に保存する（自動testは比較対象を上書きしないためAfterのみ生成）。LegacyとHigh Qualityの両方を実行する。

## Current results / known issues

Python static testsとJSON validation以外は、このLinux環境にWindows UE 5.6 / MSVC / RHIがないため **UNVERIFIED**。Build、Control Rig asset生成、FBIK runtime、各regression、30/60 FPS、Legacy/High Quality、screenshots、cm誤差、肘・膝・pelvisの見た目を確認済みとは扱わない。特にasset未作成のcheckoutではGameplayは維持されるが見た目はまだ改善しない。

11点全体へは今回のoffsetを複製せず、各route edge単位で4つのlocal contactをauthoringし、到達誤差とjoint limitを区間ごとに検証する。まず本sliceのWindows比較で浮き・貫通・反転が改善し、Gameplay/FPS回帰がないと確認できた場合だけ段階展開する。
