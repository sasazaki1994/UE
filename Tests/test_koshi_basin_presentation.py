"""Static contracts: Koshi Basin presentation preserves the proven encounter."""

import ast
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PRIVATE = ROOT / "Source/IshibashiriPrototype/Private"
PUBLIC = ROOT / "Source/IshibashiriPrototype/Public"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def function_body(source: str, name: str) -> str:
    """Extract one function so a guard in another function cannot satisfy it."""
    signature = re.search(
        rf"\b{re.escape(name)}\s*\([^)]*\)\s*(?:const\s*)?\{{",
        source,
    )
    assert signature, f"missing function {name}"
    opening = source.find("{", signature.start())
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[opening + 1 : index]
    raise AssertionError(f"unclosed function {name}")


def compact(source: str) -> str:
    return re.sub(r"\s+", "", source)


def call_arguments(source: str, name: str) -> list[list[str]]:
    """Split top-level arguments without confusing nested FVector/FString calls."""
    result = []
    for call in re.finditer(rf"\b{re.escape(name)}\s*\(", source):
        opening = source.find("(", call.start())
        depth = 0
        argument_start = opening + 1
        arguments = []
        quoted = False
        for index in range(opening, len(source)):
            char = source[index]
            if char == '"' and (index == 0 or source[index - 1] != "\\"):
                quoted = not quoted
            if quoted:
                continue
            if char == "(":
                depth += 1
            elif char == ")":
                depth -= 1
                if depth == 0:
                    arguments.append(source[argument_start:index].strip())
                    result.append(arguments)
                    break
            elif char == "," and depth == 1:
                arguments.append(source[argument_start:index].strip())
                argument_start = index + 1
        else:
            raise AssertionError(f"unclosed call to {name}")
    return result


def arena_body(name: str) -> str:
    return function_body(read(PRIVATE / "BasinPrototypeArena.cpp"), f"ABasinPrototypeArena::{name}")


def mode_body(name: str) -> str:
    return function_body(read(PRIVATE / "PrototypeGameMode.cpp"), f"APrototypeGameMode::{name}")


def test_koshi_basin_keeps_the_eighty_metre_playable_space_and_spawns():
    header = compact(read(PUBLIC / "BasinPrototypeArena.h"))
    assert "ClearingHalfExtent=4000.f;" in header
    assert "RockWallHeight=1350.f;" in header
    assert "PlayerStart=FVector(-3000.f,0.f,92.f);" in header
    assert "BossStart=FVector(550.f,0.f,352.f);" in header
    player = compact(mode_body("PlayerSpawn"))
    boss = compact(mode_body("BossSpawn"))
    assert "if(bBasinPrototype&&BasinArena)returnFTransform(FRotator::ZeroRotator,BasinArena->PlayerStart);" in player
    assert "if(bBasinPrototype&&BasinArena)returnFTransform(FRotator(0.f,180.f,0.f),BasinArena->BossStart);" in boss
    start = compact(mode_body("StartPlay"))
    assert "if(BasinArena)ArenaHalfExtent=BasinArena->ClearingHalfExtent;" in start
    assert "elseCreateArena();" in start
    assert start.count("SpawnActor<ABasinPrototypeArena>") == 1


def test_koshi_basin_floor_keeps_flat_collision_at_zero_height():
    floor = compact(arena_body("AddFloor"))
    assert "Floor->SetStaticMesh(CubeMesh);" in floor
    assert "Floor->SetRelativeLocation(FVector::ZeroVector+FVector(0,0,-50));" in floor
    assert "Floor->SetRelativeScale3D(FVector((ClearingHalfExtent*2.f+600.f)/100.f,(ClearingHalfExtent*2.f+600.f)/100.f,1.f));" in floor
    assert 'Floor->SetCollisionProfileName(TEXT("BlockAll"));' in floor
    assert floor.index("Floor->RegisterComponent();") < floor.index("GeneratedComponents.Add(Floor);") < floor.index("BasinFloor=Floor;")
    assert "NoCollision" not in floor


