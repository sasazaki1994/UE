#include "ClimbingIntegrationTest.h"
#include "PrototypeGameMode.h"
#include "PrototypePlayer.h"
#include "IshibashiriBoss.h"
#include "KakonActor.h"
#include "NushiProgressComponent.h"
#include "NushiEncounterManager.h"
#include "ColossusClimbingComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "ControlRigComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

AClimbingIntegrationTest::AClimbingIntegrationTest()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}
void AClimbingIntegrationTest::BeginPlay()
{
    Super::BeginPlay();
    FParse::Value(FCommandLine::Get(),TEXT("PrototypeTestRun="),RunId);
    int32 FPS=60; FParse::Value(FCommandLine::Get(),TEXT("PrototypeTestFPS="),FPS);
    FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0/FMath::Clamp(FPS,15,240));
    bCapture=FParse::Param(FCommandLine::Get(),TEXT("PrototypeCapture"));
    bGamepad=FParse::Param(FCommandLine::Get(),TEXT("ClimbingGamepad"));
    bClimbingIK=FParse::Param(FCommandLine::Get(),TEXT("ClimbingIKTest"));
    bGrabMotionWarp=FParse::Param(FCommandLine::Get(),TEXT("GrabMotionWarpTest"));
    bCampaignE2E=FParse::Param(FCommandLine::Get(),TEXT("CampaignE2E"));
    bGamepad|=FParse::Param(FCommandLine::Get(),TEXT("CampaignGamepad"));
    Mode=GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (!Check(Mode && Mode->GetPlayer() && Mode->GetBoss(),TEXT("Encounter spawned"))) return;
    AddTickPrerequisiteComponent(Mode->GetPlayer()->GetClimbing());
    if (bGrabMotionWarp)
    {
        Mode->RetryEncounter();
        GrabWarpPhase = -1;
        DodgeSide = EKeys::A;
    }
    else SetupGrab();
    UE_LOG(LogTemp,Display,TEXT("CLIMB_TEST_BEGIN %s"),*RunId);
}
void AClimbingIntegrationTest::Hold(const FKey& Key,bool Down)
{
    if (Held.Contains(Key)==Down) return;
    APlayerController* PC=Cast<APlayerController>(Mode->GetPlayer()->GetController());
    FKey Mapped = Key;
    const bool Axis = bGamepad && (Key == EKeys::W || Key == EKeys::S || Key == EKeys::A || Key == EKeys::D);
    if (bGamepad)
    {
        if (Key == EKeys::E) Mapped = EKeys::Gamepad_RightShoulder;
        else if (Key == EKeys::SpaceBar) Mapped = EKeys::Gamepad_FaceButton_Bottom;
        else if (Key == EKeys::LeftMouseButton) Mapped = EKeys::Gamepad_FaceButton_Left;
        else if (Key == EKeys::R) Mapped = EKeys::Gamepad_FaceButton_Top;
        else if (Key == EKeys::LeftShift) Mapped = EKeys::Gamepad_FaceButton_Right;
        else if (Key == EKeys::Q) Mapped = EKeys::Gamepad_LeftTrigger;
        else if (Key == EKeys::F) Mapped = EKeys::Gamepad_LeftShoulder;
        else if (Key == EKeys::W || Key == EKeys::S) Mapped = EKeys::Gamepad_LeftY;
        else if (Key == EKeys::A || Key == EKeys::D) Mapped = EKeys::Gamepad_LeftX;
    }
    const float Value = Down ? (Axis && (Key == EKeys::S || Key == EKeys::A) ? -1.f : 1.f) : 0.f;
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(Mapped,Axis?IE_Axis:Down?IE_Pressed:IE_Released,Value));
    if (Down) Held.Add(Key); else Held.Remove(Key);
}
void AClimbingIntegrationTest::Tap(const FKey& Key) { Hold(Key,true); Release.Add(Key); }
void AClimbingIntegrationTest::Next(int32 Value) { Phase=Value;Time=0.f; }
bool AClimbingIntegrationTest::Check(bool Condition,const TCHAR* Description)
{
    if (!Condition)
    {
        UE_LOG(LogTemp,Error,TEXT("CLIMB_TEST_FAIL %s phase=%d: %s"),*RunId,Phase,Description);
        bFinished=true; FPlatformMisc::RequestExitWithStatus(false,1);
    }
    else UE_LOG(LogTemp,Display,TEXT("CLIMB_CHECK %s: %s"),*RunId,Description);
    return Condition;
}
float AClimbingIntegrationTest::AimAt(const FVector& Position)
{
    APrototypePlayer* P=Mode->GetPlayer();
    const float DesiredYaw=(Position-P->GetActorLocation()).Rotation().Yaw;
    const float Error=FMath::FindDeltaAngleDegrees(P->GetController()->GetControlRotation().Yaw,DesiredYaw);
    Cast<APlayerController>(P->GetController())->InputKey(FInputKeyEventArgs::CreateSimulated(
        bGamepad?EKeys::Gamepad_RightX:EKeys::MouseX,IE_Axis,
        // Right-stick dead zone is 0.2; /45 would stop near 9 degrees,
        // outside the recovery counter's 8-degree aim requirement.
        bGamepad?FMath::Clamp(Error/20.f,-1.f,1.f):FMath::Clamp(Error*5.f,-300.f,300.f)));
    return FMath::Abs(Error);
}
void AClimbingIntegrationTest::WalkTo(const FVector& Position,float StopDistance)
{
    const float Error=AimAt(Position);
    Hold(EKeys::W,Error<15.f&&FVector::Dist2D(Mode->GetPlayer()->GetActorLocation(),Position)>StopDistance);
}
bool AClimbingIntegrationTest::DriveMountWindow(float Dt)
{
    APrototypePlayer* P=Mode->GetPlayer(); AIshibashiriBoss* B=Mode->GetBoss();
    AttackWait=FMath::Max(0.f,AttackWait-Dt);
    if (P->GetHealth()!=P->MaxHealth || !Mode->IsEncounterActive())
    {
        Check(false,TEXT("Ground dodge and counter route preserves health and encounter"));
        return false;
    }
    const EIshibashiriState State=B->GetState();
    if (State==EIshibashiriState::Kneel)
    {
        Hold(EKeys::W,false);Hold(EKeys::A,false);Hold(EKeys::D,false);
        return Check(B->GetPosture()==0,TEXT("Input sword counters open the Kneel mount window"));
    }
    if (LastBossState!=static_cast<int32>(State))
    {
        CombatStateTime=0.f;
        if (State==EIshibashiriState::Telegraph)
        {
            const FVector Right=FRotator(0.f,P->GetController()->GetControlRotation().Yaw,0.f).RotateVector(FVector::RightVector);
            DodgeSide=FVector::DotProduct(Right,-P->GetActorLocation())>=0.f?EKeys::D:EKeys::A;
        }
        if (State==EIshibashiriState::Charge) bDodgedThisCharge=false;
        LastBossState=static_cast<int32>(State);
    }
    CombatStateTime+=Dt;
    if (State==EIshibashiriState::Chase || (State==EIshibashiriState::Recover && !B->CanBeCountered()))
    {
        Hold(EKeys::A,false);Hold(EKeys::D,false);
        const FVector Bait=B->GetActorLocation()+(-B->GetActorLocation()).GetSafeNormal2D()*850.f;
        WalkTo(Bait,70.f);
    }
    else if (State==EIshibashiriState::Telegraph)
    {
        Hold(EKeys::W,false);
        AimAt(B->GetActorLocation());
        Hold(DodgeSide,B->GetStateTimeRemaining()<.1f);
    }
    else if (State==EIshibashiriState::Charge)
    {
        Hold(EKeys::W,false);
        if (CombatStateTime<.42f)
        {
            Hold(DodgeSide,true);
            if (CombatStateTime>=.1f && !bDodgedThisCharge)
            {
                Tap(EKeys::LeftShift);
                bDodgedThisCharge=true;
            }
        }
        else
        {
            Hold(DodgeSide,false);
            if (FVector::DotProduct(P->GetActorLocation()-B->GetActorLocation(),B->GetChargeDirection())< -250.f)
                WalkTo(B->GetActorLocation(),300.f);
            else AimAt(B->GetActorLocation());
        }
    }
    else if (State==EIshibashiriState::Recover)
    {
        Hold(DodgeSide,false);
        const float Error=AimAt(B->GetActorLocation());
        const float Distance=FVector::Dist2D(P->GetActorLocation(),B->GetActorLocation());
        Hold(EKeys::W,Error<15.f&&Distance>300.f);
        if (Distance<350.f && Error<8.f && AttackWait<=0.f)
        {
            Tap(EKeys::LeftMouseButton);
            AttackWait=.5f;
        }
    }
    return false;
}
void AClimbingIntegrationTest::SetupGrab(bool bResetEncounter,int32 ResumePhase)
{
    for (const FKey& Key: Held.Array()) Hold(Key,false);
    Release.Empty();
    if (bResetEncounter) Mode->RetryEncounter();
    Mode->GetBoss()->SetActorTickEnabled(true);
    ResumePhaseAfterMount=ResumePhase;
    LastBossState=INDEX_NONE;
    CombatStateTime=AttackWait=0.f;
    DodgeSide=EKeys::A;
    bDodgedThisCharge=false;
    bReachedForelegApproach=false;
    Next(-1);
}
void AClimbingIntegrationTest::Shot(const TCHAR* Name)
{
    if (!bCapture) return;
    FString Dir = bGrabMotionWarp
        ? FPaths::ProjectSavedDir()/TEXT("Screenshots/GrabMotionWarp")/RunId
        : bClimbingIK
        ? FPaths::ProjectSavedDir()/TEXT("Screenshots/ClimbingIK")/RunId/TEXT("After")
        : FPaths::ProjectSavedDir()/TEXT("Screenshots/Climbing")/RunId;
    IFileManager::Get().MakeDirectory(*Dir,true);
    FScreenshotRequest::RequestScreenshot(Dir/(FString(Name)+TEXT(".png")),false,false);
}

