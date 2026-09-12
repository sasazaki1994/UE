#include "PrototypeGameMode.h"
#include "PrototypeHUD.h"
#include "PrototypePlayer.h"
#include "IshibashiriBoss.h"
#include "NushiEncounterManager.h"
#include "PrototypeSmokeTest.h"
#include "PrototypePlaythroughTest.h"
#include "ClimbingIntegrationTest.h"
#include "PrototypeGrabTest.h"
#include "PrototypeClimbingTest.h"
#include "PrototypeGamepadTest.h"
#include "PrimitiveAppearance.h"
#include "BasinPrototypeArena.h"
#include "BasinPlaythroughTest.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/ConstructorHelpers.h"

APrototypeGameMode::APrototypeGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = nullptr;
    HUDClass = APrototypeHUD::StaticClass();
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    CubeMesh = Cube.Object;
    BaseMaterial = Material.Object;
}

FTransform APrototypeGameMode::PlayerSpawn() const
{
    if (bBasinPrototype && BasinArena) return FTransform(FRotator::ZeroRotator, BasinArena->PlayerStart);
    return FTransform(FRotator::ZeroRotator, FVector(-1150.f, -750.f, 92.f));
}

FTransform APrototypeGameMode::BossSpawn() const
{
    if (bBasinPrototype && BasinArena) return FTransform(FRotator(0.f, 180.f, 0.f), BasinArena->BossStart);
    return FTransform(FRotator(0.f, 180.f, 0.f), FVector(650.f, 0.f, 352.f));
}

void APrototypeGameMode::StartPlay()
{
    bBasinPrototype = FParse::Param(FCommandLine::Get(), TEXT("BasinPrototype"));
    if (bBasinPrototype)
    {
        BasinArena = GetWorld()->SpawnActor<ABasinPrototypeArena>();
        if (BasinArena) ArenaHalfExtent = BasinArena->ClearingHalfExtent;
    }
    else CreateArena();
    Super::StartPlay();
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Player = GetWorld()->SpawnActor<APrototypePlayer>(APrototypePlayer::StaticClass(), PlayerSpawn(), Params);
    Boss = GetWorld()->SpawnActor<AIshibashiriBoss>(AIshibashiriBoss::StaticClass(), BossSpawn(), Params);
    EncounterManager = GetWorld()->SpawnActor<ANushiEncounterManager>();
    APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
    if (!Player || !Boss || !Controller || !EncounterManager)
    {
        UE_LOG(LogTemp, Error, TEXT("Ishibashiri: player, boss, encounter manager or local controller failed to spawn. Use Play, not Simulate."));
        Result = EEncounterResult::Defeat;
        return;
    }
    Controller->Possess(Player);
    Controller->bShowMouseCursor = false;
    Controller->SetInputMode(FInputModeGameOnly());
    if (Controller->PlayerCameraManager)
    {
        Controller->PlayerCameraManager->ViewPitchMin = -65.f;
        Controller->PlayerCameraManager->ViewPitchMax = 20.f;
    }
    Boss->ConfigureEncounter(BossSpawn(), Player);
    EncounterManager->SetNushi(Boss);
    EncounterManager->OnEncounterCompleted.AddUniqueDynamic(this, &APrototypeGameMode::HandleEncounterCompleted);
    RetryEncounter();
    UE_LOG(LogTemp, Display, TEXT("Ishibashiri: arena ready. WASD/Left Stick move, Mouse/Right Stick camera, E/RB grab."));
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("BasinPlaythroughTest")))
        GetWorld()->SpawnActor<ABasinPlaythroughTest>();
    else if (FParse::Param(FCommandLine::Get(), TEXT("ClimbingTest")))
        GetWorld()->SpawnActor<AClimbingIntegrationTest>();
    else if (FParse::Param(FCommandLine::Get(), TEXT("PrototypeGamepadTest")))
        GetWorld()->SpawnActor<APrototypeGamepadTest>();
    else if (FParse::Param(FCommandLine::Get(), TEXT("PrototypeClimbingTest")))
        GetWorld()->SpawnActor<APrototypeClimbingTest>();
    else if (FParse::Param(FCommandLine::Get(), TEXT("PrototypeGrabTest")))
        GetWorld()->SpawnActor<APrototypeGrabTest>();
    else if (FParse::Param(FCommandLine::Get(), TEXT("PrototypePlaythrough")))
        GetWorld()->SpawnActor<APrototypePlaythroughTest>();
    else if (FParse::Param(FCommandLine::Get(), TEXT("PrototypeSmokeTest"))
        || FParse::Param(FCommandLine::Get(), TEXT("PrototypeCameraTest")))
        GetWorld()->SpawnActor<APrototypeSmokeTest>();