def test_koshi_basin_preserves_the_eight_invisible_boundary_slabs():
    boundary = compact(arena_body("AddBoundary"))
    assert "Boundary->SetStaticMesh(CubeMesh);" in boundary
    assert "Boundary->SetRelativeLocation(Location);" in boundary
    assert "Boundary->SetRelativeRotation(Rotation);" in boundary
    assert "Boundary->SetRelativeScale3D(Scale);" in boundary
    assert "Boundary->SetVisibility(false);" in boundary
    assert 'Boundary->SetCollisionProfileName(TEXT("BlockAll"));' in boundary
    assert "Boundary->SetCollisionObjectType(ECC_WorldStatic);" in boundary
    construction = compact(arena_body("OnConstruction"))
    assert "for(int32I=0;I<8;++I)" in construction
    assert "constfloatYaw=I*45.f;" in construction
    assert 'AddBoundary(*FString::Printf(TEXT("BasinBoundary%02d"),I),Out*(H+125.f)+FVector(0,0,RockWallHeight*.45f),FVector(2.5f,38.f,RockWallHeight/100.f),FRotator(0,Yaw,0));' in construction
    assert construction.count("AddBoundary(") == 1
    assert construction.count("AddFloor();") == 1


def test_koshi_basin_visual_wall_outset_remains_separate_from_combat_collision():
    construction = arena_body("OnConstruction")
    rocks = call_arguments(construction, "AddRock")
    wall_rocks = [arguments for arguments in rocks if "WeatheredRock%02d_%02d" in arguments[0]]
    assert len(wall_rocks) == 1
    assert "Out*(H+1650.f)+Along*(J*760.f)" in compact(wall_rocks[0][1])
    # These authored bases protect the outset observed in the camera repair.
    # Static placement cannot guarantee view clearance for every mesh bound or camera pose.
    assert "Rock->SetCollisionEnabled(ECollisionEnabled::NoCollision);" in compact(arena_body("AddRock"))
    assert 'Boundary->SetCollisionProfileName(TEXT("BlockAll"));' in compact(arena_body("AddBoundary"))


def test_koshi_basin_render_meshes_cannot_block_players_or_camera():
    helper = read(PRIVATE / "IshibashiriEnvironment.h")
    for name in ("Visual", "Scatter"):
        body = function_body(helper, name)
        assert "SetCollisionEnabled(ECollisionEnabled::NoCollision)" in body, name
        assert "SetCollisionProfileName" not in body, name
        assert "SetCollisionResponse" not in body, name
        assert "BlockAll" not in body, name
    for name, component in (("AddRock", "Rock"), ("AddAccent", "Accent")):
        body = compact(arena_body(name))
        assert f"{component}->SetCollisionEnabled(ECollisionEnabled::NoCollision);" in body
        assert "SetCollisionProfileName" not in body
    assert "C->SetCastShadow(false);" in function_body(helper, "Scatter")


def test_koshi_basin_reconstruction_destroys_generated_components_before_replacing_them():
    clear = compact(arena_body("ClearGeneratedComponents"))
    assert "if(IsValid(Component))Component->DestroyComponent();" in clear
    assert clear.index("Component->DestroyComponent();") < clear.index("GeneratedComponents.Reset();")
    for pointer in ("BasinFloor", "GroundFog", "KeyLight"):
        assert f"{pointer}=nullptr;" in clear
    assert "VisualRockCount=BoundaryCount=AccentCount=TreeCount=0;" in clear
    construction = compact(arena_body("OnConstruction"))
    assert construction.index("Super::OnConstruction(Transform);") < construction.index("ClearGeneratedComponents();") < construction.index("AddFloor();")


