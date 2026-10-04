#pragma once

#include "CoreMinimal.h"
#include "Components/DecalComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
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
        const FVector Ratio(Size.X / FMath::Max(1.f, Bounds.X), Size.Y / FMath::Max(1.f, Bounds.Y), Size.Z / FMath::Max(1.f, Bounds.Z));
        // Scanned rock surfaces smear when stretched; cap the per-axis deviation from a uniform scale.
        const float Uniform = FMath::Pow(Ratio.X * Ratio.Y * Ratio.Z, 1.f / 3.f);
        return FVector(FMath::Clamp(Ratio.X, Uniform / 1.6f, Uniform * 1.6f), FMath::Clamp(Ratio.Y, Uniform / 1.6f, Uniform * 1.6f),
            FMath::Clamp(Ratio.Z, Uniform / 1.6f, Uniform * 1.6f));
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

    // Ground dressing only: no collision, no shadows, culled early so it never affects play or the camera.
    inline UInstancedStaticMeshComponent* Scatter(AActor* Owner, USceneComponent* Root, const TCHAR* Name,
        UStaticMesh* Mesh, const TArray<FTransform>& Instances, float CullDistance)
    {
        auto* C = NewObject<UInstancedStaticMeshComponent>(Owner, Name);
        C->SetupAttachment(Root);
        C->SetStaticMesh(Mesh);
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetCastShadow(false);
        C->SetCullDistances(CullDistance * .8f, CullDistance);
        C->RegisterComponent();
        C->AddInstances(Instances, false);
        return C;
    }

    // Ground marks only. HalfSize is (projection depth, half width, half length along Yaw).
    inline UDecalComponent* Decal(AActor* Owner, USceneComponent* Root, const TCHAR* Name, UMaterialInterface* Material,
        const FVector& Location, const FVector& HalfSize, float Yaw)
    {
        auto* C = NewObject<UDecalComponent>(Owner, Name);
        C->SetupAttachment(Root);
        C->SetDecalMaterial(Material);
        C->DecalSize = HalfSize;
        // Decals project along local X, so the box is pitched to face the ground.
        C->SetRelativeLocationAndRotation(Location, FRotator(-90.f, Yaw, 0.f));
        C->RegisterComponent();
        return C;
    }

    // Two scanned variants alternate by name so neighbouring copies differ but captures stay stable.
    inline UStaticMesh* Variant(const TCHAR* Name, UStaticMesh* A, UStaticMesh* B, uint32 EveryNth)
    {
        return B && FCrc::StrCrc32(Name) % EveryNth == 0 ? B : A;
    }

    // Deterministic placement jitter in [0, 1) so captures stay comparable between runs.
    inline float Hash01(int32 A, int32 B)
    {
        return (HashCombine(GetTypeHash(A), GetTypeHash(B * 7919 + 13)) % 10007) / 10007.f;
    }
}
