#include "CampaignGameInstance.h"
#include "PlayerSenseComponent.h"
#include "Components/ActorComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
const FName CampaignMap(TEXT("/Game/Maps/L_Prototype_01"));

const TCHAR* GameModeFor(ECampaignState State)
{
    switch(State)
    {
    case ECampaignState::Ishibashiri: return TEXT("/Script/IshibashiriPrototype.PrototypeGameMode");
    case ECampaignState::Fuchimatoi: return TEXT("/Script/IshibashiriPrototype.FuchimatoiGameMode");
    case ECampaignState::Minedaki: return TEXT("/Script/IshibashiriPrototype.MinedakiGameMode");
    case ECampaignState::Magatsune: return TEXT("/Script/IshibashiriPrototype.MagatsuneGameMode");
    default: return TEXT("/Script/IshibashiriPrototype.CampaignGameMode");
    }
}
}

void UCampaignGameInstance::Init()
{
    Super::Init();
    bCampaignActive=FParse::Param(FCommandLine::Get(),TEXT("Campaign"));
    State=ECampaignState::Title;
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
    case ECampaignState::Prologue: State=ECampaignState::Ishibashiri; break;
    case ECampaignState::Interlude1: State=ECampaignState::Fuchimatoi; break;
    case ECampaignState::Interlude2: State=ECampaignState::Minedaki; break;
    case ECampaignState::Interlude3: State=ECampaignState::Magatsune; break;
    case ECampaignState::Ending: State=ECampaignState::Completed; break;
    case ECampaignState::Completed: State=ECampaignState::Title; break;
    default: return false;
    }
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
    return true;
}

void UCampaignGameInstance::RestartCampaign()
{
    bCampaignActive=true;
    State=ECampaignState::Title;
}

bool UCampaignGameInstance::IsEncounterState(ECampaignState Value)
{
    return Value==ECampaignState::Ishibashiri || Value==ECampaignState::Fuchimatoi
        || Value==ECampaignState::Minedaki || Value==ECampaignState::Magatsune;
}

void UCampaignGameInstance::ResetSenseState(UPlayerSenseComponent* Sense)
{
    if(Sense) Sense->ResetSense();
}

void UCampaignGameInstance::ResetChapterRuntime(UObject* WorldContext)
{
    if(!WorldContext || !WorldContext->GetWorld()) return;
    for(TActorIterator<APawn> It(WorldContext->GetWorld());It;++It)
        ResetSenseState(It->FindComponentByClass<UPlayerSenseComponent>());
}

void UCampaignGameInstance::TravelToCurrentChapter(UObject* WorldContext)
{
    ResetChapterRuntime(WorldContext);
    const FString Options=FString::Printf(TEXT("game=%s"),GameModeFor(State));
    UGameplayStatics::OpenLevel(WorldContext,CampaignMap,true,Options);
}
