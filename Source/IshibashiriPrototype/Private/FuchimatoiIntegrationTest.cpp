#include "FuchimatoiIntegrationTest.h"
#include "FuchimatoiGameMode.h"
#include "FuchimatoiBoss.h"
#include "FuchimatoiPlayer.h"
#include "FuchimatoiArena.h"
#include "FuchimatoiRouteAnchor.h"
#include "NushiEncounterManager.h"
#include "NushiProgressComponent.h"
#include "KakonActor.h"
#include "StaminaComponent.h"
#include "GrabComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraTypes.h"
#include "InputKeyEventArgs.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformTime.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"

namespace { int32 FuchimatoiIntegrationTestCountActors(UWorld* World) { int32 Count=0; for(TActorIterator<AActor> It(World);It;++It) ++Count; return Count; } }
AFuchimatoiIntegrationTest::AFuchimatoiIntegrationTest()
{
    PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.TickGroup=TG_PostUpdateWork;
}
void AFuchimatoiIntegrationTest::BeginPlay()
{
    Super::BeginPlay();
    FParse::Value(FCommandLine::Get(),TEXT("PrototypeTestRun="),RunId);
    FParse::Value(FCommandLine::Get(),TEXT("PrototypeTestFPS="),TargetFPS);
    bRealtime=FParse::Param(FCommandLine::Get(),TEXT("FuchimatoiRealtime"));
    FApp::SetUseFixedTimeStep(!bRealtime);
    if (bRealtime) GEngine->SetMaxFPS(TargetFPS);
    else FApp::SetFixedDeltaTime(1.0/FMath::Clamp(TargetFPS,15,240));
    PreviousFrameTime=PerformanceStart=FPlatformTime::Seconds();
    bCapture=FParse::Param(FCommandLine::Get(),TEXT("PrototypeCapture"));
    bGamepad=FParse::Param(FCommandLine::Get(),TEXT("FuchimatoiGamepad"));
    bRecovery=FParse::Param(FCommandLine::Get(),TEXT("FuchimatoiRecovery"));
    if(FParse::Param(FCommandLine::Get(),TEXT("CampaignE2E"))){bRecovery=true;bGamepad|=FParse::Param(FCommandLine::Get(),TEXT("CampaignGamepad"));}
    Mode=GetWorld()->GetAuthGameMode<AFuchimatoiGameMode>();
    if (!Check(Mode && Mode->GetPlayer() && Mode->GetBoss() && Mode->GetEncounterManager(),TEXT("Playable encounter spawned"))) return;
    if (!Check(Mode->IsEncounterActive() && Mode->GetBoss()->GetActionState()==EFuchimatoiActionState::Submerged
        && Mode->GetBoss()->GetNushiProgressComponent()->GetRegisteredKakonCount()==3,TEXT("Start: Submerged, Active, Running, three Kakon"))) return;
    InitialActors=FuchimatoiIntegrationTestCountActors(GetWorld());
    UE_LOG(LogTemp,Display,TEXT("FUCHIMATOI_LENGTH initial_cm=%.2f"),Mode->GetBoss()->GetBodyLength());
    if (!Check(!Mode->GetBoss()->CanRecover(),TEXT("Recovery cannot skip the initial Bite phase"))) return;
    if (bRecovery) Next(30);
}
void AFuchimatoiIntegrationTest::Hold(const FKey& Key,bool Down)
{
    if (Held.Contains(Key)==Down) return;
    FKey Mapped=Key;
    if (bGamepad)
    {
        if(Key==EKeys::E) Mapped=EKeys::Gamepad_RightShoulder;
        if(Key==EKeys::LeftShift) Mapped=EKeys::Gamepad_FaceButton_Right;
        if(Key==EKeys::LeftMouseButton) Mapped=EKeys::Gamepad_FaceButton_Left;
        if(Key==EKeys::R) Mapped=EKeys::Gamepad_FaceButton_Top;
        if(Key==EKeys::SpaceBar) Mapped=EKeys::Gamepad_FaceButton_Bottom;
    }
    Cast<APlayerController>(Mode->GetPlayer()->GetController())->InputKey(FInputKeyEventArgs::CreateSimulated(Mapped,Down?IE_Pressed:IE_Released,Down?1.f:0.f));
    if(Down) Held.Add(Key); else Held.Remove(Key);
}
void AFuchimatoiIntegrationTest::Tap(const FKey& Key) { Hold(Key,true); Releases.Add(Key); }
void AFuchimatoiIntegrationTest::SetMove(float Forward,float Right)
{
    ForwardAxis=Forward; RightAxis=Right;
    if (bGamepad) return;
    Hold(EKeys::W,Forward>.2f); Hold(EKeys::S,Forward<-.2f);
    Hold(EKeys::D,Right>.2f); Hold(EKeys::A,Right<-.2f);
}
void AFuchimatoiIntegrationTest::MoveToward(FVector WorldTarget)
{
    AFuchimatoiPlayer* P=Mode->GetPlayer();
    const FVector Delta=(WorldTarget-P->GetActorLocation()).GetSafeNormal2D();
    const FRotationMatrix Rotation(FRotator(0,P->GetControlRotation().Yaw,0));
    SetMove(FVector::DotProduct(Delta,Rotation.GetUnitAxis(EAxis::X)),FVector::DotProduct(Delta,Rotation.GetUnitAxis(EAxis::Y)));
}
void AFuchimatoiIntegrationTest::Next(int32 NewPhase) { Phase=NewPhase; Time=0; }
bool AFuchimatoiIntegrationTest::Check(bool Condition,const TCHAR* Message)
{
    if (!Condition)
    {
        UE_LOG(LogTemp,Error,TEXT("FUCHIMATOI_TEST_FAIL %s phase=%d time=%.2f: %s"),*RunId,Phase,Total,Message);
        bDone=true; FApp::SetUseFixedTimeStep(false); FPlatformMisc::RequestExitWithStatus(false,1);
    }
    else UE_LOG(LogTemp,Display,TEXT("FUCHIMATOI_CHECK %s: %s"),*RunId,Message);
    return Condition;
}
void AFuchimatoiIntegrationTest::Shot(const TCHAR* Name)
{
    if (!bCapture || Rounds>0) return;
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("Screenshots/Fuchimatoi")/RunId;
    IFileManager::Get().MakeDirectory(*Dir,true);
    const FString Path=Dir/(FString(Name)+TEXT(".png"));
    // Preserve the first 1/3 recovery; later falls must not overwrite its proof.
    if (!IFileManager::Get().FileExists(*Path)) FScreenshotRequest::RequestScreenshot(Path,false,false);
}
bool AFuchimatoiIntegrationTest::CheckCameraOrbit(const TCHAR* Place)
{
    // Orbit sweeps are a separate correctness fixture, not gameplay frame work.
    if (bRealtime) return true;
    AFuchimatoiPlayer* P=Mode->GetPlayer();
    APlayerController* PC=Cast<APlayerController>(P->GetController());
    USpringArmComponent* Arm=P->FindComponentByClass<USpringArmComponent>();
    const FRotator Original=PC->GetControlRotation();
    bool bClear=true;
    for (float Pitch : {-15.f,-35.f,-60.f})
        for (int32 Yaw=0; Yaw<360; Yaw+=15)
        {
            // Camera-only fixture: keep gameplay position, input and progress.
            PC->SetControlRotation(FRotator(Pitch,Yaw,0));
            Arm->TickComponent(1.f/60.f,LEVELTICK_All,nullptr);
            FMinimalViewInfo View; P->CalcCamera(1.f/60.f,View);
            const FVector Focus=P->GetActorLocation()+FVector(0,0,35);
            FHitResult Hit;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(FuchimatoiCameraTest),false,P);
            bClear &= !GetWorld()->LineTraceSingleByChannel(Hit,Focus,View.Location,ECC_Camera,Params);
            bClear &= FVector::Dist(Focus,View.Location)>20.f;
        }
    PC->SetControlRotation(Original); Arm->TickComponent(1.f/60.f,LEVELTICK_All,nullptr);
    return Check(bClear,*FString::Printf(TEXT("Camera sight clear in 72 yaw/pitch samples at %s"),Place));
}
void AFuchimatoiIntegrationTest::Tick(float Dt)
{
    Super::Tick(Dt); if (bDone || !Mode) return;
    if (bRealtime)
    {
        const double Now=FPlatformTime::Seconds();
        if (Now-PerformanceStart>2.0) FrameTimes.Add((Now-PreviousFrameTime)*1000.0);
        PreviousFrameTime=Now;
    }
    for(const FKey& Key:Releases) Hold(Key,false); Releases.Empty();
    AFuchimatoiPlayer* P=Mode->GetPlayer(); AFuchimatoiBoss* B=Mode->GetBoss();
    if(bGamepad)
    {
        APlayerController* PC=Cast<APlayerController>(P->GetController());
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftY,IE_Axis,ForwardAxis));
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftX,IE_Axis,RightAxis));
    }
    Time+=Dt; Total+=Dt; bSawDodge|=P->IsDodging();
    if (Total>360 || Time>35) { Check(false,TEXT("Input playthrough timeout")); return; }
    if (Phase>=1 && Phase<=4 && B->GetActionState()==EFuchimatoiActionState::BiteLunge && !LockedTarget.IsZero())
        if ((!bRealtime || !B->GetBiteTargetLocalLocation().Equals(LockedTarget))
            && !Check(B->GetBiteTargetLocalLocation().Equals(LockedTarget),TEXT("Bite target stays locked during evasion"))) return;
    switch(Phase)
    {
    case 0:
        MoveToward(Mode->GetArena()->GetBaitPosition());
        if (FVector::Dist2D(P->GetActorLocation(),Mode->GetArena()->GetBaitPosition())<20)
        { SetMove(0,0); LockedTarget=FVector::ZeroVector; Next(Rounds>=2 || (bRecovery && !bHitRetryDone)?17:1); }
        break;
    case 1:
        if (B->GetActionState()==EFuchimatoiActionState::BiteWindup && B->GetActionTimeRemaining()<.8f)
        { Shot(TEXT("01-BiteWindup")); Next(2); }
        break;
    case 2:
        if (B->GetActionState()==EFuchimatoiActionState::BiteLunge)
        { LockedTarget=B->GetBiteTargetLocalLocation(); Shot(TEXT("02-BiteLunge")); Next(3); }
        break;
    case 3:
        if (B->GetHeadWorldLocation().X>-450)
        { MoveToward(P->GetActorLocation()+FVector(0,-1000,0)); Next(4); }
        break;
    case 4:
        if(Time>.05f && !bSawDodge) Tap(EKeys::LeftShift);
        if (B->IsSnagged())
        {
            SetMove(0,0);
            if (!Check(bSawDodge && P->GetHealth()==(bRecovery && Rounds==0?2:3) && B->CanMount(),TEXT("Input dodge baits rock collision and opens Snagged grab"))) return;
            Shot(TEXT("03-Snagged")); Next(bRecovery && !bMissedGrabDone?40:5);
        }
        break;
    case 5:
        MoveToward(B->GetRouteAnchor(0)->GetActorLocation());
        if (!P->IsDodging() && FVector::Dist(P->GetActorLocation(),B->GetRouteAnchor(0)->GetActorLocation())<P->GrabRange-35)
        { SetMove(0,0); Tap(EKeys::E); Next(6); }
        break;
    case 6:
        if(Time>.15f)
        {
            if(!Check(P->IsMounted() && P->GetRouteNode()==0 && P->GetGrab()->GetGrabTarget()==B->GetRouteAnchor(0),TEXT("Grab input attaches via shared UGrabComponent"))) return;
            if(!CheckCameraOrbit(TEXT("BaitRock"))) return;
            Shot(TEXT("04-HeadGrab")); SetMove(1,0); Next(7);
        }
        break;
    case 7:
        if(P->GetRouteNode()==3 && !P->IsRouteMoving())
        {
            SetMove(0,0);
            if(!Check(P->GetStamina()->GetCurrentStamina()<95,TEXT("Snake climbing consumes shared stamina"))) return;
            Shot(TEXT("05-Kakon1")); BeforeAnchor=B->GetRouteAnchor(3)->GetActorLocation();
            RockBefore=B->GetRouteAnchor(4)->GetActorLocation(); Tap(EKeys::LeftMouseButton); Next(8);
        }
        break;
    case 8:
        if(Time>.15f && !Check(B->GetNushiProgressComponent()->GetPurifiedCount()==1 && B->IsCoiling(),TEXT("First input purification begins Coiling through existing action API"))) return;
        if (B->GetCoilingProgress()>.3f && !bSawCoilFollow)
        {
            if(!Check(FVector::Dist(P->GetActorLocation(),B->GetRouteAnchor(3)->GetActorLocation())<3,TEXT("Climber follows animated local anchor during Coiling"))) return;
            bSawCoilFollow=true; Shot(TEXT("06-Coiling"));
        }
        if(B->IsCoilingComplete())
        {
            UE_LOG(LogTemp,Display,TEXT("FUCHIMATOI_LENGTH coiled_cm=%.2f"),B->GetBodyLength());
            if(!Check(FVector::Dist(BeforeAnchor,B->GetRouteAnchor(3)->GetActorLocation())>100
                && RockBefore.Equals(B->GetRouteAnchor(4)->GetActorLocation(),.01f),TEXT("Coiling reconfigures snake route while rock anchor stays fixed"))) return;
            if (bRecovery && Rounds==0)
            {
                RecoveryBossPose=B->GetActorTransform(); RecoveryRoutePosition=B->GetRouteAnchor(3)->GetActorLocation();
                Tap(EKeys::SpaceBar); Next(20);
            }
            else { SetMove(1,0); Next(9); }
        }
        break;
    case 9:
        if(P->GetRouteNode()==4 && !P->IsRouteMoving())
        { SetMove(0,0); BeforeStamina=P->GetStamina()->GetCurrentStamina(); Shot(TEXT("07-SnakeToRock")); Next(10); }
        break;
    case 10:
        if(Time>1.2f)
        {
            if(!CheckCameraOrbit(TEXT("RockLedge"))) return;
            if(!Check(P->GetGrab()->GetGrabTarget()==B->GetRouteAnchor(4) && B->GetRouteAnchor(4)->IsRock()
                && P->GetStamina()->GetCurrentStamina()>BeforeStamina,TEXT("Snake to rock transfer changes grab owner and restores stamina"))) return;
            SetMove(1,0); Next(11);
        }
        break;
    case 11:
        if(P->GetRouteNode()==6 && !P->IsRouteMoving()) { SetMove(0,0); Next(12); }
        break;
    case 12:
        if(Time>1.2f) { if(!CheckCameraOrbit(TEXT("RockPillar"))) return; SetMove(1,0); Next(13); }
        break;
    case 13:
        if(P->GetRouteNode()==7 && !P->IsRouteMoving())
        { SetMove(0,0); Shot(TEXT("08-Kakon2")); Tap(EKeys::LeftMouseButton); Next(14); }
        break;
    case 14:
        if(Time>.2f)
        {
            if(!Check(B->GetNushiProgressComponent()->GetPurifiedCount()==2,TEXT("Second serpent Kakon advances shared progress to 2/3"))) return;
            if (bRecovery && Rounds==0 && FallsRecovered==1) { Tap(EKeys::SpaceBar); Next(20); }
            else { SetMove(1,0); Next(15); }
        }
        break;
    case 15:
        if(P->GetRouteNode()==8 && !P->IsRouteMoving()) Shot(TEXT("09-FinalClimb"));
        if(P->GetRouteNode()==9 && !P->IsRouteMoving()) { SetMove(0,0); Tap(EKeys::LeftMouseButton); Next(16); }
        break;
    case 16:
        if(Time>.2f && Time<.3f)
        {
            if(!Check(Mode->IsVictory() && !Mode->IsEncounterActive() && B->GetNushiProgressComponent()->GetPurifiedCount()==3
                && B->GetNushiState()==ENushiState::Calm && Mode->GetEncounterManager()->GetEncounterState()==ENushiEncounterState::Completed,
                TEXT("Third input purification propagates Progress 3/3, Calm, Completed, Victory"))) return;
            if(!Check(!B->CanRecover() && !B->IsRecoveryUnlocked(),TEXT("Completed/Calm disables recovery"))) return;
            if(bRecovery && Rounds==0 && !Check(FallsRecovered==2 && B->GetTelemetry().RecoveryGrabs==2
                && B->GetTelemetry().Falls==2 && bMissCycleDone && bMissedGrabDone && bHitRetryDone,
                TEXT("Two recoveries and miss/hit/timeout retries complete in one encounter"))) return;
            Shot(TEXT("10-Victory"));
        }
        if(Time>.6f) { ++Rounds; if(FParse::Param(FCommandLine::Get(),TEXT("CampaignE2E"))&&Rounds>=2)Finish();else{Tap(EKeys::R);Next(19);} }
        break;
    case 17:
        // After two complete rounds, deliberately take a bite through normal play.
        if(P->GetHealth()==2)
        {
            if(!Check(!B->CanMount() && B->GetActionState()==EFuchimatoiActionState::Submerged,TEXT("A direct player hit damages and grants no grab opportunity"))) return;
            if (bRecovery && !bHitRetryDone)
            { bHitRetryDone=true; bSawDodge=false; Next(0); }
            else { Tap(EKeys::R); Next(18); }
        }
        break;
    case 18: if(Time>.2f) { if(Check(P->GetHealth()==3 && !P->IsMounted(),TEXT("Retry also resets player hit state"))) Finish(); } break;
    case 19:
        if(Time>.2f)
        {
            if(!Check(Mode->IsEncounterActive() && B->GetActionState()==EFuchimatoiActionState::Submerged
                && !B->HasBiteTarget() && B->GetHeadProxyLocalLocation().IsZero() && B->GetCoilingProgress()==0
                && B->GetNushiProgressComponent()->GetPurifiedCount()==0 && B->GetNushiState()==ENushiState::Active
                && !P->IsMounted() && P->GetHealth()==3 && P->GetStamina()->GetCurrentStamina()==100
                && FuchimatoiIntegrationTestCountActors(GetWorld())==InitialActors && !B->CanRecover() && B->GetTelemetry().RecoveryGrabs==0,
                TEXT("Input Retry resets player, boss, target, coil, progress, lifecycle without actor growth"))) return;
            for(int32 I=0;I<3;++I) if(!Check(B->GetKakon(I)->GetState()==EKakonState::Exposed,TEXT("Retry restores each Kakon"))) return;
            bSawDodge=false; bSawCoilFollow=false; Next(0);
        }
        break;
    case 20:
        if(Time>.15f)
        {
            if(!Check(!P->IsMounted() && P->GetCharacterMovement()->IsFalling() && !B->CanRecover(),
                TEXT("Detach input falls; airborne recovery is disabled"))) return;
            Shot(FallsRecovered==0?TEXT("11-IntentionalFall"):TEXT("15-SecondFall"));
            Tap(EKeys::E); Next(21);
        }
        break;
    case 21:
        if(P->IsMounted()) { Check(false,TEXT("Airborne grab input cannot recover")); return; }
        // A high detach can land on the bait rock first. Walk off using input
        // before asserting the ground-only recovery condition.
        if(P->GetCharacterMovement()->IsMovingOnGround() && !P->IsOnRecoveryGround())
            MoveToward(B->GetRecoveryAnchor()->GetActorLocation());
        if(P->IsOnRecoveryGround())
        {
            SetMove(0,0);
            if(!Check(B->CanRecover() && B->GetNushiProgressComponent()->GetPurifiedCount()==FallsRecovered+1
                && B->IsCoilingComplete() && B->IsCoiling() && B->GetActorTransform().Equals(RecoveryBossPose)
                && B->GetRouteAnchor(3)->GetActorLocation().Equals(RecoveryRoutePosition)
                && B->GetRouteAnchor(4)->GetActorLocation().Equals(RockBefore)
                && B->GetKakon(0)->GetState()==EKakonState::Purified
                && B->GetKakon(1)->GetState()==(FallsRecovered==0?EKakonState::Exposed:EKakonState::Purified)
                && B->GetKakon(2)->GetState()==EKakonState::Exposed,
                TEXT("Floor landing preserves all Kakon, progress, boss pose, coil and route phase"))) return;
            Shot(TEXT("12-RecoveryPoint")); Next(22);
        }
        break;
    case 22:
        MoveToward(B->GetRecoveryAnchor()->GetActorLocation());
        if(FVector::Dist2D(P->GetActorLocation(),B->GetRecoveryAnchor()->GetActorLocation())<100)
        {
            SetMove(0,0);
            if(FParse::Param(FCommandLine::Get(),TEXT("CampaignE2E"))){Tap(EKeys::E);Next(25);}
            else { P->GetStamina()->ConsumeStamina(100); Tap(EKeys::E); Next(23); }
        }
        break;
    case 23:
        if(Time>.1f)
        {
            if(!Check(!P->IsMounted() && P->GetStamina()->GetCurrentStamina()<P->MinimumGrabStamina,
                TEXT("Recovery rejects low stamina instead of instant falling"))) return;
            Next(24);
        }
        break;
    case 24:
        if(P->GetStamina()->GetCurrentStamina()>=P->MinimumGrabStamina)
        { Tap(EKeys::E); Next(25); }
        break;
    case 25:
        if(Time>.15f)
        {
            if(!Check(P->IsMounted() && P->IsRecovering() && !B->CanRecover()
                && P->GetGrab()->GetGrabTarget()==B->GetRecoveryAnchor(),TEXT("Recovery input uses shared local-transform grab"))) return;
            Shot(TEXT("13-RecoveryGrab")); Next(26);
        }
        break;
    case 26:
        if(!P->IsRecovering())
        {
            if(!Check(P->IsMounted() && P->GetRouteNode()==B->RecoveryNode && !P->IsRouteMoving()
                && P->GetGrab()->GetGrabTarget()==B->GetRouteAnchor(B->RecoveryNode)
                && FVector::Dist(P->GetActorLocation(),B->GetRouteAnchor(B->RecoveryNode)->GetActorLocation())<3
                && P->GetStamina()->GetCurrentStamina()>=25,
                TEXT("Recovery reconnects to existing node 3 with stable pose and stamina"))) return;
            ++FallsRecovered; Shot(TEXT("14-RouteRecovered")); SetMove(1,0); Next(9);
        }
        break;
    case 30:
        MoveToward(FVector(-300,-900,92));
        if(FVector::Dist2D(P->GetActorLocation(),FVector(-300,-900,92))<20) { SetMove(0,0); Next(31); }
        break;
    case 31:
        if(B->GetActionState()==EFuchimatoiActionState::BiteLunge)
        { MoveToward(P->GetActorLocation()+FVector(0,-1000,0)); Next(32); }
        break;
    case 32:
        if(Time>.05f && !bSawDodge) Tap(EKeys::LeftShift);
        if(B->GetActionState()==EFuchimatoiActionState::Submerged)
        {
            SetMove(0,0);
            if(!Check(P->GetHealth()==3 && B->GetTelemetry().RockLures==0 && !B->CanMount(),
                TEXT("Bite misses player and rock then naturally returns to Submerged"))) return;
            bMissCycleDone=true; bSawDodge=false; Next(0);
        }
        break;
    case 40:
        if(B->GetActionState()==EFuchimatoiActionState::Submerged)
        {
            if(!Check(!P->IsMounted() && !B->CanMount() && !B->CanRecover()
                && B->GetNushiProgressComponent()->GetPurifiedCount()==0,
                TEXT("Missed Snag timeout returns to Submerged with no Retry"))) return;
            bMissedGrabDone=true; bSawDodge=false; Next(0);
        }
        break;
    }
}
void AFuchimatoiIntegrationTest::Finish()
{
    if(!Check(Rounds==2,TEXT("Two complete input-driven clears and retries"))) return;
    if (bRealtime) RecordPerformance();
    if (bDone) return;
    bDone=true; FApp::SetUseFixedTimeStep(false);
    UE_LOG(LogTemp,Display,TEXT("FUCHIMATOI_TEST_PASS %s rounds=%d seconds=%.2f"),*RunId,Rounds,Total);
    if(!FParse::Param(FCommandLine::Get(),TEXT("CampaignE2E")))FPlatformMisc::RequestExitWithStatus(false,0);
}