#endif
}

void APrototypeGameMode::RetryEncounter()
{
    if (!Player || !Boss || !EncounterManager) return;
    Result = EEncounterResult::Playing;
    Player->ResetForEncounter(PlayerSpawn());
    EncounterManager->ResetEncounter();
    EncounterManager->StartEncounter();
}

void APrototypeGameMode::HandleEncounterCompleted()
{
    FinishEncounter(true);
}

void APrototypeGameMode::FinishEncounter(bool bVictory)
{
    if (!IsEncounterActive()) return;
    Result = bVictory ? EEncounterResult::Victory : EEncounterResult::Defeat;
    if (Player) Player->StopCombat();
    UE_LOG(LogTemp, Display, TEXT("Ishibashiri: %s. Press R to retry."), bVictory ? TEXT("Victory") : TEXT("Defeat"));
}

void APrototypeGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    // A fallback for deliberately testing the GameMode on a level with holes.
    if (IsEncounterActive() && Player && Player->GetActorLocation().Z < -500.f) FinishEncounter(false);
}

void APrototypeGameMode::CreateBlock(const FString& Name, const FVector& Position, const FVector& Scale, const FLinearColor& Color)
{
    AStaticMeshActor* Block = GetWorld()->SpawnActor<AStaticMeshActor>(Position, FRotator::ZeroRotator);
    if (!Block) return;
    UStaticMeshComponent* Mesh = Block->GetStaticMeshComponent();
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetStaticMesh(CubeMesh);
    Mesh->SetWorldScale3D(Scale);
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));
    Mesh->SetCollisionObjectType(ECC_WorldStatic);
    Mesh->SetMaterial(0, BaseMaterial);
    if (UMaterialInstanceDynamic* Material = Mesh->CreateDynamicMaterialInstance(0))
        SetPrimitiveColor(Material, Color);
    Block->Tags.Add(FName(*Name));
}

void APrototypeGameMode::CreateArena()
{
    const float H = ArenaHalfExtent;
    CreateBlock(TEXT("ArenaFloor"), FVector(0.f, 0.f, -50.f), FVector((H * 2.f + 200.f) / 100.f, (H * 2.f + 200.f) / 100.f, 1.f), FLinearColor(0.26f, 0.29f, 0.25f));
    const FLinearColor WallColor(0.33f, 0.36f, 0.38f);
    CreateBlock(TEXT("WestWall"), FVector(-H - 50.f, 0.f, 200.f), FVector(1.f, (H * 2.f + 200.f) / 100.f, 4.f), WallColor);
    CreateBlock(TEXT("EastWall"), FVector(H + 50.f, 0.f, 200.f), FVector(1.f, (H * 2.f + 200.f) / 100.f, 4.f), WallColor);
    CreateBlock(TEXT("SouthWall"), FVector(0.f, -H - 50.f, 200.f), FVector((H * 2.f) / 100.f, 1.f, 4.f), WallColor);
    CreateBlock(TEXT("NorthWall"), FVector(0.f, H + 50.f, 200.f), FVector((H * 2.f) / 100.f, 1.f, 4.f), WallColor);

    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0.f, 0.f, 1200.f), FRotator(-55.f, -30.f, 0.f));
    if (Sun)
    {
        Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Sun->GetLightComponent()->SetIntensity(3.f);
    }
    APointLight* Fill = GetWorld()->SpawnActor<APointLight>(FVector(0.f, 0.f, 1400.f), FRotator::ZeroRotator);
    ADirectionalLight* SoftFill = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,1400),FRotator(-35,145,0));
    if (SoftFill)
    {
        SoftFill->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        SoftFill->GetLightComponent()->SetIntensity(1.1f);
        SoftFill->GetLightComponent()->SetLightColor(FLinearColor(.66f,.75f,1.f));
        SoftFill->GetLightComponent()->SetCastShadows(false);
    }
    if (Fill)
    {
        Fill->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        if (UPointLightComponent* Light = Cast<UPointLightComponent>(Fill->GetLightComponent()))
        {
            Light->SetAttenuationRadius(6000.f);
            Light->SetIntensity(120000.f);
            Light->SetCastShadows(false);
        }
    }
}
