from pathlib import Path
import re

ROOT = Path(__file__).parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def function_body(source: str, signature: str) -> str:
    """Return one C++ function body so contracts do not match unrelated code."""
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[opening + 1:index]
    raise AssertionError(f"unterminated function: {signature}")


def test_save_game_contains_only_version_and_chapter():
    save = read("Source/IshibashiriPrototype/Public/CampaignSaveGame.h")
    saved_fields = re.findall(r"UPROPERTY\(SaveGame\)\s+[^;]+\s+(\w+)\s*=", save)
    assert saved_fields == ["Version", "Chapter"]
    assert "static constexpr int32 CurrentVersion = 1;" in save
    assert "int32 Version = CurrentVersion;" in save


def test_save_slot_and_version_are_written_from_single_source_of_truth():
    source = read("Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp")
    assert source.count('CampaignSaveSlot(TEXT("MagabaraiCampaign"))') == 1
    save = function_body(source, "void UCampaignGameInstance::SaveChapter()")
    assert "Save->Version = UCampaignSaveGame::CurrentVersion;" in save
    assert "SaveGameToSlot(Save, CampaignSaveSlot, 0)" in save


def test_load_rejects_missing_wrong_type_version_and_non_resumable_chapter():
    source = read("Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp")
    load = function_body(source, "void UCampaignGameInstance::LoadChapterSave()")
    assert "bHasContinue = false;" in load
    assert "ContinueChapter = ECampaignState::Title;" in load
    assert "Cast<UCampaignSaveGame>(UGameplayStatics::LoadGameFromSlot(CampaignSaveSlot, 0))" in load
    assert "!Save || Save->Version != UCampaignSaveGame::CurrentVersion || !IsResumableChapter(Save->Chapter)" in load
    assert load.index("if (!Save") < load.index("bHasContinue = true;") < load.index("ContinueChapter = Save->Chapter;")


def test_resumable_chapter_allowlist_is_exact():
    source = read("Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp")
    body = function_body(source, "bool UCampaignGameInstance::IsResumableChapter(ECampaignState Chapter)")
    allowed = ["Prologue", "IshibashiriApproach", "Ishibashiri", "Interlude1", "Fuchimatoi",
               "Interlude2", "Minedaki", "Interlude3", "Magatsune", "Ending"]
    cases = re.findall(r"case ECampaignState::(\w+):", body)
    assert cases == allowed
    assert "default: return false;" in body
    assert "Title" not in cases and "Completed" not in cases


def test_continue_is_title_only_and_uses_only_saved_chapter():
    source = read("Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp")
    body = function_body(source, "bool UCampaignGameInstance::ContinueCampaign()")
    guard = "State != ECampaignState::Title || !bHasContinue || !IsResumableChapter(ContinueChapter)"
    assert guard in body
    assert body.index(guard) < body.index("bCampaignActive = true;")
    assert "State = ContinueChapter;" in body
    assert not any(term in body for term in ("Kakon", "Boss", "Stamina", "Grab", "Cling", "Sense", "Camera"))


def test_save_chapter_only_persists_active_resumable_state():
    source = read("Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp")
    body = function_body(source, "void UCampaignGameInstance::SaveChapter()")
    assert "!bPersistenceEnabled || !bCampaignActive || !IsResumableChapter(State)" in body
    assert body.index("!IsResumableChapter(State)") < body.index("CreateSaveGameObject")
    assert "Save->Chapter = State;" in body


def test_completed_clears_save_and_delete_failure_is_fail_safe():
    source = read("Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp")
    advance = function_body(source, "bool UCampaignGameInstance::AdvanceCardChapter()")
    clear = function_body(source, "void UCampaignGameInstance::ClearChapterSave()")
    assert "case ECampaignState::Ending: State = ECampaignState::Completed;" in advance
    assert "if (State == ECampaignState::Completed) ClearChapterSave();" in advance
    assert "!UGameplayStatics::DeleteGameInSlot(CampaignSaveSlot, 0)" in clear
    assert "&& UGameplayStatics::DoesSaveGameExist(CampaignSaveSlot, 0)" in clear
    failure = clear.index('UE_LOG(LogTemp, Warning, TEXT("CAMPAIGN_SAVE_CLEAR_FAILED"))')
    assert failure < clear.index("LoadChapterSave();") < clear.index("return;", failure)
    assert clear.index("return;", failure) < clear.index("bHasContinue = false;")
    assert "ContinueChapter = ECampaignState::Title;" in clear


def test_init_disables_test_persistence_before_loading_save():
    source = read("Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp")
    init = function_body(source, "void UCampaignGameInstance::Init()")
    for switch in ("IshibashiriDemo", "CampaignE2E", "PrototypeTestRun="):
        assert switch in init
    assignment = init.index("bPersistenceEnabled =")
    load = init.index("LoadChapterSave()")
    assert assignment < load
    assert "if (bCampaignActive && bPersistenceEnabled) LoadChapterSave();" in init
    save = function_body(source, "void UCampaignGameInstance::SaveChapter()")
    clear = function_body(source, "void UCampaignGameInstance::ClearChapterSave()")
    assert save.lstrip().startswith("if (!bPersistenceEnabled")
    assert clear.lstrip().startswith("if (!bPersistenceEnabled)")


def test_new_game_confirmation_cancel_and_continue_contract():
    mode = read("Source/IshibashiriPrototype/Private/CampaignGameMode.cpp")
    header = read("Source/IshibashiriPrototype/Public/CampaignGameMode.h")
    confirm = function_body(mode, "void ACampaignGameMode::ConfirmCard()")
    continuing = function_body(mode, "void ACampaignGameMode::ContinueSavedCampaign()")
    cancel = function_body(mode, "void ACampaignGameMode::CancelNewGameConfirmation()")
    request = function_body(header, "bool RequestStart(bool bHasContinue)")
    assert "if (bHasContinue && !bPending)" in request
    assert "bPending = true;" in request and "return false;" in request and "return true;" in request
    assert "NewGameConfirmation.RequestStart" in confirm
    assert confirm.index("NewGameConfirmation.RequestStart") < confirm.index("Campaign->AdvanceCardChapter()")
    assert "Campaign->ContinueCampaign()" in continuing
    assert continuing.index("Campaign->ContinueCampaign()") < continuing.index("NewGameConfirmation.Cancel()")
    assert "NewGameConfirmation.Cancel();" in cancel