def test_koshi_basin_calm_and_retry_only_restore_presentation():
    completed = compact(mode_body("HandleEncounterCompleted"))
    retry = compact(mode_body("RetryEncounter"))
    assert "if(BasinArena)BasinArena->SetCalmPresentation(true);" in completed
    assert "if(BasinArena)BasinArena->SetCalmPresentation(false);" in retry
    assert retry.index("SetCalmPresentation(false)") < retry.index("Player->ResetForEncounter(PlayerSpawn())")
    for forbidden in ("SpawnActor<ABasinPrototypeArena>", "OnConstruction(", "ClearGeneratedComponents("):
        assert forbidden not in retry
    calm = arena_body("SetCalmPresentation")
    for forbidden in ("CalmNushi", "Purify", "ResetEncounter", "CompleteEncounter", "TravelToCurrentChapter", "SetActorLocation"):
        assert forbidden not in calm


def test_koshi_basin_tick_reads_encounter_state_without_changing_it():
    tick = arena_body("Tick")
    dense = compact(tick)
    assert "constauto*Mode=" in dense and "constauto*Boss=" in dense
    assert "!bCalmPresentation&&Mode&&Mode->IsEncounterActive()&&IsValid(Boss)&&Boss->GetState()==EIshibashiriState::Charge" in dense
    assert set(re.findall(r"\bBoss->(\w+)\s*\(", tick)) == {"GetState", "GetChargeDirection", "GetActorLocation"}
    assert set(re.findall(r"\bMode->(\w+)\s*\(", tick)) == {"GetBoss", "IsEncounterActive"}
    for forbidden in ("SpawnActor", "NewObject", "AddInstance(", "AddInstances(", "SetTimer", "LoadObject", "Load<"):
        assert forbidden not in tick
    # Presentation may write its components, never the actors or encounter authority.
    for name in ("AddGroundHistory", "AddWallDressing", "AddRitualRemnants", "AddRitualPiece", "CreateChargePresentation", "ResetChargePresentation", "SetCalmPresentation", "Tick"):
        body = arena_body(name)
        for forbidden in (
            "SetActorLocation", "SetActorRotation", "SetActorTransform", "SetActorTickEnabled",
            "ResetForEncounter", "ResetEncounter", "StartEncounter", "CompleteEncounter",
            "TravelToCurrentChapter", "CalmNushi", "TryPurify", "PurifyKakon", "ApplyDamage", "TakeDamage", "EnterState",
        ):
            assert forbidden not in body, (name, forbidden)


def test_koshi_basin_charge_uses_a_fixed_non_blocking_pool_with_allocated_custom_data():
    create = compact(arena_body("CreateChargePresentation"))
    hidden = re.search(r"Hidden\.Init\(FTransform\(.+?\),(\d+)\);", create)
    assert hidden and int(hidden[1]) == 16
    for array, value in (("DustAges", "1.f"), ("DustOrigins", "FVector::ZeroVector"), ("DustDirections", "FVector::ZeroVector")):
        assert f"{array}.Init({value},16);" in create
    assert create.count("IshibashiriEnvironment::Scatter(") == 2
    assert "ChargeDust->SetNumCustomDataFloats(1);" in create
    assert "ChargeDust->NumCustomDataFloats=" not in create
    for component in ("ChargeDust", "ChargePebbles"):
        assert f"{component}->SetMobility(EComponentMobility::Movable);" in create
        assert f"GeneratedComponents.Add({component});" in create
    tick = compact(arena_body("Tick"))
    assert "NextDust=(NextDust+1)%DustAges.Num();" in tick
    assert "UpdateInstanceTransform" in tick and "SetCustomDataValue" in tick
    assert "SetCollision" not in create


def test_koshi_basin_expired_charge_pool_has_bounded_ages_and_stops_updating():
    tick = compact(arena_body("Tick"))
    assert "constfloatStep=FMath::Max(0.f,DeltaSeconds);" in tick
    assert "for(float&Age:DustAges)Age=FMath::Min(1.f,Age+Step);" in tick
    idle_guard = (
        "if(!bCharging&&!DustAges.ContainsByPredicate([](floatAge){returnAge<.65f;}))"
        "{ResetChargePresentation();return;}"
    )
    assert idle_guard in tick
    # Stop before touching instance transforms or submitting render-state changes.
    assert tick.index(idle_guard) < tick.index("DustSpawnRemaining-=Step;") < tick.index("UpdateInstanceTransform(")


