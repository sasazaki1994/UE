#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FuchimatoiArena.generated.h"
class AFuchimatoiRouteAnchor;
class UStaticMeshComponent;
UCLASS()
class ISHIBASHIRIPROTOTYPE_API AFuchimatoiArena : public AActor
{
    GENERATED_BODY()
public:
    AFuchimatoiArena();
    virtual void BeginPlay() override;
    UStaticMeshComponent* GetBaitRock() const { return BaitRock; }
    AFuchimatoiRouteAnchor* GetRockAnchor(int32 Node) const;
    FVector GetBaitPosition() const { return FVector(140,0,92); }
    FTransform GetPlayerSpawn() const { return FTransform(FRotator(0,180,0), FVector(140,-550,92)); }
    FTransform GetBossSpawn() const { return FTransform(FRotator::ZeroRotator, FVector(-1000,0,220)); }
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> BaitRock;
    UPROPERTY() TArray<TObjectPtr<AFuchimatoiRouteAnchor>> RockAnchors;
};
