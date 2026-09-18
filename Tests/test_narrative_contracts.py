"""Run with unittest (stdlib) or pytest. Mutations are in-memory, never source edits."""
import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "Tools"))
from ValidateNarrativeContracts import audit_contract, audit_sources, hide_noncode, validate


class NarrativeContractsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.contract = json.loads((ROOT / "Docs/Design/NarrativeContract.json").read_text(encoding="utf-8"))
        cls.sources = {
            path.relative_to(ROOT).as_posix(): path.read_text(encoding="utf-8")
            for path in (ROOT / "Source/IshibashiriPrototype").rglob("*")
            if path.is_file() and path.suffix in {".h", ".cpp"}
        }

    def changed(self, filename, source):
        files = dict(self.sources)
        files["Source/IshibashiriPrototype/Private/" + filename] = source
        return audit_sources(files, self.contract)

    def test_current_repository_passes(self):
        self.assertEqual(validate(ROOT), [])

    def test_third_appearance_fails(self):
        contract = copy.deepcopy(self.contract)
        contract["appearance_variants"].append("Final")
        self.assertTrue(audit_contract(contract))

    def test_switch_ending_and_mask_invariants(self):
        for key, value in [("switch_on_chapter_entry", "Interlude3"), ("mask_is_sealing_device", True),
                           ("corruption_stage_changes_gameplay_stats", True), ("retry_preserves_appearance", False)]:
            with self.subTest(key=key):
                contract = copy.deepcopy(self.contract)
                contract[key] = value
                self.assertTrue(audit_contract(contract))
        contract = copy.deepcopy(self.contract)
        contract["chapter_appearance"]["Ending"] = "Early"
        self.assertTrue(audit_contract(contract))

    def test_magatsune_is_not_fourth_nushi(self):
        contract = copy.deepcopy(self.contract)
        contract["encounters"][3]["kind"] = "nushi"
        self.assertTrue(audit_contract(contract))

    def test_unknown_production_calm_call_fails(self):
        issues = self.changed("ExtraBoss.cpp", "void AExtraBoss::Counter() { State->CalmNushi(); }")
        self.assertTrue(any("CALM_CALL" in issue for issue in issues))

    def test_header_inline_bypass_fails(self):
        sources = dict(self.sources)
        sources["Source/IshibashiriPrototype/Public/ExtraBoss.h"] = "void Counter() { State->CalmNushi(); }"
        self.assertTrue(any("CALM_CALL" in issue for issue in audit_sources(sources, self.contract)))

    def test_allowlisted_filename_is_not_blanket_permission(self):
        path = "Source/IshibashiriPrototype/Private/NushiStateComponent.cpp"
        issues = self.changed("NushiStateComponent.cpp", self.sources[path] + "\nvoid UNushiStateComponent::Cheat() { CalmNushi(); }\n")
        self.assertTrue(any("CALM_CALL" in issue for issue in issues))

    def test_direct_completion_and_victory_fail(self):
        for statement, rule in [("OnEncounterCompleted.Broadcast();", "COMPLETION_CALL"),
                                ("Mode->FinishEncounter(true);", "VICTORY_CALL")]:
            with self.subTest(rule=rule):
                issues = self.changed("ExtraBoss.cpp", "void AExtraBoss::Counter() { " + statement + " }")
                self.assertTrue(any(rule in issue for issue in issues))

    def test_ground_health_path_fails(self):
        path = "Source/IshibashiriPrototype/Private/IshibashiriBoss.cpp"
        mutated = self.sources[path].replace("Posture = FMath::Max(0, Posture - 1);", "Health = FMath::Max(0, Health - 1);")
        self.assertNotEqual(mutated, self.sources[path])
        self.assertTrue(any("COUNTER:" in issue for issue in self.changed("IshibashiriBoss.cpp", mutated)))

    def test_sense_cannot_purify(self):
        path = "Source/IshibashiriPrototype/Private/PlayerSenseComponent.cpp"
        issues = self.changed("PlayerSenseComponent.cpp", self.sources[path] + "\nvoid UPlayerSenseComponent::Cheat() { Target->Purify(); }")
        self.assertTrue(any("SENSE:" in issue for issue in issues))

    def test_comments_strings_and_fixture_do_not_trigger_calls(self):
        text = 'void AExtra::Note() { /* CalmNushi(); */ const char* s = "OnEncounterCompleted.Broadcast()"; // FinishEncounter(true);\n }'
        self.assertEqual(self.changed("Extra.cpp", text), [])
        self.assertEqual(self.changed("ExtraTests.cpp", "void Test() { State->CalmNushi(); }"), [])
        self.assertEqual(len(hide_noncode(text)), len(text))

    def test_missing_source_fails_closed(self):
        sources = dict(self.sources)
        del sources["Source/IshibashiriPrototype/Private/PrototypePlayer.cpp"]
        self.assertTrue(any("MISSING_SOURCE" in issue for issue in audit_sources(sources, self.contract)))

    def test_retry_missing_sense_reset_fails(self):
        path = "Source/IshibashiriPrototype/Private/PrototypePlayer.cpp"
        issues = self.changed("PrototypePlayer.cpp", self.sources[path].replace("Sense->ResetSense();", ""))
        self.assertTrue(any("RESET:" in issue for issue in issues))

    def test_progress_guard_removal_fails(self):
        path = "Source/IshibashiriPrototype/Private/NushiProgressComponent.cpp"
        mutated = self.sources[path].replace("PurifiedKakon.Num() == RegisteredKakon.Num()", "PurifiedKakon.Num() > 0")
        self.assertTrue(any("PROGRESS:" in issue for issue in self.changed("NushiProgressComponent.cpp", mutated)))

    def test_campaign_transition_regression_fails(self):
        path = "Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp"
        mutated = self.sources[path].replace("case ECampaignState::Fuchimatoi: State = ECampaignState::Interlude2;", "case ECampaignState::Fuchimatoi: State = ECampaignState::Ending;")
        self.assertTrue(any("CAMPAIGN:" in issue for issue in self.changed("CampaignGameInstance.cpp", mutated)))

    def test_corruption_stage_mapping_regression_fails(self):
        path = "Source/IshibashiriPrototype/Private/CampaignGameInstance.cpp"
        mutated = self.sources[path].replace("case ECampaignState::Interlude2:", "case ECampaignState::Title:")
        self.assertTrue(any("APPEARANCE:" in issue for issue in self.changed("CampaignGameInstance.cpp", mutated)))

    def test_story_card_and_retry_route_regressions_fail(self):
        path = "Source/IshibashiriPrototype/Private/CampaignHUD.cpp"
        mutated = self.sources[path].replace("白い面は、禍祓いのしるし。", "白面の祓い手。")
        self.assertTrue(any("STORY_CARD:" in issue for issue in self.changed("CampaignHUD.cpp", mutated)))
        path = "Source/IshibashiriPrototype/Private/MinedakiGameMode.cpp"
        mutated = self.sources[path].replace("Campaign->NotifyEncounterRetry(ECampaignState::Minedaki);", "")
        self.assertTrue(any("APPEARANCE:" in issue for issue in self.changed("MinedakiGameMode.cpp", mutated)))

    def test_display_scope_does_not_ban_player_death_or_dead_trees(self):
        self.assertEqual(self.changed("ExtraPlayer.cpp", 'void AExtraPlayer::Text() { TEXT("Player dead"); TEXT("DeadTree"); }'), [])
        self.assertTrue(any("DISPLAY:" in issue for issue in self.changed("ExtraHUD.cpp", 'void AExtraHUD::Text() { TEXT("Boss HP -1"); }')))
        self.assertEqual(self.changed("ExtraHUD.cpp", '// TEXT("Boss HP -1");'), [])


if __name__ == "__main__":
    unittest.main()
