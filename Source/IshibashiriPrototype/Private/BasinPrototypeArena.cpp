#include "BasinPrototypeArena.h"
#include "PrimitiveAppearance.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ABasinPrototypeArena::ABasinPrototypeArena()
{
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    CubeMesh = Cube.Object; SphereMesh = Sphere.Object; BaseMaterial = Material.Object;
}

void ABasinPrototypeArena::ClearGeneratedComponents()
{
    for (UActorComponent* Component : GeneratedComponents)
        if (IsValid(Component)) Component->DestroyComponent();
    GeneratedComponents.Reset(); VisualRockCount = BoundaryCount = 0;
}

void ABasinPrototypeArena::AddFloor()
{
    UStaticMeshComponent* Floor = NewObject<UStaticMeshComponent>(this, TEXT("BasinFloor"));
    Floor->SetupAttachment(Root); Floor->SetStaticMesh(CubeMesh);
    Floor->SetRelativeLocation(FVector::ZeroVector + FVector(0,0,-50));
    Floor->SetRelativeScale3D(FVector((ClearingHalfExtent * 2.f + 600.f) / 100.f, (ClearingHalfExtent * 2.f + 600.f) / 100.f, 1.f));
    Floor->SetCollisionProfileName(TEXT("BlockAll")); Floor->SetMaterial(0, BaseMaterial); Floor->RegisterComponent();
    if (UMaterialInstanceDynamic* Mat = Floor->CreateDynamicMaterialInstance(0)) SetPrimitiveColor(Mat, FLinearColor(.25f,.23f,.19f));
    GeneratedComponents.Add(Floor);
}

void ABasinPrototypeArena::AddRock(const TCHAR* Name, const FVector& Location, const FVector& Scale,
    const FRotator& Rotation, const FLinearColor& Color)
{
    UStaticMeshComponent* Rock = NewObject<UStaticMeshComponent>(this, Name);
    Rock->SetupAttachment(Root); Rock->SetStaticMesh(SphereMesh); Rock->SetRelativeLocation(Location);
    Rock->SetRelativeRotation(Rotation); Rock->SetRelativeScale3D(Scale); Rock->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Rock->SetMaterial(0, BaseMaterial); Rock->RegisterComponent();
    if (UMaterialInstanceDynamic* Mat = Rock->CreateDynamicMaterialInstance(0)) SetPrimitiveColor(Mat, Color);
    GeneratedComponents.Add(Rock); ++VisualRockCount;
}

void ABasinPrototypeArena::AddBoundary(const TCHAR* Name, const FVector& Location, const FVector& Scale, const FRotator& Rotation)
{
    UStaticMeshComponent* Boundary = NewObject<UStaticMeshComponent>(this, Name);
    Boundary->SetupAttachment(Root); Boundary->SetStaticMesh(CubeMesh); Boundary->SetRelativeLocation(Location);
    Boundary->SetRelativeRotation(Rotation); Boundary->SetRelativeScale3D(Scale); Boundary->SetVisibility(false);
    Boundary->SetCollisionProfileName(TEXT("BlockAll")); Boundary->SetCollisionObjectType(ECC_WorldStatic);
    Boundary->RegisterComponent(); GeneratedComponents.Add(Boundary); ++BoundaryCount;
}

void ABasinPrototypeArena::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform); ClearGeneratedComponents(); AddFloor();
    const float H = ClearingHalfExtent;
    // Eight overlapping collision slabs form a gap-free octagon. Visual rocks sit outside
    // their inner faces, keeping the playable 80 m clearing open and the camera sweep useful.
    for (int32 I=0; I<8; ++I)
    {
        const float Yaw = I * 45.f;
        const FVector Out = FRotator(0,Yaw,0).Vector();
        AddBoundary(*FString::Printf(TEXT("BasinBoundary%02d"),I), Out * (H + 125.f) + FVector(0,0,RockWallHeight*.45f),
            FVector(2.5f,38.f,RockWallHeight/100.f), FRotator(0,Yaw,0));
        for (int32 J=-2; J<=2; ++J)
        {
            const FVector Along = FRotator(0,Yaw+90.f,0).Vector();
            const float HeightFactor = 0.82f + 0.09f * ((I * 3 + J + 7) % 4);
            const FLinearColor Color = ((I+J)&1) ? FLinearColor(.28f,.29f,.27f) : FLinearColor(.34f,.35f,.32f);
            AddRock(*FString::Printf(TEXT("WeatheredRock%02d_%02d"),I,J+2), Out*(H+850.f)+Along*(J*760.f)
                + FVector(0,0,RockWallHeight*.48f), FVector(8.8f,5.4f,RockWallHeight*HeightFactor/100.f),
                FRotator((J%2)*6.f,Yaw+J*11.f,(I%3-1)*7.f),Color);
        }
    }
    // Low moss-toned shoulders frame, rather than block, the western entrance view.
    AddRock(TEXT("EntranceShoulderNorth"),FVector(-H+300,1500,430),FVector(8,7,5),FRotator(4,12,-8),FLinearColor(.25f,.29f,.20f));
    AddRock(TEXT("EntranceShoulderSouth"),FVector(-H+300,-1500,430),FVector(8,7,5),FRotator(-5,-8,7),FLinearColor(.25f,.29f,.20f));

    UDirectionalLightComponent* Sun = NewObject<UDirectionalLightComponent>(this,TEXT("BasinSun"));
    Sun->SetupAttachment(Root); Sun->SetRelativeRotation(FRotator(-48,-32,0)); Sun->SetIntensity(3.2f); Sun->SetMobility(EComponentMobility::Movable); Sun->RegisterComponent(); GeneratedComponents.Add(Sun);
    UPointLightComponent* Fill = NewObject<UPointLightComponent>(this,TEXT("BasinFill"));
    Fill->SetupAttachment(Root); Fill->SetRelativeLocation(FVector(0,0,1500)); Fill->SetIntensity(90000.f); Fill->SetAttenuationRadius(6200.f);
    Fill->SetCastShadows(false); Fill->SetLightColor(FLinearColor(.70f,.76f,.82f)); Fill->SetMobility(EComponentMobility::Movable); Fill->RegisterComponent(); GeneratedComponents.Add(Fill);
}
