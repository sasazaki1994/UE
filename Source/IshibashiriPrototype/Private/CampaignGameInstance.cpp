#include "CampaignGameInstance.h"
#include "CampaignSaveGame.h"
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
    const FString CampaignSaveSlot(TEXT("MagabaraiCampaign"));

    const TCHAR* GameModeFor(ECampaignState State)
    {
        switch (State)
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
        switch (State)
        {
        case ECampaignState::Title: return TEXT("Title");
        case ECampaignState::Prologue: return TEXT("Prologue");
        case ECampaignState::IshibashiriApproach: return TEXT("IshibashiriApproach");
        case ECampaignState::Ishibashiri: return TEXT("Ishibashiri Started");
        case ECampaignState::Interlude1: return TEXT("Interlude1");
        case ECampaignState::Fuchimatoi: return TEXT("Fuchimatoi Started");
        case ECampaignState::Interlude2: return TEXT("Interlude2");
        case ECampaignState::Minedaki: return TEXT("Minedaki Started");
        case ECampaignState::Interlude3: return TEXT("Interlude3");
        case ECampaignState::Magatsune: return TEXT("Magatsune Started");
        case ECampaignState::Ending: return TEXT("Ending");
        case ECampaignState::Completed: return TEXT("Completed");
        default: return TEXT("Unknown");
        }
    }
}

void UCampaignGameInstance::Init()
{
    Super::Init();
    bIshibashiriDemo = FParse::Param(FCommandLine::Get(), TEXT("IshibashiriDemo"));
    bCampaignActive = FParse::Param(FCommandLine::Get(), TEXT("Campaign")) || bIshibashiriDemo;
    FString TestRun;
    bPersistenceEnabled = bCampaignActive && !bIshibashiriDemo
        && !FParse::Param(FCommandLine::Get(), TEXT("CampaignE2E"))
        && !FParse::Value(FCommandLine::Get(), TEXT("PrototypeTestRun="), TestRun);
    State = ECampaignState::Title;
    if (bCampaignActive && bPersistenceEnabled) LoadChapterSave();
    CampaignStartSeconds = ChapterStartSeconds = FPlatformTime::Seconds();
    UE_LOG(LogTemp, Display, TEXT("CAMPAIGN_E2E Title chapter_start_time=0.000"));
}

void UCampaignGameInstance::StartCampaign()
{
    bCampaignActive = true;
    bIshibashiriDemoCompleteLogged = false;
    State = ECampaignState::Prologue;
    CampaignStartSeconds = ChapterStartSeconds = FPlatformTime::Seconds();
    SaveChapter();
}

bool UCampaignGameInstance::AdvanceCardChapter()
{
    const bool bStartingFromTitle = State == ECampaignState::Title;
    switch (State)
    {
    case ECampaignState::Title: StartCampaign(); break;
    case ECampaignState::Prologue: State = ECampaignState::IshibashiriApproach; break;
    case ECampaignState::Interlude1:
        if (bIshibashiriDemo)
        {
            State = ECampaignState::Title;
            if (!bIshibashiriDemoCompleteLogged)
            {
                bIshibashiriDemoCompleteLogged = true;
                UE_LOG(LogTemp, Display, TEXT("ISHIBASHIRI_DEMO_COMPLETE"));
            }
        }
        else State = ECampaignState::Fuchimatoi;
        break;
    case ECampaignState::Interlude2: State = ECampaignState::Minedaki; break;
    case ECampaignState::Interlude3: State = ECampaignState::Magatsune; break;
    case ECampaignState::Ending: State = ECampaignState::Completed; break;
    case ECampaignState::Completed: State = ECampaignState::Title; break;
    default: return false;
    }
    ChapterStartSeconds = FPlatformTime::Seconds();
    if (State == ECampaignState::Completed) ClearChapterSave();
    else if (State != ECampaignState::Title && !bStartingFromTitle) SaveChapter();
    UE_LOG(LogTemp, Display, TEXT("CAMPAIGN_E2E %s chapter_start_time=%.3f"), CampaignStateName(State), GetCampaignElapsedSeconds());
    return true;
}

