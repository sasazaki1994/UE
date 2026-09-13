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
#include "InputKeyEventArgs.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"

namespace { int32 CountActors(UWorld* World) { int32 Count=0; for(TActorIterator<AActor> It(World);It;++It) ++Count; return Count; } }
AFuchimatoiIntegrationTest::AFuchimatoiIntegrationTest()
{
    PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.TickGroup=TG_PostUpdateWork;
}
void AFuchimatoiIntegrationTest::BeginPlay()
{
    Super::BeginPlay();
    FParse::Value(FCommandLine::Get(),TEXT("PrototypeTestRun="),RunId);
    int32 FPS=60; FParse::Value(FCommandLine::Get(),TEXT("PrototypeTestFPS="),FPS);
    FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0/FPS);
    bCapture=FParse::Param(FCommandLine::Get(),TEXT("PrototypeCapture"));
    bGamepad=FParse::Param(FCommandLine::Get(),TEXT("FuchimatoiGamepad"));
    Mode=GetWorld()->GetAuthGameMode<AFuchimatoiGameMode>();
    if (!Check(Mode && Mode->GetPlayer() && Mode->GetBoss() && Mode->GetEncounterManager(),TEXT("Playable encounter spawned"))) return;
    if (!Check(Mode->IsEncounterActive() && Mode->GetBoss()->GetActionState()==EFuchimatoiActionState::Submerged
        && Mode->GetBoss()->GetNushiProgressComponent()->GetRegisteredKakonCount()==3,TEXT("Start: Submerged, Active, Running, three Kakon"))) return;
    InitialActors=CountActors(GetWorld());
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
    FScreenshotRequest::RequestScreenshot(Dir/(FString(Name)+TEXT(".png")),false,false);
}
void AFuchimatoiIntegrationTest::Tick(float Dt)
{
    Super::Tick(Dt); if (bDone || !Mode) return;
    for(const FKey& Key:Releases) Hold(Key,false); Releases.Empty();
    AFuchimatoiPlayer* P=Mode->GetPlayer(); AFuchimatoiBoss* B=Mode->GetBoss();
    if(bGamepad)
    {
        APlayerController* PC=Cast<APlayerController>(P->GetController());
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftY,IE_Axis,ForwardAxis));
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftX,IE_Axis,RightAxis));
    }
    Time+=Dt; Total+=Dt; bSawDodge|=P->IsDodging();
    if (Total>240 || Time>35) { Check(false,TEXT("Input playthrough timeout")); return; }
    if (Phase>=1 && Phase<=4 && B->GetActionState()==EFuchimatoiActionState::BiteLunge && !LockedTarget.IsZero())
        if (!Check(B->GetBiteTargetLocalLocation().Equals(LockedTarget),TEXT("Bite target stays locked during evasion"))) return;
    switch(Phase)
    {
    case 0:
        MoveToward(Mode->GetArena()->GetBaitPosition());
        if (FVector::Dist2D(P->GetActorLocation(),Mode->GetArena()->GetBaitPosition())<20)
        { SetMove(0,0); LockedTarget=FVector::ZeroVector; Next(Rounds>=2?17:1); }
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
            if (!Check(bSawDodge && P->GetHealth()==3 && B->CanMount(),TEXT("Input dodge baits rock collision and opens Snagged grab"))) return;
            Shot(TEXT("03-Snagged")); Next(5);
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
            if(!Check(FVector::Dist(BeforeAnchor,B->GetRouteAnchor(3)->GetActorLocation())>100
                && RockBefore.Equals(B->GetRouteAnchor(4)->GetActorLocation(),.01f),TEXT("Coiling reconfigures snake route while rock anchor stays fixed"))) return;
            SetMove(1,0); Next(9);
        }
        break;
    case 9:
        if(P->GetRouteNode()==4 && !P->IsRouteMoving())
        { SetMove(0,0); BeforeStamina=P->GetStamina()->GetCurrentStamina(); Shot(TEXT("07-SnakeToRock")); Next(10); }
        break;
    case 10:
        if(Time>1.2f)
        {
            if(!Check(P->GetGrab()->GetGrabTarget()==B->GetRouteAnchor(4) && B->GetRouteAnchor(4)->IsRock()
                && P->GetStamina()->GetCurrentStamina()>BeforeStamina,TEXT("Snake to rock transfer changes grab owner and restores stamina"))) return;
            SetMove(1,0); Next(11);
        }
        break;
    case 11:
        if(P->GetRouteNode()==6 && !P->IsRouteMoving()) { SetMove(0,0); Next(12); }
        break;
    case 12:
        if(Time>1.2f) { SetMove(1,0); Next(13); }
        break;
    case 13:
        if(P->GetRouteNode()==7 && !P->IsRouteMoving())
        { SetMove(0,0); Shot(TEXT("08-Kakon2")); Tap(EKeys::LeftMouseButton); Next(14); }
        break;
    case 14:
        if(Time>.2f)
        {
            if(!Check(B->GetNushiProgressComponent()->GetPurifiedCount()==2,TEXT("Second serpent Kakon advances shared progress to 2/3"))) return;
            SetMove(1,0); Next(15);
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
            Shot(TEXT("10-Victory"));
        }
        if(Time>.6f) { ++Rounds; Tap(EKeys::R); Next(19); }
        break;
    case 17:
        // After two complete rounds, deliberately take a bite through normal play.
        if(P->GetHealth()==2)
        {
            if(!Check(!B->CanMount() && B->GetActionState()==EFuchimatoiActionState::Submerged,TEXT("A direct player hit damages and grants no grab opportunity"))) return;
            Tap(EKeys::R); Next(18);
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
                && CountActors(GetWorld())==InitialActors,TEXT("Input Retry resets player, boss, target, coil, progress, lifecycle without actor growth"))) return;
            for(int32 I=0;I<3;++I) if(!Check(B->GetKakon(I)->GetState()==EKakonState::Exposed,TEXT("Retry restores each Kakon"))) return;
            bSawDodge=false; bSawCoilFollow=false; Next(0);
        }
        break;
    }
}
void AFuchimatoiIntegrationTest::Finish()
{
    if(!Check(Rounds==2,TEXT("Two complete input-driven clears and retries"))) return;
    bDone=true; FApp::SetUseFixedTimeStep(false);
    UE_LOG(LogTemp,Display,TEXT("FUCHIMATOI_TEST_PASS %s rounds=%d seconds=%.2f"),*RunId,Rounds,Total);
    FPlatformMisc::RequestExitWithStatus(false,0);
}
