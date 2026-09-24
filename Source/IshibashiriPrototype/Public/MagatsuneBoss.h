#pragma once
#include "CoreMinimal.h"
#include "NushiBase.h"
#include "MagatsuneBoss.generated.h"
class AMagatsunePlayer;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UPointLightComponent;

UENUM()
enum class EMagatsunePhase : uint8 { SurfaceRoot, RootRockRoute, FinalRise, Calming, Calm };

struct FMagatsuneTelemetry
{
    int32 GrabAttempts=0, GrabSuccesses=0, PhaseTransitions=0, KakonPurified=0, LargePulses=0,
        Falls=0, Recoveries=0, Exhaustions=0, Retries=0;
    float Elapsed=0, ClingSeconds=0, ClearSeconds=0;
};

// ANushiBase is reused only as the existing Kakon lifecycle host. Magatsune is not a fourth Nushi.
UCLASS()
class ISHIBASHIRIPROTOTYPE_API AMagatsuneBoss : public ANushiBase
{
    GENERATED_BODY()
public:
    AMagatsuneBoss();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void ResetNushi() override;
    void ConfigurePlayer(AMagatsunePlayer* Value) { Player=Value; }
    bool TryPurify();
    bool IsRouteNodeEnabled(int32 Node) const;
    bool IsLargePulse() const { return bLargePulse; }
    float GetAnticipation() const { return Anticipation; }
    float GetTransitionAlpha() const;
    FVector GetPresentationFocus() const;
    bool IsSafeNode(int32 Node) const { return Node==3||Node==7||Node==10; }
    int32 GetRecoveryNode() const;
    FVector GetRecoveryWorld() const;
    FVector GetRouteLocal(int32 Node) const;
    FVector GetRouteWorld(int32 Node) const;
    AActor* GetGrabFrame() const { return GrabFrame; }
    USceneComponent* GetMovingRoot() const { return MovingRoot; }
    AKakonActor* GetKakon(int32 Index) const;
    int32 GetKakonCount() const { return Kakons.Num(); }
    EMagatsunePhase GetPhase() const { return Phase; }
    FString GetPhaseLabel() const;
    FMagatsuneTelemetry Telemetry;
    void LogTelemetry(const TCHAR* Event) const;
    void RefreshSenseGuidance() { RefreshRoutes(); }
    static constexpr int32 RouteNodeCount=12;
    static constexpr int32 KakonNodes[3]={3,7,11};
    UPROPERTY(EditAnywhere,Category="Magatsune") float PulsePeriod=4.f;
    UPROPERTY(EditAnywhere,Category="Magatsune") float TransitionSeconds=2.5f;
    UPROPERTY(EditAnywhere,Category="Magatsune") float CalmSeconds=2.f;
private:
    UFUNCTION() void HandlePurified(AKakonActor* Kakon);
    // Colors are applied together in BeginPlay once dynamic material instances exist.
    void BuildPrimitive(UStaticMesh* Mesh,UMaterialInterface* Material,const TCHAR* Name,USceneComponent* Parent,FVector At,FVector Scale);
    void RefreshRoutes();
    UPROPERTY() TObjectPtr<USceneComponent> MovingRoot;
    UPROPERTY() TObjectPtr<UPointLightComponent> RootCueLight;
    UPROPERTY() TObjectPtr<AActor> GrabFrame;
    UPROPERTY() TObjectPtr<AMagatsunePlayer> Player;
    UPROPERTY() TArray<TObjectPtr<AKakonActor>> Kakons;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> RouteMarkers;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> RootVisuals;
    TArray<FVector> RootVisualBaseLocations;
    FTransform SpawnTransform;
    EMagatsunePhase Phase=EMagatsunePhase::SurfaceRoot;
    float PhaseTime=0, CalmTime=0, Anticipation=0;
    bool bLargePulse=false, bPulseResolved=false;
};
