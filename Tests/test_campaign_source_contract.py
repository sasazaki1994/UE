from pathlib import Path

ROOT = Path(__file__).parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def test_campaign_has_fixed_minimal_state_order():
    header = read("Source/IshibashiriPrototype/Public/CampaignGameInstance.h")
    states = ["Title", "Prologue", "Ishibashiri", "Interlude1", "Fuchimatoi", "Interlude2", "Minedaki", "Interlude3", "Magatsune", "Ending", "Completed"]
    positions = [header.index(state) for state in states]
    assert positions == sorted(positions)
    assert "SaveGame" not in header


def test_campaign_completion_is_connected_to_all_existing_managers():
    for encounter in ("Prototype", "Fuchimatoi", "Minedaki", "Magatsune"):
        source = read(f"Source/IshibashiriPrototype/Private/{encounter}GameMode.cpp")
        assert "OnEncounterCompleted.AddUniqueDynamic" in source
        assert "CompleteEncounter" in source


def test_travel_resets_sense_and_uses_existing_game_modes():
    source = read("Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp")
    assert "ResetSense" in source
    assert "OpenLevel" in source
    for mode in ("PrototypeGameMode", "FuchimatoiGameMode", "MinedakiGameMode", "MagatsuneGameMode"):
        assert mode in source


def test_campaign_launcher_and_documentation_are_present():
    script = read("Tools/Prototype.ps1")
    assert "[switch]$Campaign" in script
    assert "CampaignGameMode" in script
    assert "Automation RunTests IshibashiriPrototype.Campaign" in script
    assert "-Action Play -Campaign" in read("README.md")
