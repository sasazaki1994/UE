from pathlib import Path

ROOT = Path(__file__).parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def test_campaign_has_fixed_minimal_state_order():
    header = read("Source/IshibashiriPrototype/Public/CampaignGameInstance.h")
    states = ["Title", "Prologue", "IshibashiriApproach", "Ishibashiri", "Interlude1", "Fuchimatoi", "Interlude2", "Minedaki", "Interlude3", "Magatsune", "Ending", "Completed"]
    positions = [header.index(state) for state in states]
    assert positions == sorted(positions)
    # Save only the chapter boundary; the state machine still owns no boss progress.
    save = read("Source/IshibashiriPrototype/Public/CampaignSaveGame.h")
    assert "ECampaignState Chapter" in save
    assert not any(field in save for field in ("KakonCount", "BossPhase", "Stamina", "RetryCount"))


def test_continue_is_title_only_and_input_tests_do_not_write_player_saves():
    source = read("Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp")
    controller = read("Source/IshibashiriPrototype/Private/CampaignPlayerController.cpp")
    assert "State != ECampaignState::Title" in source
    assert "PrototypeTestRun=" in source and "bPersistenceEnabled" in source
    assert "LoadGameFromSlot" in source and "SaveGameToSlot" in source
    assert 'BindAction(TEXT("Retry")' in controller



def test_new_game_overwrite_requires_confirmation_without_blocking_continue():
    mode = read("Source/IshibashiriPrototype/Private/CampaignGameMode.cpp")
    header = read("Source/IshibashiriPrototype/Public/CampaignGameMode.h")
    controller = read("Source/IshibashiriPrototype/Private/CampaignPlayerController.cpp")
    hud = read("Source/IshibashiriPrototype/Private/CampaignHUD.cpp")
    assert "State == ECampaignState::Title && !NewGameConfirmation.RequestStart(Campaign->HasContinue())" in mode
    assert "!NewGameConfirmation.RequestStart(Campaign->HasContinue())" in mode
    assert mode.index("NewGameConfirmation.RequestStart") < mode.index("Campaign->AdvanceCardChapter()")
    assert "ContinueCampaign()" in mode and "NewGameConfirmation.Cancel()" in mode
    assert "FCampaignNewGameConfirmation NewGameConfirmation" in header
    assert "Escape" in controller and "Gamepad_FaceButton_Right" in controller
    assert "CancelNewGameConfirmation" in controller
    assert "新しく始めると前回の続きが上書きされます" in hud
    assert "ESC / B : CANCEL" in hud

def test_campaign_completion_is_connected_to_all_existing_managers():
    for encounter in ("Prototype", "Fuchimatoi", "Minedaki", "Magatsune"):
        source = read(f"Source/IshibashiriPrototype/Private/{encounter}GameMode.cpp")
        assert "OnEncounterCompleted.AddUniqueDynamic" in source
        assert "CompleteEncounter" in source


def test_travel_resets_sense_and_uses_existing_game_modes():
    source = read("Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp")
    assert "ResetSense" in source
    assert "OpenLevel" in source
    for mode in ("IshibashiriApproachGameMode", "PrototypeGameMode", "FuchimatoiGameMode", "MinedakiGameMode", "MagatsuneGameMode"):
        assert mode in source


def test_corruption_stage_is_derived_and_retry_routes_use_it():
    header = read("Source/IshibashiriPrototype/Public/CampaignGameInstance.h")
    source = read("Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp")
    assert "EShirotsuraCorruptionStage" in header
    assert "TryGetCorruptionStageForChapter" in header
    assert "bCampaignActive ? State : StandaloneEncounter" in source
    for mode, encounter in (("Prototype", "Ishibashiri"), ("Fuchimatoi", "Fuchimatoi"),
                            ("Minedaki", "Minedaki"), ("Magatsune", "Magatsune")):
        retry = read(f"Source/IshibashiriPrototype/Private/{mode}GameMode.cpp")
        assert f"NotifyEncounterRetry(ECampaignState::{encounter})" in retry


def test_campaign_launcher_and_documentation_are_present():
    script = read("Tools/Prototype.ps1")
    assert "[switch]$Campaign" in script
    assert "CampaignGameMode" in script
    assert "-CampaignE2E" in script
    assert "CAMPAIGN_E2E_PASS" in script
    assert "-Action Play -Campaign" in read("README.md")
