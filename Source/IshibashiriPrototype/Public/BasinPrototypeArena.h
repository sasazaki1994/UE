#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BasinPrototypeArena.generated.h"

class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UPrimitiveComponent;

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
    int32 GetTreeCount() const { return TreeCount; }
    void SetCalmPresentation(bool bCalm);
    UPrimitiveComponent* GetFloorComponent() const { return BasinFloor; }

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
    void AddTree(const TCHAR* Name, const FVector& Location, float Height, float Width);
    void AddFloor();
    void ClearGeneratedComponents();

    UPROPERTY() TObjectPtr<USceneComponent> Root;
    UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> SphereMesh;
    UPROPERTY() TObjectPtr<UMaterialInterface> BaseMaterial;
    UPROPERTY(Transient) TArray<TObjectPtr<UActorComponent>> GeneratedComponents;
    UPROPERTY(Transient) TObjectPtr<UPrimitiveComponent> BasinFloor;
    int32 VisualRockCount = 0;
    int32 BoundaryCount = 0;
    int32 AccentCount = 0;
    int32 TreeCount = 0;
    UPROPERTY(Transient) TObjectPtr<class UExponentialHeightFogComponent> GroundFog;
    UPROPERTY(Transient) TObjectPtr<class UDirectionalLightComponent> KeyLight;
};
