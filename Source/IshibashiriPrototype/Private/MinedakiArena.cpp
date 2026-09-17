#include "MinedakiArena.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "PrimitiveAppearance.h"

AMinedakiArena::AMinedakiArena()
{
    PrimaryActorTick.bCanEverTick = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("ArenaRoot")));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    auto Part = [&](const TCHAR* Name, FVector Position, FVector Scale)
    {
        auto* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Mesh->SetupAttachment(RootComponent);
        Mesh->SetStaticMesh(Cube.Object);
        Mesh->SetMaterial(0, Material.Object);
        Mesh->SetRelativeLocation(Position);
        Mesh->SetRelativeScale3D(Scale);
        Mesh->SetCollisionProfileName(TEXT("BlockAll"));
    };
    Part(TEXT("Ground"), FVector(-600, 0, -60), FVector(100, 90, 1.2));
    Part(TEXT("CliffAndUpperPlateau"), FVector(-3900, 0, 1000), FVector(36, 45, 20));
    // Small recovery ledges require a short walk and an explicit shared Grab; they do not bypass the route.
    Part(TEXT("RecoveryLedgeKakon1"), FVector(-2850, -500, 3450), FVector(8, 7, 1));
    Part(TEXT("RecoveryLedgeKakon2"), FVector(-2850, 500, 3700), FVector(8, 7, 1));
    for (int32 I = 0; I < 4; ++I)
        Part(*FString::Printf(TEXT("HandHold%d"), I), FVector(-2070, I % 2 ? -480 : 480, 600 + I * 420), FVector(1, 3, .9));
    for (int32 I = 0; I < 5; ++I) Part(*FString::Printf(TEXT("HeightBand%d"), I), FVector(-2090, 0, 100 + I * 450), FVector(.3, 45, .2));
}

void AMinedakiArena::BeginPlay()
{
    Super::BeginPlay();
    TArray<UStaticMeshComponent*> Parts;
    GetComponents(Parts);
    for (auto* Part : Parts)
        SetPrimitiveColor(Part->CreateDynamicMaterialInstance(0),
            Part->GetName().Contains(TEXT("HandHold"))         ? FLinearColor(.8, .6, .25)
                : Part->GetName().Contains(TEXT("HeightBand")) ? FLinearColor(.65, .62, .48)
                                                               : FLinearColor(.22, .3, .26));
    auto* Sun = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0, 0, 5000), FRotator(-40, 145, 0));
    Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sun->GetLightComponent()->SetIntensity(3);
    auto* Fill = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0, 0, 5000), FRotator(-25, -35, 0));
    Fill->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Fill->GetLightComponent()->SetIntensity(2);
    Fill->GetLightComponent()->SetCastShadows(false);
    auto* WallFill = GetWorld()->SpawnActor<APointLight>(FVector(1200, -1400, 2500), FRotator::ZeroRotator);
    WallFill->PointLightComponent->SetMobility(EComponentMobility::Movable);
    WallFill->PointLightComponent->SetIntensity(180000.f);
    WallFill->PointLightComponent->SetAttenuationRadius(10000.f);
    WallFill->PointLightComponent->SetCastShadows(false);
}