def test_koshi_basin_calm_retry_and_reconstruction_clear_dynamic_effects():
    reset = compact(arena_body("ResetChargePresentation"))
    assert "DustSpawnRemaining=0.f;NextDust=0;" in reset
    assert "for(float&Age:DustAges)Age=1.f;" in reset
    for component in ("ChargeDust", "ChargePebbles"):
        assert f"if({component}){component}->SetVisibility(false);" in reset
    assert "if(RumbleAudio)RumbleAudio->Stop();" in reset
    calm = compact(arena_body("SetCalmPresentation"))
    assert "bCalmPresentation=bCalm;" in calm
    assert "ResetChargePresentation();" in calm
    assert "GroundFog->SetFogDensity(bCalm?.008f:.012f);" in calm
    assert "GroundFog->SetFogMaxOpacity(bCalm?.16f:.22f);" in calm
    assert "KeyLight->SetLightColor(bCalm?" in calm
    assert "ForestAudio->SetVolumeMultiplier(bCalm?.45f:.12f);" in calm
    tick = compact(arena_body("Tick"))
    assert "if(bCalmPresentation||!Mode||!Mode->IsEncounterActive()){ResetChargePresentation();return;}" in tick
    clear = compact(arena_body("ClearGeneratedComponents"))
    for pointer in ("ChargeDust", "ChargePebbles", "ForestAudio", "RumbleAudio"):
        assert f"{pointer}=nullptr;" in clear
    for array in ("DustAges", "DustOrigins", "DustDirections"):
        assert f"{array}.Reset();" in clear
    assert "DustSpawnRemaining=0.f;NextDust=0;bCalmPresentation=false;" in clear


def test_koshi_basin_ground_history_is_shallow_decal_projection_only():
    history = arena_body("AddGroundHistory")
    projections = call_arguments(history, "IshibashiriEnvironment::Decal")
    assert projections
    for arguments in projections:
        assert len(arguments) == 7
        depth = re.match(r"FVector\(([\d.]+)f,", compact(arguments[5]))
        assert depth and 0 < float(depth[1]) <= 4, arguments[5]
    for forbidden in ("NewObject", "SpawnActor", "AddRock(", "AddAccent(", "AddBoundary(", "SetCollision", "SetActor"):
        assert forbidden not in history
    assert "if (FootprintDecal)" in history
    assert "if (Material)" in history
    assert "if (CrackDecal && I % 3 == 0)" in history
    assert "if (WetSoilDecal)" in history


def test_koshi_basin_wall_dressing_uses_deterministic_outer_bases_and_culled_instances():
    wall = arena_body("AddWallDressing")
    dense = compact(wall)
    assert "IshibashiriEnvironment::Hash01" in wall
    assert "ClearingHalfExtent+1300.f+Hash01(I,52)*620.f" in dense
    assert "ClearingHalfExtent+240.f+Hash01(I,57)*250.f" in dense
    assert "ClearingHalfExtent+3300.f" in dense
    scatters = call_arguments(wall, "IshibashiriEnvironment::Scatter")
    assert len(scatters) == 3
    for arguments in scatters:
        assert len(arguments) == 6
        assert 0 < float(arguments[-1].rstrip("f")) <= 16000
    assert "SetCollision" not in wall and "AddBoundary(" not in wall
    assert "FMath::Rand" not in wall and "FMath::FRand" not in wall