void AFuchimatoiIntegrationTest::RecordPerformance()
{
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("FuchimatoiPerformance")/RunId;
    IFileManager::Get().MakeDirectory(*Dir,true);
    TArray<FString> Rows; Rows.Add(TEXT("frame,wall_ms"));
    double Sum=0; int32 OverBudget=0, Hitches=0;
    for (int32 I=0; I<FrameTimes.Num(); ++I)
    {
        const double Ms=FrameTimes[I]; Sum+=Ms;
        OverBudget+=Ms>1000.0/TargetFPS+1.0;
        Hitches+=Ms>50.0;
        Rows.Add(FString::Printf(TEXT("%d,%.4f"),I,Ms));
    }
    if (!Check(FrameTimes.Num()>0 && FFileHelper::SaveStringArrayToFile(Rows,*(Dir/TEXT("Frames.csv"))),
        TEXT("Realtime wall-clock frame samples saved"))) return;
    FrameTimes.Sort();
    const auto Percentile=[&](double P) { return FrameTimes[FMath::Clamp(FMath::CeilToInt(P*FrameTimes.Num())-1,0,FrameTimes.Num()-1)]; };
    UE_LOG(LogTemp,Display,TEXT("FUCHIMATOI_PERF %s target_fps=%d frames=%d avg_fps=%.2f p95_ms=%.3f p99_ms=%.3f max_ms=%.3f over_budget_plus_1ms=%d over_50ms=%d warmup_seconds=2 rendered=1 fixed_timestep=0"),
        *RunId,TargetFPS,FrameTimes.Num(),1000.0*FrameTimes.Num()/Sum,
        Percentile(.95),Percentile(.99),FrameTimes.Last(),OverBudget,Hitches);
}
