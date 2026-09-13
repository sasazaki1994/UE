#include "FuchimatoiGameMode.h"
#include "FuchimatoiBoss.h"
#include "FuchimatoiArena.h"
#include "FuchimatoiPlayer.h"
#include "FuchimatoiHUD.h"
#include "FuchimatoiIntegrationTest.h"
#include "NushiEncounterManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
AFuchimatoiGameMode::AFuchimatoiGameMode()
{
    DefaultPawnClass=nullptr; HUDClass=AFuchimatoiHUD::StaticClass();
}
void AFuchimatoiGameMode::StartPlay()
{
    Super::StartPlay();
    Arena=GetWorld()->SpawnActor<AFuchimatoiArena>();
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Player=GetWorld()->SpawnActor<AFuchimatoiPlayer>(AFuchimatoiPlayer::StaticClass(),Arena->GetPlayerSpawn(),Params);
    Boss=GetWorld()->SpawnActor<AFuchimatoiBoss>(AFuchimatoiBoss::StaticClass(),Arena->GetBossSpawn(),Params);
    Manager=GetWorld()->SpawnActor<ANushiEncounterManager>();
    APlayerController* PC=UGameplayStatics::GetPlayerController(this,0);
    if (!Player || !Boss || !Manager || !PC) { bDefeated=true; return; }
    PC->Possess(Player); PC->SetInputMode(FInputModeGameOnly()); PC->bShowMouseCursor=false;
    PC->PlayerCameraManager->ViewPitchMin=-70; PC->PlayerCameraManager->ViewPitchMax=15;
    Boss->ConfigureEncounter(Arena,Player); Player->ConfigureBoss(Boss);
    Manager->SetNushi(Boss);
    Manager->OnEncounterCompleted.AddUniqueDynamic(this,&AFuchimatoiGameMode::HandleCompleted);
    RetryEncounter();
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(),TEXT("FuchimatoiTest"))) GetWorld()->SpawnActor<AFuchimatoiIntegrationTest>();
#endif
}
void AFuchimatoiGameMode::RetryEncounter()
{
    if (!Player || !Manager) return;
    Player->ResetForEncounter(Arena->GetPlayerSpawn());
    Manager->ResetEncounter(); bDefeated=false; Manager->StartEncounter();
}
bool AFuchimatoiGameMode::IsEncounterActive() const { return Manager && !bDefeated && Manager->GetEncounterState()==ENushiEncounterState::Running; }
bool AFuchimatoiGameMode::IsVictory() const { return Manager && Manager->GetEncounterState()==ENushiEncounterState::Completed; }
void AFuchimatoiGameMode::HandleCompleted()
{
    Player->StopEncounter(); UE_LOG(LogTemp,Display,TEXT("FUCHIMATOI_VICTORY via EncounterManager"));
}
void AFuchimatoiGameMode::Defeat()
{
    if (!IsEncounterActive()) return;
    bDefeated=true; Player->StopEncounter();
}
