from pathlib import Path


ROOT = Path(__file__).parents[1]
PRIVATE = ROOT / "Source/IshibashiriPrototype/Private"


def read(name: str) -> str:
    return (PRIVATE / name).read_text(encoding="utf-8")


def body(source: str, signature: str) -> str:
    return source.split(signature, 1)[1].split("\n}\n", 1)[0]


def test_campaign_arena_is_dressed_without_moving_its_blockers():
    mode = read("PrototypeGameMode.cpp")
    arena = body(mode, "void APrototypeGameMode::CreateArena()")
    for wall in ("WestWall", "EastWall", "SouthWall", "NorthWall"):
        assert f'CreateBlock(TEXT("{wall}")' in arena
    assert 'Mesh->SetCollisionProfileName(TEXT("BlockAll"))' in body(mode, "APrototypeGameMode::CreateBlock(")
    assert arena.index("NorthWall") < arena.index("SpawnActor<APrototypeArenaDressing>") < arena.index("ADirectionalLight* Sun")
    assert "Dressing->Dress(H, Floor, Walls)" in arena
    # The basin path never builds the legacy arena, so it is never dressed twice.
    assert "else CreateArena();" in mode
    assert "SoftFillLight->SetAtmosphereSunLight(false)" in arena


def test_dressing_is_render_only_and_falls_back_to_the_blockout():
    source = read("PrototypeArenaDressing.cpp")
    dress = body(source, "void APrototypeArenaDressing::Dress(")
    assert "if (!Cedar && !Rock && !Ground) return;" in dress
    assert dress.index("return;") < dress.index("bDressed = true")
    assert "ForestGround->SetCollisionEnabled(ECollisionEnabled::NoCollision)" in dress
    # Walls are only hidden when the rock rim that replaces them exists; collision is untouched.
    rock_branch = dress.split("if (Rock)", 1)[1].split("AddForest", 1)[0]
    assert "Wall->SetVisibility(false)" in rock_branch
    assert "AddRockRim(H, Rock, RockB)" in rock_branch
    assert "SetCollision" not in rock_branch
    for forbidden in ("SetCollisionProfileName", "BlockAll", "SetActorLocation", "Kakon", "Boss", "Player"):
        assert forbidden not in source
    assert "NewObject<UStaticMeshComponent>" in source
    assert source.count("NewObject<UStaticMeshComponent>") == 1


def test_dressing_stays_outside_the_playable_square():
    source = read("PrototypeArenaDressing.cpp")
    rim = body(source, "void APrototypeArenaDressing::AddRockRim(")
    assert "(Row ? H + 1250.f : H + 550.f)" in rim
    assert "Out * Inset" in rim
    forest = body(source, "void APrototypeArenaDressing::AddForest(")
    assert "H / FMath::Max(FMath::Abs(Out.X), FMath::Abs(Out.Y))" in forest
    assert "Edge + 150.f" in forest
    assert "H * 1.42f + 900.f" in forest
