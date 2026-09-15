#include "MinedakiGameMode.h"
#include "CampaignGameInstance.h"
#include "MinedakiBoss.h"
#include "MinedakiPlayer.h"
#include "MinedakiArena.h"
#include "MinedakiHUD.h"
#include "MinedakiIntegrationTest.h"
#include "NushiEncounterManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
AMinedakiGameMode::AMinedakiGameMode() { DefaultPawnClass=nullptr; HUDClass=AMinedakiHUD::StaticClass(); }
void AMinedakiGameMode::StartPlay()
{
    Super::StartPlay(); GetWorld()->SpawnActor<AMinedakiArena>();
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Boss=GetWorld()->SpawnActor<AMinedakiBoss>();
    Player=GetWorld()->SpawnActor<AMinedakiPlayer>(AMinedakiPlayer::StaticClass(),AMinedakiPlayer::SpawnTransform(),Params);
    Manager=GetWorld()->SpawnActor<ANushiEncounterManager>();
    auto* PC=UGameplayStatics::GetPlayerController(this,0);
    if(!Boss || !Player || !Manager || !PC) { UE_LOG(LogTemp,Error,TEXT("MINEDAKI_SPAWN_FAILED")); return; }
    PC->Possess(Player); PC->SetInputMode(FInputModeGameOnly()); PC->bShowMouseCursor=false;
    Player->ConfigureBoss(Boss); Boss->ConfigurePlayer(Player); Manager->SetNushi(Boss);
    Manager->OnEncounterCompleted.AddUniqueDynamic(this,&AMinedakiGameMode::HandleCompleted); RetryEncounter();
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("MinedakiTest")) || FParse::Param(FCommandLine::Get(),TEXT("CampaignE2E"))) GetWorld()->SpawnActor<AMinedakiIntegrationTest>();
#endif
}
void AMinedakiGameMode::RetryEncounter() { if(Player && Manager && Boss) { GetWorldTimerManager().ClearTimer(CampaignAdvanceTimer); Boss->LogTelemetry(TEXT("Retry")); Player->ResetForEncounter(); Manager->ResetEncounter(); Manager->StartEncounter(); } }
void AMinedakiGameMode::HandleCompleted(){if(auto* C=GetGameInstance<UCampaignGameInstance>();C&&C->IsCurrentEncounter(ECampaignState::Minedaki))GetWorldTimerManager().SetTimer(CampaignAdvanceTimer,this,&AMinedakiGameMode::AdvanceCampaign,2.f,false);}
void AMinedakiGameMode::AdvanceCampaign(){if(auto* C=GetGameInstance<UCampaignGameInstance>();C&&C->CompleteEncounter(ECampaignState::Minedaki))C->TravelToCurrentChapter(this);}
