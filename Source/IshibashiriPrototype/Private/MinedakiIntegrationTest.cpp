#include "MinedakiIntegrationTest.h"
#include "MinedakiGameMode.h"
#include "MinedakiBoss.h"
#include "MinedakiPlayer.h"
#include "GrabComponent.h"
#include "StaminaComponent.h"
#include "KakonActor.h"
#include "NushiEncounterManager.h"
#include "NushiProgressComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SceneComponent.h"
#include "InputKeyEventArgs.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"
namespace { int32 ActorCount(UWorld* W) { int32 N=0; for(TActorIterator<AActor> It(W);It;++It) ++N; return N; } }
AMinedakiIntegrationTest::AMinedakiIntegrationTest() { PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.TickGroup=TG_PostUpdateWork; }
void AMinedakiIntegrationTest::BeginPlay()
{
    Super::BeginPlay(); int32 FPS=60;
    FParse::Value(FCommandLine::Get(),TEXT("PrototypeTestRun="),RunId); FParse::Value(FCommandLine::Get(),TEXT("PrototypeTestFPS="),FPS);
    bGamepad=FParse::Param(FCommandLine::Get(),TEXT("MinedakiGamepad")); bCapture=FParse::Param(FCommandLine::Get(),TEXT("PrototypeCapture"));
    FApp::SetUseFixedTimeStep(true); FApp::SetFixedDeltaTime(1.0/FPS);
    Mode=GetWorld()->GetAuthGameMode<AMinedakiGameMode>();
    if(!Check(Mode && Mode->GetBoss() && Mode->GetPlayer() && Mode->GetManager(),TEXT("Encounter spawned"))) return;
    InitialActors=ActorCount(GetWorld()); AddTickPrerequisiteComponent(Mode->GetPlayer()->GetGrab());
}
bool AMinedakiIntegrationTest::Check(bool Condition,const TCHAR* Message)
{
    if(!Condition) { UE_LOG(LogTemp,Error,TEXT("MINEDAKI_TEST_FAIL %s phase=%d round=%d seconds=%.3f: %s"),*RunId,Phase,Rounds,Total,Message); bDone=true; FApp::SetUseFixedTimeStep(false); FPlatformMisc::RequestExitWithStatus(false,1); }
    else UE_LOG(LogTemp,Display,TEXT("MINEDAKI_CHECK %s"),Message);
    return Condition;
}
void AMinedakiIntegrationTest::Hold(FKey Key,bool Down)
{
    if(Held.Contains(Key)==Down) return;
    FKey Mapped=Key;
    if(bGamepad) { if(Key==EKeys::E) Mapped=EKeys::Gamepad_RightShoulder; if(Key==EKeys::R) Mapped=EKeys::Gamepad_FaceButton_Top; if(Key==EKeys::LeftMouseButton) Mapped=EKeys::Gamepad_FaceButton_Left; }
    Cast<APlayerController>(Mode->GetPlayer()->GetController())->InputKey(FInputKeyEventArgs::CreateSimulated(Mapped,Down?IE_Pressed:IE_Released,Down?1.f:0.f));
    if(Down) Held.Add(Key); else Held.Remove(Key);
}
void AMinedakiIntegrationTest::Tap(FKey Key) { Hold(Key,true); Releases.Add(Key); }
void AMinedakiIntegrationTest::Move(float Forward,float Right)
{
    ForwardAxis=Forward; RightAxis=Right; if(bGamepad) return;
    Hold(EKeys::W,Forward>.2f); Hold(EKeys::S,Forward<-.2f); Hold(EKeys::D,Right>.2f); Hold(EKeys::A,Right<-.2f);
}
void AMinedakiIntegrationTest::MoveToward(FVector Target)
{
    auto* P=Mode->GetPlayer(); FVector Direction=(Target-P->GetActorLocation()).GetSafeNormal2D();
    FRotationMatrix Rotation(FRotator(0,P->GetControlRotation().Yaw,0));
    Move(FVector::DotProduct(Direction,Rotation.GetUnitAxis(EAxis::X)),FVector::DotProduct(Direction,Rotation.GetUnitAxis(EAxis::Y)));
}
void AMinedakiIntegrationTest::Shot(const TCHAR* Name)
{
    if(!bCapture || Rounds!=0 || Captures.Contains(Name)) return;
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("Screenshots/Minedaki")/RunId; IFileManager::Get().MakeDirectory(*Dir,true);
    FScreenshotRequest::RequestScreenshot(Dir/(FString(Name)+TEXT(".png")),false,false); Captures.Add(Name);
}
void AMinedakiIntegrationTest::Tick(float Dt)
{
    Super::Tick(Dt); if(bDone || !Mode) return;
    for(FKey Key:Releases) Hold(Key,false); Releases.Empty();
    auto* P=Mode->GetPlayer(); auto* B=Mode->GetBoss();
    if(bGamepad)
    {
        auto* PC=Cast<APlayerController>(P->GetController());
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftY,IE_Axis,ForwardAxis));
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftX,IE_Axis,RightAxis));
    }
    Time+=Dt; Total+=Dt;
    if(Total>240 || Time>40) { Check(false,TEXT("Input playthrough timeout")); return; }
    if(P->IsMounted())
    {
        const FTransform Expected=P->GetGrab()->GetRelativeGrabTransform()*B->GetGrabFrame()->GetActorTransform();
        MaxFollowError=FMath::Max(MaxFollowError,static_cast<float>(FVector::Dist(Expected.GetLocation(),P->GetActorLocation())));
        MaxAngleError=FMath::Max(MaxAngleError,static_cast<float>(FMath::RadiansToDegrees(Expected.GetRotation().AngularDistance(P->GetActorQuat()))));
        if(MaxFollowError>1 || MaxAngleError>.1f) { Check(false,TEXT("Full transform attachment error")); return; }
    }
    PeakPitch=FMath::Max(PeakPitch,static_cast<float>(B->GetBodyRoot()->GetRelativeRotation().Pitch));
    PeakYaw=FMath::Max(PeakYaw,static_cast<float>(B->GetBodyRoot()->GetRelativeRotation().Yaw));
    PeakRoll=FMath::Max(PeakRoll,static_cast<float>(B->GetBodyRoot()->GetRelativeRotation().Roll));
    switch(Phase)
    {
    case 0:
        if(Time>.1f) Shot(TEXT("01-Ground"));
        MoveToward(B->GetRouteWorld(0));
        if(FVector::Dist(P->GetActorLocation(),B->GetRouteWorld(0))<P->GrabRange-30)
        { Move(0); Tap(EKeys::E); Next(1); }
        break;
    case 1:
        if(Time>.15f)
        {
            if(!Check(P->IsMounted() && P->GetRouteNode()==0,TEXT("Input leg Grab through shared component"))) return;
            Shot(TEXT("02-LegGrab")); Move(1); Next(2);
        }
        break;
    case 2:
        if(P->GetRouteNode()==2) Shot(TEXT("03-Waist"));
        if(B->IsWallMoving())
        { Move(0); WallRelative=P->GetGrab()->GetRelativeGrabTransform(); if(Rounds<2) Hold(EKeys::E,true); Shot(TEXT("04-WallStart")); Next(3); }
        break;
    case 3:
        if(P->IsMounted() && !P->GetGrab()->GetRelativeGrabTransform().Equals(WallRelative,.01f)) { Check(false,TEXT("Wall phase must preserve the mounted body location")); return; }
        if(Rounds<2 && P->HasFallen()) { Check(false,TEXT("Cling should survive entire climb")); return; }
        if(B->GetBodyRoot()->GetRelativeRotation().Pitch>45 && B->GetBodyRoot()->GetRelativeRotation().Pitch<55) Shot(TEXT("05-Pitch45"));
        if(B->GetClimbTime()>B->PrepareSeconds+1.8f) Shot(TEXT("06-Cling"));
        if(B->GetActionState()==EMinedakiActionState::Shaking) { bSawShake=true; if(B->GetBodyRoot()->GetRelativeRotation().Roll>18) Shot(TEXT("07-Shake")); }
        if(B->GetClimbTime()>B->PrepareSeconds+B->WallSeconds*.8f) Shot(TEXT("08-HighWall"));
        if(Rounds==2 && P->HasFallen()) { Move(0); Next(7); break; }
        if(B->GetActionState()==EMinedakiActionState::UpperPlatform)
        {
            if(!Check(P->IsMounted() && P->GetRouteNode()==4 && bSawShake && PeakPitch>=69 && PeakYaw>=19 && PeakRoll>=23 && B->Telemetry.Shakes==1
                && P->GetActorLocation().Z>3000 && P->GetStamina()->GetCurrentStamina()<80,
                TEXT("Wall traversal: large pitch/yaw, shake, stamina cost, stable attachment, upper height"))) return;
            Shot(TEXT("09-UpperLedge")); Hold(EKeys::E,false); Next(4);
        }
        break;
    case 4:
        if(Time>1.5f) { Move(1); Next(5); }
        break;
    case 5:
        if(P->GetRouteNode()==6 && !P->IsRouteMoving()) { Move(0); Shot(TEXT("10-Kakon1")); Tap(EKeys::LeftMouseButton); Next(6); }
        break;
    case 6:
        if(Time>.2f)
        {
            if(!Check(B->IsSliceComplete() && B->GetNushiProgressComponent()->GetPurifiedCount()==1
                && B->GetNushiProgressComponent()->GetRegisteredKakonCount()==3 && B->GetNushiState()==ENushiState::Active
                && Mode->GetManager()->GetEncounterState()==ENushiEncounterState::Running
                && B->GetKakon(1)->GetState()==EKakonState::Covered && B->GetKakon(2)->GetState()==EKakonState::Covered,
                TEXT("Input Kakon1: slice complete at 1/3, Active/Running, future Kakon inaccessible"))) return;
            Shot(TEXT("11-SliceComplete")); Next(8);
        }
        break;
    case 7:
        if(P->GetCharacterMovement()->IsMovingOnGround() && P->GetActorLocation().Z<150)
        { if(!Check(B->Telemetry.Falls==1 && !P->IsMounted(),TEXT("No-cling shake falls to ground"))) return; bFallValidated=true; Tap(EKeys::R); Next(9); }
        break;
    case 8: if(Time>.7f) { ++Rounds; Tap(EKeys::R); Next(9); } break;
    case 9:
        if(Time>.2f)
        {
            if(!Check(!P->IsMounted() && !P->HasFallen() && P->GetStamina()->GetCurrentStamina()==100
                && B->GetActionState()==EMinedakiActionState::Grounded && B->GetNushiProgressComponent()->GetPurifiedCount()==0
                && B->GetKakon(0)->GetState()==EKakonState::Covered && B->Telemetry.Shakes==0 && ActorCount(GetWorld())==InitialActors,
                TEXT("Input Retry resets transforms, stamina, flags, Kakon, telemetry with no actor growth"))) return;
            if(Rounds==2 && bFallValidated)
            {
                UE_LOG(LogTemp,Display,TEXT("MINEDAKI_TEST_PASS %s rounds=2 fall_retry=1 seconds=%.3f max_position_error_cm=%.6f max_rotation_error_deg=%.6f peak_pitch=%.2f peak_yaw=%.2f peak_roll=%.2f gamepad=%d"),*RunId,Total,MaxFollowError,MaxAngleError,PeakPitch,PeakYaw,PeakRoll,bGamepad);
                bDone=true; FApp::SetUseFixedTimeStep(false); FPlatformMisc::RequestExitWithStatus(false,0);
            }
            else { bSawShake=false; Next(0); }
        }
        break;
    }
}