void AClimbingIntegrationTest::TickGrabMotionWarp(float Dt)
{
    APrototypePlayer* P=Mode->GetPlayer(); AIshibashiriBoss* B=Mode->GetBoss(); UColossusClimbingComponent* C=P->GetClimbing();
    const FVector Entry=B->GetClimbPosition(0);
    const float Distance=FVector::Dist(P->GetActorLocation(),Entry);
    switch (GrabWarpPhase)
    {
    case 0:
    {
        FVector ToEntry=Entry-P->GetActorLocation(); ToEntry.Z=0.f;
        P->GetController()->SetControlRotation(ToEntry.Rotation());
        if (Distance>C->GrabRange-20.f) { Hold(EKeys::W,true); return; }
        Hold(EKeys::W,false);
        P->SetActorRotation((B->GetActorLocation()-Entry).GetSafeNormal2D().Rotation());
        if (B->GetState()==EIshibashiriState::Charge) return;
        Shot(TEXT("01-BeforeGrab")); Tap(EKeys::E); GrabWarpPhase=1; Time=0.f; return;
    }
    case 1:
        if (!C->IsGrabWarping()) { Check(false,TEXT("Motion Warp starts from normal Grab input (asset contract required)")); return; }
        Shot(TEXT("02-WarpStart")); GrabWarpPhase=2; Time=0.f; return;
    case 2:
        if (Time>.22f) { Shot(TEXT("03-Approach")); GrabWarpPhase=3; } return;
    case 3:
        if (Time>.48f) { Shot(TEXT("04-BeforeContact")); GrabWarpPhase=4; } return;
    case 4:
        if (C->IsGrabWarping()) return;
        if (!Check(C->IsClimbing() && C->GetNode()==0,TEXT("Warp hands off to Route Climbing node 0"))) return;
        Shot(TEXT("05-Attached")); Hold(EKeys::E,true); Hold(EKeys::W,true); GrabWarpPhase=5; Time=0.f; return;
    case 5:
        if (C->IsMoving() || C->GetNode()>0)
        {
            Shot(TEXT("06-ClimbStart")); Hold(EKeys::W,false); Hold(EKeys::E,false);
            UE_LOG(LogTemp,Display,TEXT("GRAB_MOTION_WARP_TEST_PASS %s %.2fs"),*RunId,Total);
            bFinished=true; FApp::SetUseFixedTimeStep(false); FPlatformMisc::RequestExitWithStatus(false,0);
        }
        return;
    }
}
void AClimbingIntegrationTest::Tick(float Dt)
{
    Super::Tick(Dt); if (bFinished || !Mode) return;
    for (const FKey& Key:Release) Hold(Key,false); Release.Empty();
    if (bGamepad)
    {
        // Analog axis events must be sent every frame, including zero after release/retry.
        APlayerController* PC=Cast<APlayerController>(Mode->GetPlayer()->GetController());
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftY,IE_Axis,
            float(Held.Contains(EKeys::W))-float(Held.Contains(EKeys::S))));
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftX,IE_Axis,
            float(Held.Contains(EKeys::D))-float(Held.Contains(EKeys::A))));
    }
    Time+=Dt;Total+=Dt;
    if (bGrabMotionWarp)
    {
        if (Total>45.f) { Check(false,TEXT("Grab Motion Warp validation timeout")); return; }
        if (GrabWarpPhase<0)
        {
            if (!DriveMountWindow(Dt)) return;
            GrabWarpPhase=0;
            Time=0.f;
        }
        TickGrabMotionWarp(Dt); return;
    }
    auto P=Mode->GetPlayer();auto B=Mode->GetBoss();auto C=P->GetClimbing();
    if(bCampaignE2E){bSawCampaignCharge|=B->GetState()==EIshibashiriState::Charge;bSawCampaignDodge|=P->IsDodging();}
    // Both Sense holds must pass through the real input bindings at least once
    // so the review gate's senseReset evidence exists in every climbing run.
    bSawBoundarySense|=P->GetSense()->IsBoundarySenseActive();
    bSawCorruptionSense|=P->GetSense()->IsCorruptionSenseActive();
    if (bClimbingIK && !bIKShakeShot && B->IsBucking() && C->IsClimbing() && C->IsGripping())
    {
        bIKShakeShot=true; Shot(TEXT("07-ShakeCling"));
    }
    if (Total>300.f) { Check(false,TEXT("Integration test timeout")); return; }
    switch (Phase)
    {
    case -1:
    {
        if (!DriveMountWindow(Dt)) break;
        if (bCampaignE2E) { Next(-2); break; }
        // The standalone route test isolates locomotion and climbing after the
        // input-driven posture break. Keep Kneel open while it exercises both.
        B->SetActorTickEnabled(false);
        FVector Entry=B->GetClimbPosition(0); Entry.Z=92.f;
        const FVector Outward=(Entry-B->GetActorLocation()).GetSafeNormal2D();
        // Phase 0 walks toward the foreleg before E. Later regrabs press E
        // directly, so start those inside the three-dimensional GrabRange.
        P->SetActorLocation(Entry+Outward*(ResumePhaseAfterMount==0?210.f:120.f));
        P->GetController()->SetControlRotation(FRotator(-12.f,(-Outward).Rotation().Yaw,0.f));
        if (ResumePhaseAfterMount!=0) P->SetActorRotation((-Outward).Rotation());
        Next(ResumePhaseAfterMount==0?0:-3);
        break;
    }
    case -2:
    {
        const FVector Entry=B->GetClimbPosition(0);
        if (!B->CanMount()) { Check(false,TEXT("Kneel mount window stays open during the ground approach")); break; }
        // Walk to the same reachable outer foreleg position as the standalone
        // fixture, then face the entry before pressing Grab.
        const FVector Outward=(Entry-B->GetActorLocation()).GetSafeNormal2D();
        const FVector Approach=Entry+Outward*190.f;
        if (!bReachedForelegApproach)
        {
            if (FVector::Dist2D(P->GetActorLocation(),Approach)>25.f) WalkTo(Approach,25.f);
            else { Hold(EKeys::W,false); bReachedForelegApproach=true; }
            break;
        }
        // Camera yaw alone does not turn the Character's actor while idle.
        // Walking inward aligns the actor with the motion-warp facing gate.
        const float Error=AimAt(Entry);
        const float Distance=FVector::Dist2D(P->GetActorLocation(),Entry);
        const FVector Facing=(B->GetActorLocation()-Entry).GetSafeNormal2D();
        const float Angle=FMath::Abs(FMath::FindDeltaAngleDegrees(P->GetActorRotation().Yaw,Facing.Rotation().Yaw));
        Hold(EKeys::W,Error<8.f && (Distance>150.f || (Angle>=C->MaximumWarpAngle && Distance>80.f)));
        if(Distance<=150.f && Error<8.f && Angle<C->MaximumWarpAngle
            && !P->IsAttacking() && !P->IsDodging())
        {
            if (!Check(FVector::Dist(P->GetActorLocation(),Entry)<C->GrabRange,
                TEXT("Campaign foreleg approach reaches the grab range"))) return;
            Hold(EKeys::W,false);Shot(TEXT("01-Ground"));Tap(EKeys::Q);Tap(EKeys::F);Tap(EKeys::E);Next(2);
        }
        break;
    }
    case -3:
        if (Time>.3f && !P->IsAttacking() && !P->IsDodging())
        {
            if (!Check(FVector::Dist(P->GetActorLocation(),B->GetClimbPosition(0))<C->GrabRange,
                TEXT("Regrab fixture is within the foreleg grab range"))) return;
            Tap(EKeys::E);
            Next(ResumePhaseAfterMount);
        }
        break;
    case 0:
        if (Time>.3f && !P->IsAttacking())
        {
            if (!Check(P->GetMesh()->GetNumBones()>=19 && B->GetVisualMesh()->GetNumBones()>=20,TEXT("Both deformation skeletons loaded"))) return;
            if (bClimbingIK && !Check(P->GetClimbingControlRig()->GetControlRig()!=nullptr,TEXT("Climbing Control Rig asset is instantiated"))) return;
            if (!Check(P->GetMesh()->GetSingleNodeInstance() && P->GetMesh()->GetSingleNodeInstance()->GetCurrentAsset(),TEXT("Player animation is playing"))) return;
            BeforeFoot=P->GetMesh()->GetBoneLocation(TEXT("foot_L"),EBoneSpaces::ComponentSpace);
            BeforeWalk=P->GetActorLocation();
            MaxFootPoseDelta=0.f;
            Shot(TEXT("01-Ground"));Tap(EKeys::Q);Tap(EKeys::F);Hold(EKeys::W,true);Next(1);
        } break;
    case 1:
        if (!P->IsAttacking())
            MaxFootPoseDelta=FMath::Max(MaxFootPoseDelta,FVector::Dist(
                P->GetMesh()->GetBoneLocation(TEXT("foot_L"),EBoneSpaces::ComponentSpace),BeforeFoot));
        if (Time>.35f)
        {
            Hold(EKeys::W,false);
            if (!Check(FVector::Dist2D(P->GetActorLocation(),BeforeWalk)>20.f && MaxFootPoseDelta>.05f,
                TEXT("Locomotion moves the player and changes the foot bone pose"))) return;
            Tap(EKeys::E);Next(2);
        } break;
    case 2:
        if (Time>.12f)
        {
            if (!Check(C->IsClimbing() && C->GetNode()==0,TEXT("E mounts from ground after movement input"))) return;
            if (bClimbingIK) Shot(TEXT("01-Grab"));
            BeforeBoss=B->GetActorLocation(); B->SetActorTickEnabled(true);
            Hold(EKeys::E,true);Hold(EKeys::W,true);Next(3);
        } break;
    case 3:
        if (bClimbingIK && C->IsMoving() && C->GetIKWeight()>.5f && IKContactShot<3
            && C->GetNode()==IKContactShot)
        {
            const TCHAR* Names[]={TEXT("02-ForelegClimb"),TEXT("03-HandsContact"),TEXT("04-FeetContact")};
            Shot(Names[IKContactShot++]);
            if (!Check(C->IsIKVerticalSlice(),TEXT("FBIK remains scoped to nodes 0-3"))) return;
            const FClimbingIKTargets& T=C->GetIKTargets();
            const FName Bones[]={TEXT("hand_L"),TEXT("hand_R"),TEXT("foot_L"),TEXT("foot_R")};
            const FTransform Targets[]={T.LeftHand,T.RightHand,T.LeftFoot,T.RightFoot};
            for (int32 I=0;I<4;++I)
                UE_LOG(LogTemp,Display,TEXT("CLIMBING_IK_ERROR_CM %s %.2f"),*Bones[I].ToString(),
                    FVector::Distance(P->GetMesh()->GetBoneLocation(Bones[I]),Targets[I].GetLocation()));
        }
        if (C->GetNode()==3 && !C->IsMoving())
        {
            Hold(EKeys::W,false);BeforeStamina=C->GetStamina();Shot(bClimbingIK ? TEXT("05-FirstShoulder") : TEXT("02-FirstLedge"));Next(4);
        } break;
    case 4:
        if (Time>1.f)
        {
            if (!Check(C->GetStamina()>BeforeStamina,TEXT("Safe ledge restores stamina"))) return;
            if (!Check(FVector::Dist(BeforeBoss,B->GetActorLocation())>50 && FVector::Dist(P->GetActorLocation(),B->GetClimbPosition(3))<3,TEXT("Rider follows moving creature"))) return;
            if (bClimbingIK) Shot(TEXT("06-BossMoving"));
            B->AddActorWorldRotation(FRotator(0,35,0));Next(5);
        } break;
    case 5:
        if (Time>.1f)
        {
            if (!Check(FVector::Dist(P->GetActorLocation(),B->GetClimbPosition(3))<3,TEXT("Rider follows creature rotation"))) return;
            Hold(EKeys::W,true);Next(6);
        } break;
    case 6:
        if (C->GetNode()==5 && !C->IsMoving()) { Hold(EKeys::W,false);Hold(EKeys::D,true);Next(7); } break;
    case 7:
        if (C->GetNode()==10 && !C->IsMoving()) { Hold(EKeys::D,false);Hold(EKeys::W,true);Next(8); } break;
    case 8:
        if (C->GetNode()==9 && !C->IsMoving()) { Hold(EKeys::W,false);Next(9); } break;
    case 9:
        if (!B->IsBucking() && Time>.6f) { Tap(EKeys::LeftMouseButton);Shot(TEXT("03-ShoulderCore"));Next(10); } break;
    case 10:
        if (Time>.6f)
        {
            if (!Check(B->GetPurifiedCount()==1,TEXT("Branch leads to first purifiable core"))) return;
            if (!Check(B->GetNushiProgressComponent()->GetRegisteredKakonCount()==3
                && B->GetNushiProgressComponent()->GetPurifiedCount()==1
                && B->GetNushiState()==ENushiState::Active
                && Mode->GetEncounterManager()->GetEncounterState()==ENushiEncounterState::Running,
                TEXT("Normal gameplay uses shared progress 1/3 and Running lifecycle"))) return;
            Tap(EKeys::LeftMouseButton);Next(11);
        } break;
    case 11:
        if (Time>.6f)
        {
            if (!Check(B->GetPurifiedCount()==1,TEXT("Purified core cannot be damaged twice"))) return;
            Hold(EKeys::S,true);Next(12);
        } break;
    case 12:
        if (C->GetNode()==5 && !C->IsMoving()) { Hold(EKeys::S,false);Hold(EKeys::W,true);Next(13); } break;
    case 13:
        if (C->GetNode()==6 && !C->IsMoving()) { Hold(EKeys::W,false);Next(14); } break;
    case 14:
        if (!B->IsBucking() && Time>.6f) { Tap(EKeys::LeftMouseButton);Shot(TEXT("04-Summit"));Next(15); } break;
    case 15:
        if (Time>.6f)
        {
            if (!Check(B->GetPurifiedCount()==2,TEXT("Summit core purified after braced climbing"))) return;
            Hold(EKeys::W,true);Next(16);
        } break;
    case 16:
        if (C->GetNode()==8 && !C->IsMoving()) { Hold(EKeys::W,false);Next(17); } break;
    case 17:
        if (!B->IsBucking() && Time>.6f) { Tap(EKeys::LeftMouseButton);Next(18); } break;
    case 18:
        if (Time>.6f)
        {
            if (!Check(Mode->GetResult()==EEncounterResult::Victory && B->GetPurifiedCount()==3,TEXT("Three distinct cores finish encounter"))) return;
            if (!Check(B->GetNushiProgressComponent()->IsAllPurified()
                && B->GetNushiState()==ENushiState::Calm
                && Mode->GetEncounterManager()->GetEncounterState()==ENushiEncounterState::Completed,
                TEXT("Three Kakon propagate through Progress, Calm, Completed and Victory"))) return;
            ++CompletedRoutes;
            Shot(TEXT("05-Victory"));
            if(bCampaignE2E&&CompletedRoutes>=2){if(!Check(bSawCampaignCharge&&bSawCampaignDodge,TEXT("Campaign ground approach includes Charge and Dodge input")))return;Hold(EKeys::F,true);UE_LOG(LogTemp,Display,TEXT("CLIMB_TEST_PASS %s routes=2 retry=1 sense=boundary,corruption"),*RunId);bFinished=true;FApp::SetUseFixedTimeStep(false);}
            else {Tap(EKeys::R);++Retries;Next(19);}
        } break;
    case 19:
        if (Time>.3f)
        {
            if (!Check(Mode->IsEncounterActive() && !C->IsClimbing() && C->GetStamina()==100 && B->GetPurifiedCount()==0,TEXT("R resets victory, attachment, stamina and cores"))) return;
            if (!Check(B->GetNushiProgressComponent()->GetRegisteredKakonCount()==3
                && !B->GetNushiProgressComponent()->IsAllPurified()
                && B->GetNushiState()==ENushiState::Active
                && Mode->GetEncounterManager()->GetEncounterState()==ENushiEncounterState::Running
                && B->GetCoreKakon(0)->GetState()==EKakonState::Exposed
                && B->GetCoreKakon(1)->GetState()==EKakonState::Exposed
                && B->GetCoreKakon(2)->GetState()==EKakonState::Exposed,
                TEXT("Retry resets all shared Kakon and restarts the common lifecycle"))) return;
            // Re-run the full route after the actual keyboard/gamepad Retry.
            // Do not hide an incomplete reset behind another setup reset.
            if (CompletedRoutes>=2 && !Check(CompletedRoutes==2,TEXT("Full three-core route clears again after input Retry"))) return;
            SetupGrab(false,CompletedRoutes==2?20:0);
        } break;
    case 20:
        if (Time>.15f)
        {
            if (!Check(C->IsClimbing(),TEXT("Can grab again after retry"))) return;
            Tap(EKeys::SpaceBar);Next(21);
        } break;
    case 21:
        if (Time>.15f)
        {
            if (!Check(!C->IsClimbing() && P->GetCharacterMovement()->IsFalling(),TEXT("Space detaches and restores falling movement"))) return;
            SetupGrab(true,22);
        } break;
    case 22:
        if (Time>.15f) { Hold(EKeys::E,true);Next(23); } break;
    case 23:
        if (!C->IsClimbing())
        {
            if (!Check(C->GetStamina()<1.f,TEXT("Exhausted stamina forces detachment"))) return;
            SetupGrab(true,24);
        } break;
    case 24:
        if (Time>.15f) { B->SetActorTickEnabled(true);Hold(EKeys::W,true);Next(25); } break;
    case 25:
        if (C->GetNode()==3 && !C->IsMoving()) { Hold(EKeys::W,false);Hold(EKeys::E,false);Next(26); } break;
    case 26:
        if (!C->IsClimbing())
        {
            if (!Check(B->IsBucking() && C->GetStamina()>0,TEXT("Unbraced shaking throws rider off before exhaustion"))) return;
            if (!bClimbingIK) Shot(TEXT("06-ThrownOff"));
            Next(27);
        } break;
    case 27:
        if (Time>.3f)
        {
            if (!Check(bSawBoundarySense && bSawCorruptionSense,TEXT("Q and F Sense holds were exercised through input bindings"))) return;
            UE_LOG(LogTemp,Display,TEXT("%s %s %.2fs routes=%d retry=%d sense=boundary,corruption"),
                bClimbingIK ? TEXT("CLIMBING_IK_TEST_PASS") : TEXT("CLIMB_TEST_PASS"),*RunId,Total,CompletedRoutes,Retries);
            bFinished=true;FApp::SetUseFixedTimeStep(false);FPlatformMisc::RequestExitWithStatus(false,0);
        } break;
    }
}
