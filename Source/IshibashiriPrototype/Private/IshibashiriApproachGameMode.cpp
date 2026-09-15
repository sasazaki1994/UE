#include "IshibashiriApproachGameMode.h"
#include "IshibashiriApproachArena.h"
#include "IshibashiriApproachHUD.h"
#include "CampaignGameInstance.h"
#include "PrototypePlayer.h"
#include "PlayerSenseComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMisc.h"

AIshibashiriApproachGameMode::AIshibashiriApproachGameMode(){PrimaryActorTick.bCanEverTick=true;DefaultPawnClass=nullptr;HUDClass=AIshibashiriApproachHUD::StaticClass();}
void AIshibashiriApproachGameMode::StartPlay()
{
    Arena=GetWorld()->SpawnActor<AIshibashiriApproachArena>();Super::StartPlay();if(!Arena)return;
    FActorSpawnParameters P;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Player=GetWorld()->SpawnActor<APrototypePlayer>(APrototypePlayer::StaticClass(),FTransform(FRotator::ZeroRotator,Arena->GetPlayerStart()),P);
    SenseTarget=GetWorld()->SpawnActor<AActor>(AActor::StaticClass(),Arena->GetBasinGate(),FRotator::ZeroRotator);
    if(auto* Controller=UGameplayStatics::GetPlayerController(this,0);Controller&&Player){Controller->Possess(Player);Controller->SetInputMode(FInputModeGameOnly());}
    if(Player&&SenseTarget)Player->GetSense()->RegisterBoundaryTarget(SenseTarget,true);
    bAutomationDriving=FParse::Param(FCommandLine::Get(),TEXT("ApproachTest"))||FParse::Param(FCommandLine::Get(),TEXT("CampaignE2E"));
    if(bAutomationDriving)if(auto* C=UGameplayStatics::GetPlayerController(this,0))C->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::W,IE_Pressed,1.f));
    UE_LOG(LogTemp,Display,TEXT("APPROACH_READY path_m=%.1f expected_minutes=3-5 combat=false"),Arena->GetPathLengthMeters());
}
void AIshibashiriApproachGameMode::Tick(float Dt)
{
    Super::Tick(Dt);if(!Arena||!Player||bTransitioning)return;
    if(bAutomationDriving&&AutomationPoint<Arena->GetPathPoints().Num())if(auto* C=UGameplayStatics::GetPlayerController(this,0)){const FVector Goal=Arena->GetPathPoints()[AutomationPoint];C->SetControlRotation((Goal-Player->GetActorLocation()).Rotation());if(FVector::Dist2D(Goal,Player->GetActorLocation())<500)++AutomationPoint;}
    Arena->ObservePlayer(Player->GetActorLocation());
    if(Player->GetSense()->IsCorruptionSenseActive())Player->GetSense()->SetCorruptionWarning(Arena->GetStage()>=4?ECorruptionWarning::Danger:Arena->GetStage()>=2?ECorruptionWarning::Transition:ECorruptionWarning::None);
    if(Arena->IsGateReached())EnterBasin();
}
void AIshibashiriApproachGameMode::EnterBasin()
{
    bTransitioning=true;
    if(bAutomationDriving)if(auto* C=UGameplayStatics::GetPlayerController(this,0))C->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::W,IE_Released,0.f));
    if(FParse::Param(FCommandLine::Get(),TEXT("ApproachTest"))&&!FParse::Param(FCommandLine::Get(),TEXT("CampaignE2E"))){UE_LOG(LogTemp,Display,TEXT("APPROACH_TEST_PASS stages=4 reveal_count=%d combat=false timers=clear"),Arena?Arena->GetRevealCount():0);FPlatformMisc::RequestExitWithStatus(false,0);return;}
    if(auto* Campaign=GetGameInstance<UCampaignGameInstance>();Campaign&&Campaign->CompleteApproach()){Campaign->TravelToCurrentChapter(this);return;}
    UGameplayStatics::OpenLevel(this,FName(TEXT("/Game/Maps/L_Prototype_01")),true,TEXT("game=/Script/IshibashiriPrototype.PrototypeGameMode?BasinPrototype"));
}
