#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnvironmentReviewCapture.generated.h"

/** Arranged visual evidence only; never used as a gameplay pass. */
UCLASS(NotBlueprintable)
class ISHIBASHIRIPROTOTYPE_API AEnvironmentReviewCapture : public AActor
{
    GENERATED_BODY()
public:
    AEnvironmentReviewCapture();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
private:
    void PlaceView();
    UPROPERTY() TObjectPtr<class ACameraActor> ReviewCamera;
    UPROPERTY() TObjectPtr<class APrototypePlayer> Player;
    TArray<FVector> PlayerLocations;
    TArray<FVector> CameraLocations;
    TArray<FVector> CameraTargets;
    TArray<FString> Names;
    FString Directory;
    FString RunId;
    int32 ViewIndex = 0;
    float Elapsed = 0.f;
    bool bRequested = false;
    bool bFinished = false;
};