int32 UCampaignGameInstance::GetLastCardIndex() const
{
    if (State == ECampaignState::Prologue || State == ECampaignState::Ending) return 3;
    if (State == ECampaignState::Interlude1) return bIshibashiriDemo ? 1 : 2;
    return State == ECampaignState::Interlude3 ? 2 : 1;
}

bool UCampaignGameInstance::CompleteApproach()
{
    if (!bCampaignActive || State != ECampaignState::IshibashiriApproach) return false;
    State = ECampaignState::Ishibashiri;
    ChapterStartSeconds = FPlatformTime::Seconds();
    SaveChapter();
    UE_LOG(LogTemp, Display, TEXT("CAMPAIGN_E2E IshibashiriApproach Completed approach_clear_time=%.3f"), GetCampaignElapsedSeconds());
    UE_LOG(LogTemp, Display, TEXT("CAMPAIGN_E2E Ishibashiri Started chapter_start_time=%.3f"), GetCampaignElapsedSeconds());
    return true;
}

bool UCampaignGameInstance::CompleteEncounter(ECampaignState Encounter)
{
    if (!bCampaignActive || State != Encounter || !IsEncounterState(State)) return false;
    switch (State)
    {
    case ECampaignState::Ishibashiri: State = ECampaignState::Interlude1; break;
    case ECampaignState::Fuchimatoi: State = ECampaignState::Interlude2; break;
    case ECampaignState::Minedaki: State = ECampaignState::Interlude3; break;
    case ECampaignState::Magatsune: State = ECampaignState::Ending; break;
    default: return false;
    }
    const double Now = FPlatformTime::Seconds();
    UE_LOG(LogTemp, Display, TEXT("CAMPAIGN_E2E %s Completed encounter_clear_time=%.3f"), CampaignStateName(Encounter),
        Now - ChapterStartSeconds);
    ChapterStartSeconds = Now;
    SaveChapter();
    UE_LOG(LogTemp, Display, TEXT("CAMPAIGN_E2E %s chapter_start_time=%.3f"), CampaignStateName(State), GetCampaignElapsedSeconds());
    return true;
}

void UCampaignGameInstance::RestartCampaign()
{
    bCampaignActive = true;
    State = ECampaignState::Title;
    CampaignStartSeconds = ChapterStartSeconds = FPlatformTime::Seconds();
    UE_LOG(LogTemp, Display, TEXT("CAMPAIGN_E2E Title chapter_start_time=0.000"));
}

bool UCampaignGameInstance::IsResumableChapter(ECampaignState Chapter)
{
    switch (Chapter)
    {
    case ECampaignState::Prologue:
    case ECampaignState::IshibashiriApproach:
    case ECampaignState::Ishibashiri:
    case ECampaignState::Interlude1:
    case ECampaignState::Fuchimatoi:
    case ECampaignState::Interlude2:
    case ECampaignState::Minedaki:
    case ECampaignState::Interlude3:
    case ECampaignState::Magatsune:
    case ECampaignState::Ending: return true;
    default: return false;
    }
}

void UCampaignGameInstance::LoadChapterSave()
{
    bHasContinue = false;
    ContinueChapter = ECampaignState::Title;
    const UCampaignSaveGame* Save = Cast<UCampaignSaveGame>(UGameplayStatics::LoadGameFromSlot(CampaignSaveSlot, 0));
    if (!Save || Save->Version != UCampaignSaveGame::CurrentVersion || !IsResumableChapter(Save->Chapter)) return;
    bHasContinue = true;
    ContinueChapter = Save->Chapter;
}

void UCampaignGameInstance::SaveChapter()
{
    if (!bPersistenceEnabled || !bCampaignActive || !IsResumableChapter(State)) return;
    UCampaignSaveGame* Save = Cast<UCampaignSaveGame>(UGameplayStatics::CreateSaveGameObject(UCampaignSaveGame::StaticClass()));
    if (!Save) return;
    Save->Chapter = State;
    if (!UGameplayStatics::SaveGameToSlot(Save, CampaignSaveSlot, 0))
    {
        UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN_SAVE_FAILED chapter=%d"), static_cast<int32>(State));
        return;
    }
    bHasContinue = true;
    ContinueChapter = State;
    UE_LOG(LogTemp, Display, TEXT("CAMPAIGN_SAVED chapter=%d"), static_cast<int32>(State));
}

