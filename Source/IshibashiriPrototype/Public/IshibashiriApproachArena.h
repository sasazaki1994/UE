#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IshibashiriApproachArena.generated.h"

class UMaterialInterface;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMesh;

/** Self-contained primitive blockout for the quiet, non-combat walk to Ishibashiri. */
UCLASS(NotBlueprintable)
class ISHIBASHIRIPROTOTYPE_API AIshibashiriApproachArena : public AActor
{
    GENERATED_BODY()
public:
    AIshibashiriApproachArena();
    virtual void Tick(float DeltaSeconds) override;
    void ObservePlayer(const FVector& PlayerLocation);
    FVector GetPlayerStart() const { return PathPoints[0] + FVector(0,0,92); }
    FVector GetBasinGate() const { return PathPoints.Last(); }
    const TArray<FVector>& GetPathPoints() const { return PathPoints; }
    int32 GetStage() const { return Stage; }
    int32 GetRevealCount() const { return RevealCount; }
    bool IsRevealVisible() const { return bRevealVisible; }
    bool IsGateReached() const { return bGateReached; }
    FString GetSubtitle() const { return Subtitle; }
    float GetPathLengthMeters() const { return PathLengthMeters; }

protected:
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    void ClearGenerated();
    UStaticMeshComponent* AddPrimitive(const TCHAR* Name,UStaticMesh* Mesh,const FVector& Location,
        const FVector& Scale,const FRotator& Rotation,const FLinearColor& Color,bool bCollision=false);
    void AddTree(const TCHAR* Name,const FVector& Location,float Height,bool bFallen=false);
    void SetStage(int32 NewStage);

    UPROPERTY() TObjectPtr<USceneComponent> Root;
    UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> SphereMesh;
    UPROPERTY() TObjectPtr<UMaterialInterface> BaseMaterial;
    UPROPERTY(Transient) TArray<TObjectPtr<UActorComponent>> Generated;
    UPROPERTY(Transient) TArray<TObjectPtr<UPrimitiveComponent>> RevealParts;
    TArray<FVector> PathPoints;
    FVector RevealOrigin=FVector(70400,8500,2100);
    float RevealElapsed=0.f;
    float SubtitleRemaining=0.f;
    float PathLengthMeters=0.f;
    int32 Stage=0;
    int32 RevealCount=0;
    bool bRevealVisible=false;
    bool bGateReached=false;
    FString Subtitle;
};
