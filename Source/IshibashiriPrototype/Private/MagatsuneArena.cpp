#include "MagatsuneArena.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "PrimitiveAppearance.h"

AMagatsuneArena::AMagatsuneArena()
{
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("ArenaRoot")));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    auto Part = [&](const TCHAR* N, FVector P, FVector S)
    {
        auto* M = CreateDefaultSubobject<UStaticMeshComponent>(N);
        M->SetupAttachment(RootComponent);
        M->SetStaticMesh(Cube.Object);
        M->SetMaterial(0, Mat.Object);
        M->SetRelativeLocation(P);
        M->SetRelativeScale3D(S);
        M->SetCollisionProfileName(TEXT("BlockAll"));
    };
    Part(TEXT("Ground"), {0, 0, -60}, {35, 25, 1});
    Part(TEXT("Phase1SafeGround"), {-220, -600, 160}, {5, 5, 1});
    Part(TEXT("Phase2RockPillar"), {450, 650, 480}, {3, 3, 10});
    Part(TEXT("Phase2BrokenColumn"), {650, -520, 700}, {2, 2, 14});
    Part(TEXT("Phase3RecoveryLedge"), {900, 0, 1050}, {6, 5, 1});
    for (int32 I = 0; I < 6; ++I)
        Part(*FString::Printf(TEXT("DeadTree%d"), I), {-700.f + I * 300.f, (I % 2 ? 1 : -1) * 900.f, 250}, {.7, .7, 6});
}

void AMagatsuneArena::BeginPlay()
{
    Super::BeginPlay();
    TArray<UStaticMeshComponent*> P;
    GetComponents(P);
    for (auto* M : P)
        SetPrimitiveColor(M->CreateDynamicMaterialInstance(0),
            M->GetName().Contains(TEXT("Safe")) || M->GetName().Contains(TEXT("Recovery")) ? FLinearColor(.28, .42, .25)
                                                                                           : FLinearColor(.17, .13, .19));
    auto* Sun = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0, 0, 3000), FRotator(-45, 130, 0));
    Sun->GetLightComponent()->SetIntensity(3);
}
