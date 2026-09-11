from pathlib import Path


ROOT = Path(__file__).parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def test_basin_is_explicit_and_does_not_create_legacy_arena() -> None:
    source = read("Source/IshibashiriPrototype/Private/PrototypeGameMode.cpp")
    assert 'FParse::Param(FCommandLine::Get(), TEXT("BasinPrototype"))' in source
    assert "if (bBasinPrototype)" in source
    assert "else CreateArena();" in source


def test_basin_separates_visual_rocks_from_blocking_boundary() -> None:
    source = read("Source/IshibashiriPrototype/Private/BasinPrototypeArena.cpp")
    assert "Rock->SetCollisionEnabled(ECollisionEnabled::NoCollision)" in source
    assert 'Boundary->SetCollisionProfileName(TEXT("BlockAll"))' in source
    assert "for (int32 I=0; I<8; ++I)" in source
    assert "for (int32 J=-2; J<=2; ++J)" in source


def test_basin_switch_is_forwarded_to_play_and_tests() -> None:
    script = read("Tools/Prototype.ps1")
    assert "[switch]$Basin" in script
    assert script.count("$TestArguments += '-BasinPrototype'") == 1
    assert script.count("$PlayArguments += '-BasinPrototype'") == 1


def test_basin_visual_language_is_readable_and_non_blocking() -> None:
    source = read("Source/IshibashiriPrototype/Private/BasinPrototypeArena.cpp")
    assert 'TEXT("SacredPath%02d")' in source
    assert 'TEXT("RitualStake%d")' in source
    assert "Accent->SetCollisionEnabled(ECollisionEnabled::NoCollision)" in source
    assert 'TEXT("BasinGroundHaze")' in source
    assert "Fog->SetFogMaxOpacity(.22f)" in source


def test_basin_scenario_is_separate_and_requires_basin() -> None:
    script = read("Tools/Prototype.ps1")
    mode = read("Source/IshibashiriPrototype/Private/PrototypeGameMode.cpp")
    assert "[switch]$BasinScenario" in script
    assert "if ($BasinScenario -and !$Basin)" in script
    assert "'-BasinPlaythroughTest'" in script
    assert 'TEXT("BasinPlaythroughTest")' in mode


def test_basin_scenario_does_not_arrange_or_stop_gameplay() -> None:
    source = read("Source/IshibashiriPrototype/Private/BasinPlaythroughTest.cpp")
    assert "SetActorLocation" not in source
    assert "SetActorTickEnabled" not in source
    assert "GrabRange - 12.f" in source
    assert "BASIN_SCENARIO_FAIL" in source
    assert "ReleaseAll();" in source
    assert 'Tap(EKeys::R)' in source
