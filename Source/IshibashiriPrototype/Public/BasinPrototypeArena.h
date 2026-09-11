#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BasinPrototypeArena.generated.h"

class UMaterialInterface;
class USceneComponent;
class UStaticMesh;

/** Deterministic, primitive-only staging terrain selected with -BasinPrototype. */
UCLASS(NotBlueprintable)
class ISHIBASHIRIPROTOTYPE_API ABasinPrototypeArena : public AActor
{
    GENERATED_BODY()

public:
    ABasinPrototypeArena();
    int32 GetVisualRockCount() const { return VisualRockCount; }
    int32 GetBoundaryCount() const { return BoundaryCount; }
    int32 GetAccentCount() const { return AccentCount; }

    UPROPERTY(EditAnywhere, Category="Basin", meta=(ClampMin="3000")) float ClearingHalfExtent = 4000.f;
    UPROPERTY(EditAnywhere, Category="Basin", meta=(ClampMin="400")) float RockWallHeight = 1350.f;
    UPROPERTY(EditAnywhere, Category="Basin") FVector PlayerStart = FVector(-3000.f, 0.f, 92.f);
    UPROPERTY(EditAnywhere, Category="Basin") FVector BossStart = FVector(550.f, 0.f, 352.f);

protected:
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    void AddRock(const TCHAR* Name, const FVector& Location, const FVector& Scale, const FRotator& Rotation,
        const FLinearColor& Color);
    void AddBoundary(const TCHAR* Name, const FVector& Location, const FVector& Scale, const FRotator& Rotation);
    void AddAccent(const TCHAR* Name, const FVector& Location, const FVector& Scale, const FRotator& Rotation,
        const FLinearColor& Color);
    void AddFloor();
    void ClearGeneratedComponents();

    UPROPERTY() TObjectPtr<USceneComponent> Root;
    UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> SphereMesh;
    UPROPERTY() TObjectPtr<UMaterialInterface> BaseMaterial;
    UPROPERTY(Transient) TArray<TObjectPtr<UActorComponent>> GeneratedComponents;
    int32 VisualRockCount = 0;
    int32 BoundaryCount = 0;
    int32 AccentCount = 0;
};
