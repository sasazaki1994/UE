"""Stable contracts for repository-wide AI development guidance."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(relative_path: str) -> str:
    return (ROOT / relative_path).read_text(encoding="utf-8")


def test_root_agents_file_preserves_identity_encounters_and_core_loop():
    rules = read("AGENTS.md")
    assert "禍祓い" in rules
    assert "大歳ノ島風の和風世界観 × ワンダと巨像型の巨大生物戦" in rules
    for encounter in ("Ishibashiri", "Fuchimatoi", "Minedaki", "Magatsune"):
        assert encounter in rules
    for system in ("Grab", "Climbing", "Kakon", "Calm"):
        assert system in rules
    assert "Do not turn this into a game about reducing enemy HP to zero" in rules


def test_rules_preserve_scope_workflow_and_validation_contracts():
    rules = read("AGENTS.md")
    for contract in (
        "Campaign Save/Continue **already exists**",
        "Small PRs",
        "python -m pytest Tests -q",
        "Tools/GameplayContract.py --update",
        "Tools/Prototype.ps1",
        "**PASS:**",
        "**NOT_RUN:**",
        "**STATIC_PASS:**",
    ):
        assert contract in rules
    assert "never edit the JSON manually" in rules


def test_cursor_rule_is_a_thin_agents_adapter_without_obsolete_save_ban():
    cursor_rule = read(".cursor/rules/ishibashiri.mdc")
    assert "AGENTS.md" in cursor_rule
    assert "python -m pytest Tests -q" in cursor_rule
    assert "Tools/GameplayContract.py --update" in cursor_rule
    assert "Tools/Prototype.ps1" in cursor_rule
    assert "Do not add NPCs, gear, inventory, saves" not in cursor_rule
    assert "Campaign Save/Continue is an existing supported feature" in cursor_rule