void UCampaignGameInstance::ClearChapterSave()
{
    if (!bPersistenceEnabled) return;
    if (!UGameplayStatics::DeleteGameInSlot(CampaignSaveSlot, 0) && UGameplayStatics::DoesSaveGameExist(CampaignSaveSlot, 0))
    {
        UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN_SAVE_CLEAR_FAILED"));
        LoadChapterSave();
        return;
    }
    bHasContinue = false;
    ContinueChapter = ECampaignState::Title;
}

bool UCampaignGameInstance::ContinueCampaign()
{
    if (State != ECampaignState::Title || !bHasContinue || !IsResumableChapter(ContinueChapter)) return false;
    bCampaignActive = true;
    State = ContinueChapter;
    CampaignStartSeconds = ChapterStartSeconds = FPlatformTime::Seconds();
    UE_LOG(LogTemp, Display, TEXT("CAMPAIGN_CONTINUE chapter=%d"), static_cast<int32>(State));
    return true;
}

double UCampaignGameInstance::GetCampaignElapsedSeconds() const { return FPlatformTime::Seconds() - CampaignStartSeconds; }

bool UCampaignGameInstance::IsEncounterState(ECampaignState Value)
{
    return Value == ECampaignState::Ishibashiri || Value == ECampaignState::Fuchimatoi || Value == ECampaignState::Minedaki ||
        Value == ECampaignState::Magatsune;
}

bool UCampaignGameInstance::TryGetCorruptionStageForChapter(ECampaignState Chapter, EShirotsuraCorruptionStage& OutStage)
{
    switch (Chapter)
    {
    case ECampaignState::Prologue:
    case ECampaignState::IshibashiriApproach:
    case ECampaignState::Ishibashiri:
    case ECampaignState::Interlude1:
    case ECampaignState::Fuchimatoi: OutStage = EShirotsuraCorruptionStage::Early; return true;
    case ECampaignState::Interlude2:
    case ECampaignState::Minedaki:
    case ECampaignState::Interlude3:
    case ECampaignState::Magatsune:
    case ECampaignState::Ending: OutStage = EShirotsuraCorruptionStage::Advanced; return true;
    case ECampaignState::Title:
    case ECampaignState::Completed:
    default: return false;
    }
}

bool UCampaignGameInstance::GetCorruptionStageForEncounter(ECampaignState StandaloneEncounter, EShirotsuraCorruptionStage& OutStage) const
{
    // A standalone launch also begins with State == Title, so campaign activity,
    // not that initial state, selects the fallback encounter default.
    return TryGetCorruptionStageForChapter(bCampaignActive ? State : StandaloneEncounter, OutStage);
}

bool UCampaignGameInstance::NotifyEncounterRetry(ECampaignState Encounter) const
{
    EShirotsuraCorruptionStage Stage;
    if (!GetCorruptionStageForEncounter(Encounter, Stage)) return false;
    UE_LOG(LogTemp, Verbose, TEXT("Shirotsura corruption stage retained on retry: %s"),
        Stage == EShirotsuraCorruptionStage::Early ? TEXT("Early") : TEXT("Advanced"));
    return true;
}

void UCampaignGameInstance::ResetSenseState(UPlayerSenseComponent* Sense)
{
    if (Sense) Sense->ResetSense();
}

void UCampaignGameInstance::ResetChapterRuntime(UObject* ChapterWorldContext)
{
    if (!ChapterWorldContext || !ChapterWorldContext->GetWorld()) return;
    for (TActorIterator<APawn> It(ChapterWorldContext->GetWorld()); It; ++It)
        ResetSenseState(It->FindComponentByClass<UPlayerSenseComponent>());
}

void UCampaignGameInstance::TravelToCurrentChapter(UObject* ChapterWorldContext)
{
    ResetChapterRuntime(ChapterWorldContext);
    const FString Options = FString::Printf(TEXT("game=%s"), GameModeFor(State));
    UGameplayStatics::OpenLevel(ChapterWorldContext, CampaignMap, true, Options);
}
