#include "MagatsuneGameMode.h"
#include "CampaignGameInstance.h"
#include "MagatsuneBoss.h"
#include "MagatsunePlayer.h"
#include "MagatsuneArena.h"
#include "MagatsuneHUD.h"
#include "MagatsuneIntegrationTest.h"
#include "NushiEncounterManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

AMagatsuneGameMode::AMagatsuneGameMode()
{
    DefaultPawnClass = nullptr;
    HUDClass = AMagatsuneHUD::StaticClass();
}

void AMagatsuneGameMode::StartPlay()
{
    Super::StartPlay();
    GetWorld()->SpawnActor<AMagatsuneArena>();
    FActorSpawnParameters P;
    P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Boss = GetWorld()->SpawnActor<AMagatsuneBoss>();
    Player = GetWorld()->SpawnActor<AMagatsunePlayer>(AMagatsunePlayer::StaticClass(), AMagatsunePlayer::SpawnTransform(), P);
    Manager = GetWorld()->SpawnActor<ANushiEncounterManager>();
    auto* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!Boss || !Player || !Manager || !PC)
    {
        UE_LOG(LogTemp, Error, TEXT("MAGATSUNE_SPAWN_FAILED"));
        return;
    }
    PC->Possess(Player);
    PC->SetInputMode(FInputModeGameOnly());
    Player->ConfigureBoss(Boss);
    Boss->ConfigurePlayer(Player);
    Manager->SetNushi(Boss);
    Manager->OnEncounterCompleted.AddUniqueDynamic(this, &AMagatsuneGameMode::HandleCompleted);
    RetryEncounter();
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("MagatsuneTest")) || FParse::Param(FCommandLine::Get(), TEXT("CampaignE2E")))
        GetWorld()->SpawnActor<AMagatsuneIntegrationTest>();
#endif
}

void AMagatsuneGameMode::RetryEncounter()
{
    if (Boss && Player && Manager)
    {
        GetWorldTimerManager().ClearTimer(CampaignAdvanceTimer);
        ++Boss->Telemetry.Retries;
        Boss->LogTelemetry(TEXT("Retry"));
        Player->ResetForEncounter();
        Manager->ResetEncounter();
        Manager->StartEncounter();
    }
}

void AMagatsuneGameMode::HandleCompleted()
{
    if (auto* C = GetGameInstance<UCampaignGameInstance>(); C && C->IsCurrentEncounter(ECampaignState::Magatsune))
        GetWorldTimerManager().SetTimer(CampaignAdvanceTimer, this, &AMagatsuneGameMode::AdvanceCampaign, 2.f, false);
}

void AMagatsuneGameMode::AdvanceCampaign()
{
    if (auto* C = GetGameInstance<UCampaignGameInstance>(); C && C->CompleteEncounter(ECampaignState::Magatsune))
        C->TravelToCurrentChapter(this);
}