def test_koshi_basin_ritual_assets_are_optional_render_only_replacement_points():
    remnants = arena_body("AddRitualRemnants")
    expected = {
        "SM_Ishibashiri_CollapsedTorii_A", "SM_Ishibashiri_ShimenawaPillar_A",
        "SM_Ishibashiri_SmallShrine_A", "SM_Ishibashiri_SacredStone_A",
        "SM_Ishibashiri_StoneLantern_A", "SM_Ishibashiri_Stele_A",
    }
    loads = set(re.findall(r'IshibashiriEnvironment::Load<UStaticMesh>\(TEXT\("([^\"]+)"\)\)', remnants))
    assert loads == expected
    piece = compact(arena_body("AddRitualPiece"))
    assert piece.index("if(!Mesh)return;") < piece.index("IshibashiriEnvironment::Visual(")
    assert "GeneratedComponents.Add(IshibashiriEnvironment::Visual(" in piece
    assert "SetCollision" not in piece
    for fallback in ("RitualPostMesh", "FallenCedarMesh", "BoundaryMesh", "RopeMesh", "SteppingStoneMesh", "RockBMesh.Get()"):
        assert fallback in remnants
    # Check placement bases, not final mesh bounds; silhouette clearance still needs UE review.
    dense = compact(remnants)
    for outer_base in (
        "GateBase(-H-200.f,1750.f,0.f)", "PillarBase(1650.f,H+180.f,0)",
        "ShrineBase(H+140.f,-1350.f,0)", "FVector(2950,-H-150.f,0)",
        "FVector(-1650,-H-120.f,0)", "FVector(-2650,H+160.f,0)",
    ):
        assert outer_base in dense


def test_koshi_basin_adds_each_presentation_layer_once_after_existing_ground_dressing():
    construction = compact(arena_body("OnConstruction"))
    previous = construction.index("AddGroundDressing();")
    for name in ("AddGroundHistory", "AddWallDressing", "AddRitualRemnants", "CreateChargePresentation"):
        call = f"{name}();"
        assert construction.count(call) == 1
        current = construction.index(call)
        assert previous < current
        previous = current
    create = compact(arena_body("CreateChargePresentation"))
    assert "IshibashiriEnvironment::Load<USoundBase>(Names[I])" in create
    assert create.index("if(!Sound)continue;") < create.index("NewObject<UAudioComponent>")
    assert "Audio->bAutoActivate=false;" in create
    assert "GeneratedComponents.Add(Audio);" in create


def test_koshi_basin_reuses_six_scale_trees_at_the_outer_rim():
    construction = compact(arena_body("OnConstruction"))
    trees = re.search(r"constFVectorTrees\[\]=\{(.*?)\};", construction)
    assert trees and trees[1].count("{") == 6
    placements = call_arguments(arena_body("OnConstruction"), "AddTree")
    scale_trees = [arguments for arguments in placements if "OldCedar%02d" in arguments[0]]
    assert len(scale_trees) == 1
    assert compact(scale_trees[0][1]) == "Trees[I].GetSafeNormal2D()*(H+850.f)"


def test_koshi_basin_flat_footprints_reuse_the_existing_mask():
    construction = compact(arena_body("OnConstruction"))
    assert 'FootprintDecal=IshibashiriEnvironment::Load<UMaterialInterface>(TEXT("D_Ishibashiri_BasinFootprint"));' in construction
    script = read(ROOT / "Tools/CreateKoshiBasinMaterials.py")
    tree = ast.parse(script)
    footprint_node = next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == "footprint")
    footprint = ast.get_source_segment(script, footprint_node)
    assert footprint
    assert "'/Decals/T_D_Ishibashiri_Footprint_A_BaseColor'" in footprint
    assert "unreal.MaterialDomain.MD_DEFERRED_DECAL" in footprint
    assert "if texture is None:" in footprint and "raise RuntimeError" in footprint
    assert "prop(alpha, MP.MP_OPACITY)" in footprint
    assert "prop(rough, MP.MP_ROUGHNESS)" in footprint
    assert "prop(specular, MP.MP_SPECULAR)" in footprint
    for raised_surface in ("MP_NORMAL", "MP_WORLD_POSITION_OFFSET", "MP_DISPLACEMENT", "import_asset"):
        assert raised_surface not in footprint
