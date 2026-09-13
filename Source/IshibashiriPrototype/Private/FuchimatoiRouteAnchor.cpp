#include "FuchimatoiRouteAnchor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "PrimitiveAppearance.h"
#include "UObject/ConstructorHelpers.h"

AFuchimatoiRouteAnchor::AFuchimatoiRouteAnchor()
{
    PrimaryActorTick.bCanEverTick = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Anchor")));
    Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RouteMarker"));
    Marker->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Marker->SetStaticMesh(Mesh.Object);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    Marker->SetMaterial(0,Material.Object);
    Marker->SetRelativeLocation(FVector(0,0,-75));
    Marker->SetRelativeScale3D(FVector(.32));
    Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Marker->SetCanEverAffectNavigation(false);
}
void AFuchimatoiRouteAnchor::Configure(bool bRock, int32 Index)
{
    bRockAnchor = bRock;
    SetPrimitiveColor(Marker->CreateDynamicMaterialInstance(0), bRock ? FLinearColor(1,.7,.08) : FLinearColor(.1,.95,.9));
    Tags.Add(FName(*FString::Printf(TEXT("FuchimatoiRoute%d"),Index)));
}

void AFuchimatoiRouteAnchor::SetGuidance(bool bVisible, bool bHighlighted, bool bGrab)
{
    SetActorHiddenInGame(!bVisible);
    Marker->SetRelativeScale3D(FVector(bHighlighted ? .65f : .18f));
    const FLinearColor Color=bGrab ? FLinearColor(1,.8,.15)
        : bRockAnchor ? FLinearColor(1,.7,.08) : FLinearColor(.1,.95,.9);
    SetPrimitiveColor(Cast<UMaterialInstanceDynamic>(Marker->GetMaterial(0)),Color*(bHighlighted?1.f:.3f));
}
