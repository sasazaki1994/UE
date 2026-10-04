#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PrototypeArenaDressing.generated.h"

class UMaterialInterface;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMesh;

/** Render-only Ishibashiri environment around the campaign arena. The arena blocks keep all collision. */
UCLASS(NotBlueprintable)
class ISHIBASHIRIPROTOTYPE_API APrototypeArenaDressing : public AActor
{
    GENERATED_BODY()

public:
    APrototypeArenaDressing();
    void Dress(float HalfExtent, UPrimitiveComponent* Floor, const TArray<UPrimitiveComponent*>& Walls);
    bool IsDressed() const { return bDressed; }
    int32 GetRockCount() const { return RockCount; }
    int32 GetTreeCount() const { return TreeCount; }

private:
    void AddRockRim(float HalfExtent, UStaticMesh* Rock, UStaticMesh* RockB);
    void AddForest(float HalfExtent, UStaticMesh* Cedar, UStaticMesh* CedarB, UStaticMesh* Fern, UStaticMesh* FallenCedar);
    void AddAtmosphere();

    UPROPERTY() TObjectPtr<USceneComponent> Root;
    UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
    bool bDressed = false;
    int32 RockCount = 0;
    int32 TreeCount = 0;
};
