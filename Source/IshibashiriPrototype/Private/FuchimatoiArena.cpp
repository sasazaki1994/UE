#include "FuchimatoiArena.h"
#include "FuchimatoiRouteAnchor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "PrimitiveAppearance.h"
#include "UObject/ConstructorHelpers.h"

AFuchimatoiArena::AFuchimatoiArena()
{
    PrimaryActorTick.bCanEverTick = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Arena")));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    auto Part = [this](const TCHAR* Name,FVector Position,FVector Scale,bool Collision)
    {
        UStaticMeshComponent* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Mesh->SetupAttachment(RootComponent); Mesh->SetStaticMesh(Cube.Object);
        Mesh->SetMaterial(0,Material.Object);
        Mesh->SetRelativeLocation(Position); Mesh->SetRelativeScale3D(Scale);
        Mesh->SetCollisionProfileName(Collision ? TEXT("BlockAll") : TEXT("NoCollision"));
        return Mesh;
    };
    Part(TEXT("CanyonFloor"),FVector(-700,0,-60),FVector(64,40,1.2),true);
    Part(TEXT("BlueWaterRegion"),FVector(-1700,0,2),FVector(26,23,.04),false);
    BaitRock=Part(TEXT("BiteBaitRock"),FVector(530,0,320),FVector(2.8,4.2,6.4),true);
    Part(TEXT("RestLedge"),FVector(-450,400,812),FVector(3.8,3.4,1),true);
    Part(TEXT("RockPillar"),FVector(220,430,631),FVector(3.6,3.4,12.62),true);
    Part(TEXT("FinalLedge"),FVector(-120,-520,1780),FVector(4,3,1),true);
    Part(TEXT("CanyonBack"),FVector(-700,1950,1050),FVector(64,1,22),true);
    Part(TEXT("CanyonEnd"),FVector(2450,0,900),FVector(1,40,19),true);
    Part(TEXT("BaitSpot"),FVector(140,0,4),FVector(1.9,1.9,.04),false);
}
void AFuchimatoiArena::BeginPlay()
{
    Super::BeginPlay();
    TArray<UStaticMeshComponent*> Parts; GetComponents(Parts);
    for (UStaticMeshComponent* Part : Parts)
    {
        FLinearColor Color(.24,.29,.32);
        if (Part==BaitRock) Color=FLinearColor(.55,.42,.22);
        else if (Part->GetName()==TEXT("BlueWaterRegion")) Color=FLinearColor(.025,.22,.45);
        else if (Part->GetName()==TEXT("BaitSpot")) Color=FLinearColor(1,.6,.02);
        else if (Part->GetName()==TEXT("CanyonFloor")) Color=FLinearColor(.42,.44,.38);
        SetPrimitiveColor(Part->CreateDynamicMaterialInstance(0),Color);
    }
    for (int32 I=0; I<2; ++I)
    {
        AFuchimatoiRouteAnchor* Anchor=GetWorld()->SpawnActor<AFuchimatoiRouteAnchor>();
        Anchor->AttachToActor(this,FAttachmentTransformRules::KeepWorldTransform);
        Anchor->SetActorLocation(I==0 ? FVector(-450,400,950) : FVector(220,430,1350));
        Anchor->Configure(true,I==0?4:6); RockAnchors.Add(Anchor);
    }
    ADirectionalLight* Sun=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,2500),FRotator(-45,-40,0));
    Sun->GetLightComponent()->SetIntensity(3.f);
    ADirectionalLight* Ambient=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,2500),FRotator(-25,140,0));
    Ambient->GetLightComponent()->SetIntensity(2.f);
    Ambient->GetLightComponent()->SetCastShadows(false);
    APointLight* Fill=GetWorld()->SpawnActor<APointLight>(FVector(-300,-1500,2300),FRotator::ZeroRotator);
    Fill->PointLightComponent->SetIntensity(1800000.f);
    Fill->PointLightComponent->SetAttenuationRadius(6500.f);
}
AFuchimatoiRouteAnchor* AFuchimatoiArena::GetRockAnchor(int32 Node) const
{
    return RockAnchors.Num()==2 ? (Node==4?RockAnchors[0].Get():Node==6?RockAnchors[1].Get():nullptr) : nullptr;
}
