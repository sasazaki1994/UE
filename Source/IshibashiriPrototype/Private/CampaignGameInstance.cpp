#include "CampaignGameInstance.h"
#include "PlayerSenseComponent.h"
#include "Components/ActorComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformTime.h"

namespace
{
const FName CampaignMap(TEXT("/Game/Maps/L_Prototype_01"));

const TCHAR* GameModeFor(ECampaignState State)
{
    switch(State)
    {
    case ECampaignState::IshibashiriApproach: return TEXT("/Script/IshibashiriPrototype.IshibashiriApproachGameMode");
    case ECampaignState::Ishibashiri: return TEXT("/Script/IshibashiriPrototype.PrototypeGameMode");
    case ECampaignState::Fuchimatoi: return TEXT("/Script/IshibashiriPrototype.FuchimatoiGameMode");
    case ECampaignState::Minedaki: return TEXT("/Script/IshibashiriPrototype.MinedakiGameMode");
    case ECampaignState::Magatsune: return TEXT("/Script/IshibashiriPrototype.MagatsuneGameMode");
    default: return TEXT("/Script/IshibashiriPrototype.CampaignGameMode");
    }
}

const TCHAR* CampaignStateName(ECampaignState State)
{
    switch(State)
    {
    case ECampaignState::Title:return TEXT("Title"); case ECampaignState::Prologue:return TEXT("Prologue");
    case ECampaignState::IshibashiriApproach:return TEXT("IshibashiriApproach");
    case ECampaignState::Ishibashiri:return TEXT("Ishibashiri Started"); case ECampaignState::Interlude1:return TEXT("Interlude1");
    case ECampaignState::Fuchimatoi:return TEXT("Fuchimatoi Started"); case ECampaignState::Interlude2:return TEXT("Interlude2");
    case ECampaignState::Minedaki:return TEXT("Minedaki Started"); case ECampaignState::Interlude3:return TEXT("Interlude3");
    case ECampaignState::Magatsune:return TEXT("Magatsune Started"); case ECampaignState::Ending:return TEXT("Ending");
    case ECampaignState::Completed:return TEXT("Completed"); default:return TEXT("Unknown");
    }
}
}

void UCampaignGameInstance::Init()
{
    Super::Init();
    bCampaignActive=FParse::Param(FCommandLine::Get(),TEXT("Campaign"));
    State=ECampaignState::Title;
    CampaignStartSeconds=ChapterStartSeconds=FPlatformTime::Seconds();
    UE_LOG(LogTemp,Display,TEXT("CAMPAIGN_E2E Title chapter_start_time=0.000"));
}

void UCampaignGameInstance::StartCampaign()
{
    bCampaignActive=true;
    State=ECampaignState::Prologue;
}

bool UCampaignGameInstance::AdvanceCardChapter()
{
    switch(State)
    {
    case ECampaignState::Title: StartCampaign(); break;
    case ECampaignState::Prologue: State=ECampaignState::IshibashiriApproach; break;
    case ECampaignState::Interlude1: State=ECampaignState::Fuchimatoi; break;
    case ECampaignState::Interlude2: State=ECampaignState::Minedaki; break;
    case ECampaignState::Interlude3: State=ECampaignState::Magatsune; break;
    case ECampaignState::Ending: State=ECampaignState::Completed; break;
    case ECampaignState::Completed: State=ECampaignState::Title; break;
    default: return false;
    }
    ChapterStartSeconds=FPlatformTime::Seconds();
    UE_LOG(LogTemp,Display,TEXT("CAMPAIGN_E2E %s chapter_start_time=%.3f"),CampaignStateName(State),GetCampaignElapsedSeconds());
    return true;
}

bool UCampaignGameInstance::CompleteApproach()
{
    if(!bCampaignActive || State!=ECampaignState::IshibashiriApproach) return false;
    State=ECampaignState::Ishibashiri;
    ChapterStartSeconds=FPlatformTime::Seconds();
    UE_LOG(LogTemp,Display,TEXT("CAMPAIGN_E2E IshibashiriApproach Completed approach_clear_time=%.3f"),GetCampaignElapsedSeconds());
    UE_LOG(LogTemp,Display,TEXT("CAMPAIGN_E2E Ishibashiri Started chapter_start_time=%.3f"),GetCampaignElapsedSeconds());
    return true;
}

bool UCampaignGameInstance::CompleteEncounter(ECampaignState Encounter)
{
    if(!bCampaignActive || State!=Encounter || !IsEncounterState(State)) return false;
    switch(State)
    {
    case ECampaignState::Ishibashiri: State=ECampaignState::Interlude1; break;
    case ECampaignState::Fuchimatoi: State=ECampaignState::Interlude2; break;
    case ECampaignState::Minedaki: State=ECampaignState::Interlude3; break;
    case ECampaignState::Magatsune: State=ECampaignState::Ending; break;
    default: return false;
    }
    const double Now=FPlatformTime::Seconds();
    UE_LOG(LogTemp,Display,TEXT("CAMPAIGN_E2E %s Completed encounter_clear_time=%.3f"),CampaignStateName(Encounter),Now-ChapterStartSeconds);
    ChapterStartSeconds=Now;
    UE_LOG(LogTemp,Display,TEXT("CAMPAIGN_E2E %s chapter_start_time=%.3f"),CampaignStateName(State),GetCampaignElapsedSeconds());
    return true;
}

void UCampaignGameInstance::RestartCampaign()
{
    bCampaignActive=true;
    State=ECampaignState::Title;
    CampaignStartSeconds=ChapterStartSeconds=FPlatformTime::Seconds();
    UE_LOG(LogTemp,Display,TEXT("CAMPAIGN_E2E Title chapter_start_time=0.000"));
}

double UCampaignGameInstance::GetCampaignElapsedSeconds() const{return FPlatformTime::Seconds()-CampaignStartSeconds;}

bool UCampaignGameInstance::IsEncounterState(ECampaignState Value)
{
    return Value==ECampaignState::Ishibashiri || Value==ECampaignState::Fuchimatoi
        || Value==ECampaignState::Minedaki || Value==ECampaignState::Magatsune;
}

void UCampaignGameInstance::ResetSenseState(UPlayerSenseComponent* Sense)
{
    if(Sense) Sense->ResetSense();
}

void UCampaignGameInstance::ResetChapterRuntime(UObject* ChapterWorldContext)
{
    if(!ChapterWorldContext || !ChapterWorldContext->GetWorld()) return;
    for(TActorIterator<APawn> It(ChapterWorldContext->GetWorld());It;++It)
        ResetSenseState(It->FindComponentByClass<UPlayerSenseComponent>());
}

void UCampaignGameInstance::TravelToCurrentChapter(UObject* ChapterWorldContext)
{
    ResetChapterRuntime(ChapterWorldContext);
    const FString Options=FString::Printf(TEXT("game=%s"),GameModeFor(State));
    UGameplayStatics::OpenLevel(ChapterWorldContext,CampaignMap,true,Options);
}
