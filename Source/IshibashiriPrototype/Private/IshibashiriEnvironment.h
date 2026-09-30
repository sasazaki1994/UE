#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"

// The primitive option keeps the same placement/collision for matched-camera reviews.
// Missing optional art always leaves the playable blockout intact.
namespace IshibashiriEnvironment
{
    template <typename T> T* Load(const TCHAR* Name)
    {
        if (FParse::Param(FCommandLine::Get(), TEXT("PrimitiveEnvironment"))) return nullptr;
        const FString Package = FString(TEXT("/Game/Environment/Ishibashiri/")) + Name;
        return FPackageName::DoesPackageExist(Package) ? LoadObject<T>(nullptr, *(Package + TEXT(".") + Name)) : nullptr;
    }

    inline FVector Fit(UStaticMesh* Mesh, const FVector& Size)
    {
        const FVector Bounds = Mesh->GetBounds().BoxExtent * 2.f;
        return FVector(Size.X / FMath::Max(1.f, Bounds.X), Size.Y / FMath::Max(1.f, Bounds.Y), Size.Z / FMath::Max(1.f, Bounds.Z));
    }

    inline UStaticMeshComponent* Visual(AActor* Owner, USceneComponent* Root, const TCHAR* Name,
        UStaticMesh* Mesh, const FVector& Base, const FVector& Scale, const FRotator& Rotation)
    {
        auto* C = NewObject<UStaticMeshComponent>(Owner, Name);
        C->SetupAttachment(Root);
        C->SetStaticMesh(Mesh);
        C->SetRelativeLocation(Base);
        C->SetRelativeRotation(Rotation);
        C->SetRelativeScale3D(Scale);
        // Gameplay surfaces are the original simple colliders, not the render mesh.
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetCullDistance(24000.f);
        C->RegisterComponent();
        return C;
    }
}
