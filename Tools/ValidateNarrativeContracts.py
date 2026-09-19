"""Narrow, dependency-free source-contract lint; never claims UE runtime validation."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
PREFIX = "Source/IshibashiriPrototype/"
CHAPTERS = [
    "Title", "Prologue", "IshibashiriApproach", "Ishibashiri", "Interlude1",
    "Fuchimatoi", "Interlude2", "Minedaki", "Interlude3", "Magatsune", "Ending", "Completed",
]
APPEARANCE = dict(zip(CHAPTERS, [None] + ["Early"] * 5 + ["Advanced"] * 5 + [None]))
ENCOUNTERS = ["Ishibashiri", "Fuchimatoi", "Minedaki", "Magatsune"]
PLAYERS = ["PrototypePlayer", "FuchimatoiPlayer", "MinedakiPlayer", "MagatsunePlayer"]
STORY_CARDS = {
    "Prologue": ["白い面は、禍祓いのしるし。", "境界石が砕け、白面の手と顔にも禍が残った。", "自らの穢れを祓う手掛かりを求め、主のもとへ。", "主を討つな。宿った禍だけを祓え。"],
    "Interlude1": ["石走りの息が、ゆっくりと戻る。", "水の底で、同じ脈動が続いている。", "第二の主　淵纏い"],
    "Interlude2": ["腕の痕が広がり、顔の痕も深くなっていた。", "第三の主　峰抱き"],
    "Interlude3": ["三柱は鎮まった。顔と腕の痕は消えない。", "禍の流れは、島の奥へ続いている。", "禍津根"],
    "Ending": ["地の脈動が静まり、主たちの息が戻る。", "面を外す。顔と腕の痕は、まだそこにある。", "白面は、再び面を着けた。", "禍がまた生じても、ここで向き合う。"],
}
REQUIRED = [PREFIX + "Private/" + name + ".cpp" for name in [
    "IshibashiriBoss", "PrototypeGameMode", "NushiStateComponent", "NushiProgressComponent",
    "NushiEncounterManager", "KakonActor", "PlayerSenseComponent", "CampaignGameInstance",
    "CampaignHUD", *PLAYERS,
]] + [PREFIX + "Public/CampaignGameInstance.h"]
# This intentionally is not a C++ parser. Preserve offsets when hiding comments/strings.
TOKENS = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')


def hide_noncode(text: str, *, keep_strings: bool = False) -> str:
    def hide(match: re.Match) -> str:
        token = match.group()
        if keep_strings and not token.startswith(("//", "/*")):
            return token
        return "".join("\n" if char == "\n" else " " for char in token)
    return TOKENS.sub(hide, text)


def function_span(code: str, qualified: str) -> tuple[int, int] | None:
    match = re.search(r"\b" + re.escape(qualified) + r"\s*\([^)]*\)\s*(?:const\s*)?\{", code)
    if not match:
        return None
    start, depth = match.end(), 1
    for pos in range(start, len(code)):
        depth += (code[pos] == "{") - (code[pos] == "}")
        if depth == 0:
            return start, pos
    return None


def body(code: str, qualified: str) -> str:
    span = function_span(code, qualified)
    return code[span[0]:span[1]] if span else ""


def audit_contract(contract: dict) -> list[str]:
    issues = []
    expected = {
        "schema_version": 1, "status": "PARTIALLY_IMPLEMENTED",
        "mask_is_sealing_device": False,
        "corruption_is_absorbed_from_purified_nushi": False,
        "corruption_stage_changes_gameplay_stats": False,
        "appearance_variants": ["Early", "Advanced"],
        "shared_assets": ["mesh", "rig", "costume", "mask", "animations"],
        "switch_on_chapter_entry": "Interlude2",
        "retry_preserves_appearance": True, "new_campaign_appearance": "Early",
        "chapter_appearance": APPEARANCE,
        "standalone_appearance": {name: APPEARANCE[name] for name in ENCOUNTERS},
        "normal_victory_condition": "all_registered_kakon_purified",
        "ending": {"nushi_survive": True, "magatsune_destroyed": False,
                   "corruption_remains": True, "appearance": "Advanced"},
        "story_cards": STORY_CARDS,
        "encounters": [
            {"id": name, "kind": "living_terrain" if name == "Magatsune" else "nushi", "kakon_count": 3}
            for name in ENCOUNTERS
        ],
    }
    for key, value in expected.items():
        if key not in contract or contract[key] != value:
            issues.append("CONTRACT: unexpected or missing " + key)
    delivery = contract.get("delivery", {})
    if (not isinstance(delivery, dict)
            or delivery.get("story_cards") != "IMPLEMENTED_IN_CPP"
            or delivery.get("appearance_stage_management") != "IMPLEMENTED_IN_CPP"
            or delivery.get("appearance_material_connection") != "IMPLEMENTED_LEFT_ARM_ONLY"
            or delivery.get("appearance_runtime") != "IMPLEMENTED_SAFE_FALLBACK"
            or delivery.get("appearance_material_script") != "IMPLEMENTED_NOT_RUN"
            or delivery.get("appearance_material_assets") != "NOT_GENERATED"
            or delivery.get("ue_runtime_validation") != "NOT_RUN"):
        issues.append("CONTRACT: delivery implementation and validation status differs")
    return issues


def audit_sources(sources: dict[str, str], contract: dict) -> list[str]:
    issues = audit_contract(contract)
    for path in REQUIRED:
        if path not in sources:
            issues.append("MISSING_SOURCE: " + path)
    codes = {p: hide_noncode(t) for p, t in sources.items()}

    # Calls are permitted only in these specific bodies, never just by filename.
    gates = [
        (r"\bCalmNushi\s*\(", "NushiStateComponent", "UNushiStateComponent::HandleAllKakonPurified", "CALM_CALL"),
        (r"\bOnEncounterCompleted\s*\.\s*Broadcast\s*\(", "NushiEncounterManager", "ANushiEncounterManager::HandleNushiStateChanged", "COMPLETION_CALL"),
        (r"\bFinishEncounter\s*\(\s*true\s*\)", "PrototypeGameMode", "APrototypeGameMode::HandleEncounterCompleted", "VICTORY_CALL"),
    ]
    for pattern, filename, qualified, rule in gates:
        allowed_path = PREFIX + "Private/" + filename + ".cpp"
        for path, code in codes.items():
            if "test" in Path(path).stem.lower():
                continue  # State fixtures/drivers are not production gameplay.
            span = function_span(code, qualified) if path == allowed_path else None
            for match in re.finditer(pattern, code):
                pos = match.start()
                if rule == "CALM_CALL":
                    # The implementation and declaration are not call sites.
                    if path == allowed_path and re.search(r"\bvoid\s+UNushiStateComponent::\s*$", code[max(0, pos - 80):pos]):
                        continue
                    if path == PREFIX + "Public/NushiStateComponent.h" and re.search(r"\bvoid\s*$", code[max(0, pos - 20):pos]):
                        continue
                if span is None or not span[0] <= pos < span[1]:
                    line = code.count("\n", 0, pos) + 1
                    issues.append(f"{rule}: {path}:{line}")

    def get(filename: str, qualified: str) -> str:
        path = PREFIX + "Private/" + filename + ".cpp"
        result = body(codes.get(path, ""), qualified)
        if not result.strip():
            issues.append("MISSING_FUNCTION: " + qualified)
        return result

    counter = get("IshibashiriBoss", "AIshibashiriBoss::TryReceiveCounter")
    if re.search(r"\b(?:Health|MaxHealth|CalmNushi|FinishEncounter|OnEncounterCompleted)\b", counter):
        issues.append("COUNTER: body-health or direct completion path")
    for pattern in [r"\bCanBeCountered\s*\(", r"\bPosture\s*-\s*1\b", r"EIshibashiriState::Kneel"]:
        if not re.search(pattern, counter):
            issues.append("COUNTER: missing posture contract " + pattern)

    progress = get("NushiProgressComponent", "UNushiProgressComponent::HandleKakonPurified")
    if not re.search(r"PurifiedKakon\.Num\(\)\s*==\s*RegisteredKakon\.Num\(\)", progress) or "OnAllPurified.Broadcast()" not in progress:
        issues.append("PROGRESS: registered/all-purified guard changed; review required")
    state = get("NushiStateComponent", "UNushiStateComponent::HandleAllKakonPurified")
    if not re.search(r"\bCalmNushi\s*\(\s*\)", state):
        issues.append("PROGRESS: missing all-purified to calm connection")
    manager = get("NushiEncounterManager", "ANushiEncounterManager::HandleNushiStateChanged")
    if "ENushiState::Calm" not in manager or "OnEncounterCompleted.Broadcast()" not in manager:
        issues.append("PROGRESS: missing manager calm/completion connection")

    for path, code in codes.items():
        if "test" not in Path(path).stem.lower() and Path(path).stem == "PlayerSenseComponent":
            if re.search(r"\b(?:Purify|CalmNushi)\s*\(|OnEncounterCompleted\s*\.\s*Broadcast", code):
                issues.append("SENSE: sensing must not directly purify or complete encounters")

    for player in PLAYERS:
        reset = get(player, "A" + player + "::ResetForEncounter")
        if not re.search(r"Sense\s*->\s*ResetSense\s*\(", reset):
            issues.append("RESET: missing Sense reset in " + player)
    reset_sense = get("CampaignGameInstance", "UCampaignGameInstance::ResetSenseState")
    reset_chapter = get("CampaignGameInstance", "UCampaignGameInstance::ResetChapterRuntime")
    travel = get("CampaignGameInstance", "UCampaignGameInstance::TravelToCurrentChapter")
    if "ResetSense()" not in reset_sense or "ResetSenseState(" not in reset_chapter or "ResetChapterRuntime(" not in travel:
        issues.append("RESET: missing Campaign travel sense-reset chain")

    header = codes.get(PREFIX + "Public/CampaignGameInstance.h", "")
    enum = re.search(r"enum\s+class\s+ECampaignState\b[^\{]*\{([^}]+)\}", header)
    if not enum or re.findall(r"\b[A-Za-z_]\w*\b", enum.group(1)) != CHAPTERS:
        issues.append("CAMPAIGN: chapter enum differs from two-appearance mapping")
    complete = get("CampaignGameInstance", "UCampaignGameInstance::CompleteEncounter")
    advance = get("CampaignGameInstance", "UCampaignGameInstance::AdvanceCardChapter")
    for code, origin, destination in [(complete, "Fuchimatoi", "Interlude2"), (advance, "Interlude2", "Minedaki")]:
        if not re.search(r"case\s+ECampaignState::" + origin + r"\s*:\s*State\s*=\s*ECampaignState::" + destination + r"\s*;", code):
            issues.append("CAMPAIGN: expected transition " + origin + " -> " + destination)

    stage = get("CampaignGameInstance", "UCampaignGameInstance::TryGetCorruptionStageForChapter")
    effective_stage = get("CampaignGameInstance", "UCampaignGameInstance::GetCorruptionStageForEncounter")
    retry = get("CampaignGameInstance", "UCampaignGameInstance::NotifyEncounterRetry")
    for chapter, expected_stage in APPEARANCE.items():
        if expected_stage and not re.search(r"ECampaignState::" + chapter, stage):
            issues.append("APPEARANCE: missing chapter mapping " + chapter)
    if "bCampaignActive ? State : StandaloneEncounter" not in effective_stage:
        issues.append("APPEARANCE: standalone fallback must not infer mode from Title")
    if "GetCorruptionStageForEncounter(" not in retry:
        issues.append("APPEARANCE: retry does not retain the derived stage")
    for filename, encounter in zip(["Prototype", "Fuchimatoi", "Minedaki", "Magatsune"], ENCOUNTERS):
        reset = get(filename + "GameMode", "A" + filename + "GameMode::RetryEncounter")
        if "NotifyEncounterRetry(ECampaignState::" + encounter + ")" not in reset:
            issues.append("APPEARANCE: retry route missing stage retention hook for " + encounter)

    hud = sources.get(PREFIX + "Private/CampaignHUD.cpp", "")
    for chapter, cards in STORY_CARDS.items():
        for card in cards:
            if card not in hud:
                issues.append("STORY_CARD: missing " + chapter + ": " + card)

    # Inspect display strings only: not comments, documentation, Player death or DeadTree.
    forbidden = re.compile(r"\bBoss\s+HP\b|\b(?:NUSHI|COLOSSUS)\s+(?:KILLED|DEAD)\b|第四の主", re.IGNORECASE)
    for path, text in sources.items():
        stem = Path(path).stem
        if stem.endswith(("HUD", "Player")) and "test" not in stem.lower():
            for match in re.finditer(r'\bTEXT\s*\(\s*"((?:\\.|[^"\\])*)"\s*\)', hide_noncode(text, keep_strings=True)):
                if forbidden.search(match.group(1)):
                    issues.append("DISPLAY: review forbidden player-facing text in " + path)
    return issues


def validate(root: Path) -> list[str]:
    try:
        contract = json.loads((root / "Docs/Design/NarrativeContract.json").read_text(encoding="utf-8"))
        if not isinstance(contract, dict):
            return ["CONTRACT: expected JSON object"]
        source_root = root / "Source/IshibashiriPrototype"
        sources = {
            p.relative_to(root).as_posix(): p.read_text(encoding="utf-8")
            for p in source_root.rglob("*") if p.is_file() and p.suffix in {".cpp", ".h"}
        }
        return audit_sources(sources, contract)
    except (OSError, ValueError) as exc:
        return ["INPUT_ERROR: " + str(exc)]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    args = parser.parse_args()
    issues = validate(args.root.resolve())
    for issue in issues:
        print(issue)
    print("NARRATIVE_SOURCE_CONTRACT_" + ("FAIL" if issues else "PASS"))
    print("UE_RUNTIME=NOT_RUN; APPEARANCE_STAGE_MANAGEMENT=IMPLEMENTED; MATERIAL_SCRIPT=IMPLEMENTED_NOT_RUN; "
          "MATERIAL_CONNECTION=IMPLEMENTED_LEFT_ARM_ONLY; no gameplay certification")
    return int(bool(issues))


if __name__ == "__main__":
    raise SystemExit(main())
