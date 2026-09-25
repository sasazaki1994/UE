from pathlib import Path


ROOT = Path(__file__).parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def test_demo_flag_and_runner_are_explicit():
    instance = read("Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp")
    runner = read("Tools/Prototype.ps1")
    assert 'TEXT("IshibashiriDemo")' in instance
    assert "[switch]$IshibashiriDemo" in runner
    assert "'-IshibashiriDemo'" in runner
    assert "ISHIBASHIRI_DEMO_E2E_PASS" in runner


def test_demo_persistence_is_disabled_before_any_load():
    source = read("Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp")
    init = source[source.index("void UCampaignGameInstance::Init()") : source.index("void UCampaignGameInstance::StartCampaign()")]
    assert "!bIshibashiriDemo" in init
    assert init.index("bPersistenceEnabled =") < init.index("LoadChapterSave()")
    assert "if (bCampaignActive && bPersistenceEnabled) LoadChapterSave();" in init
    assert "if (!bPersistenceEnabled" in source
    assert "if (!bPersistenceEnabled) return;" in source


def test_demo_short_ending_returns_to_title_without_fuchimatoi():
    instance = read("Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp")
    advance = instance.index("bool UCampaignGameInstance::AdvanceCardChapter()")
    interlude_start = instance.index("case ECampaignState::Interlude1:", advance)
    interlude = instance[interlude_start : instance.index("case ECampaignState::Interlude2:", interlude_start)]
    assert "if (bIshibashiriDemo)" in interlude
    assert "State = ECampaignState::Title" in interlude
    assert "else State = ECampaignState::Fuchimatoi" in interlude
    assert interlude.index("State = ECampaignState::Title") < interlude.index("ISHIBASHIRI_DEMO_COMPLETE")


def test_demo_has_two_interlude_cards_while_campaign_has_three():
    instance = read("Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp")
    hud = read("Source/IshibashiriPrototype/Private/CampaignHUD.cpp")
    assert "return bIshibashiriDemo ? 1 : 2" in instance
    for card in (
        "石走りは生きている。息が戻る。",
        "水の底で、同じ脈動が続いている。",
        "第二の主　淵纏い",
    ):
        assert card in hud


def test_save_slot_version_and_ishibashiri_gameplay_contract_remain_unchanged():
    instance = read("Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp")
    save = read("Source/IshibashiriPrototype/Public/CampaignSaveGame.h")
    gameplay_contract = read("Tools/GameplayContract.py")
    assert 'CampaignSaveSlot(TEXT("MagabaraiCampaign"))' in instance
    assert "CurrentVersion = 1" in save
    assert "Ishibashiri" in gameplay_contract


def test_completion_marker_exists_only_at_demo_interlude_exit():
    sources = "\n".join(
        path.read_text(encoding="utf-8")
        for path in (ROOT / "Source").rglob("*.cpp")
    )
    assert sources.count('TEXT("ISHIBASHIRI_DEMO_COMPLETE")') == 1
    instance = read("Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp")
    assert "bIshibashiriDemoCompleteLogged" in instance
    assert "case ECampaignState::Interlude1:" in instance
