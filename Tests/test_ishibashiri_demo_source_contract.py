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


def test_demo_ground_driver_baits_from_player_side_in_the_basin():
    # The Demo fights in the basin, whose spawns put the boss on a chase path
    # through the world origin; the driver must not use the origin as a reference.
    driver = read("Source/IshibashiriPrototype/Private/ClimbingIntegrationTest.cpp")
    start = driver.index("bool AClimbingIntegrationTest::DriveMountWindow(float Dt)")
    body = driver[start : driver.index("void AClimbingIntegrationTest::SetupGrab", start)]
    assert "(P->GetActorLocation()-B->GetActorLocation()).GetSafeNormal2D()*850.f" in body
    assert "(-B->GetActorLocation())" not in body
    health_guard = body.index("P->GetHealth()!=P->MaxHealth || !Mode->IsEncounterActive()")
    assert health_guard < body.index("const EIshibashiriState State=B->GetState();")
    assert "Ground dodge and counter route preserves health and encounter" in body
    assert "Tap(EKeys::LeftShift)" in body and "Tap(EKeys::LeftMouseButton)" in body
    for authority in ("TryReceiveCounter", "SetHealth", "Posture=", "Purify", "ResetForEncounter"):
        assert authority not in body
    instance = read("Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp")
    assert 'if (bIshibashiriDemo && State == ECampaignState::Ishibashiri) Options += TEXT("?BasinPrototype");' in instance


def test_first_playtest_keeps_human_evidence_separate_from_automation():
    acceptance = read("Specs/Acceptance/IshibashiriDemo.feature")
    protocol = read("Docs/IshibashiriFirstPlaytest.md")
    assert "初見プレイは自動E2Eと分離して評価する" in acceptance
    assert "初見プレイをNOT_RUN" in acceptance
    assert "Status: **READY / HUMAN PLAYTEST NOT_RUN**" in protocol
    assert "Source SHA / Package / Package SHA-256:" in protocol
    assert "Completed without coaching: YES / NO / NOT_RUN" in protocol
    assert "人間の記録が作成されるまでは結果を **NOT_RUN**" in protocol
    assert "ValidateIshibashiriFirstPlaytest.py" in protocol
