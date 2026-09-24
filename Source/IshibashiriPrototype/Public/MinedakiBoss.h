#pragma once
#include "CoreMinimal.h"
#include "NushiBase.h"
#include "MinedakiBoss.generated.h"
class AMinedakiPlayer;
class UStaticMeshComponent;
class UPointLightComponent;

UENUM()
enum class EMinedakiActionState : uint8
{
    Grounded, PreparingClimb, ClimbingWall, Shaking, LedgeTransition, UpperPlatform,
    ArmBridgeTransition, ArmBridge, FinalTransition, FinalRoute, Calming, Calm
};

struct FMinedakiTelemetry
{
    int32 GrabAttempts=0, GrabSuccesses=0, NodesReached=0, WallStarts=0, Shakes=0, ShakeSuccesses=0,
        ShakeFailures=0, Falls=0, Exhaustions=0, UpperReached=0, KakonPurified=0,
        Phase1Starts=0, Phase1Completes=0, Phase2Starts=0, Phase2Completes=0,
        Phase3Starts=0, Phase3Completes=0, BodyRouteTransitions=0, RecoveryStarts=0, RecoverySuccesses=0;
    float Elapsed=0, ClingSeconds=0, CompletionSeconds=0;
};

// A moving stage: translation belongs to the actor, posture and authored routes to BodyRoot.
UCLASS()
class ISHIBASHIRIPROTOTYPE_API AMinedakiBoss : public ANushiBase
{
    GENERATED_BODY()
public:
    AMinedakiBoss();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void ResetNushi() override;
    void ConfigurePlayer(AMinedakiPlayer* InPlayer) { Player=InPlayer; }
    void NotifyRouteNode(int32 Node);
    void AdvanceWallClimb(float DeltaSeconds);
    bool TryPurifyKakon();
    bool IsEncounterComplete() const;
    bool IsSliceComplete() const;
    bool IsWallMoving() const;
    bool IsBodyTransitioning() const;
    bool IsRouteNodeEnabled(int32 Node) const;
    int32 GetMaximumRouteNode() const;
    int32 GetRecoveryNode() const;
    FVector GetRecoveryAnchorWorld() const;
    EMinedakiActionState GetActionState() const { return ActionState; }
    FString GetActionLabel() const;
    AActor* GetGrabFrame() const { return GrabFrame; }
    USceneComponent* GetBodyRoot() const { return BodyRoot; }
    AKakonActor* GetKakon(int32 Index) const;
    int32 GetKakonCount() const { return Kakons.Num(); }
    FVector GetRouteLocal(int32 Node) const;
    FVector GetRouteWorld(int32 Node) const;
    float GetClimbTime() const { return ClimbTime; }
    FMinedakiTelemetry Telemetry;
    void LogTelemetry(const TCHAR* Event) const;
    void RefreshSenseGuidance() { SetRouteVisibility(); }
    static constexpr int32 RouteNodeCount=14;
    static constexpr int32 WallStartNode=4;
    static constexpr int32 Kakon1Node=6;
    static constexpr int32 KakonNode=Kakon1Node;
    static constexpr int32 Kakon2Node=10;
    static constexpr int32 Kakon3Node=13;
    UPROPERTY(EditAnywhere,Category="Minedaki|Timing") float PrepareSeconds=2.f;
    UPROPERTY(EditAnywhere,Category="Minedaki|Timing") float WallSeconds=7.f;
    UPROPERTY(EditAnywhere,Category="Minedaki|Timing") float LedgeSeconds=3.f;
    UPROPERTY(EditAnywhere,Category="Minedaki|Timing") float RouteTransitionSeconds=3.f;
    UPROPERTY(EditAnywhere,Category="Minedaki|Timing") float CalmSeconds=2.f;
    UPROPERTY(EditAnywhere,Category="Minedaki|Pose") float PlateauHeight=2000.f;
    UPROPERTY(EditAnywhere,Category="Minedaki|Pose") float MaximumPitch=70.f;
    UPROPERTY(EditAnywhere,Category="Minedaki|Pose") float MaximumYaw=20.f;
    UPROPERTY(EditAnywhere,Category="Minedaki|Pose") float ShakeRoll=24.f;
private:
    UFUNCTION() void HandleKakonPurified(AKakonActor* Kakon);
    void AdvancePostKakon(float Dt);
    void BeginPhase(int32 Phase);
    void FinishTransition();
    void UpdateArmHolds();
    void SetRouteVisibility();
    UPROPERTY() TObjectPtr<USceneComponent> BodyRoot;
    UPROPERTY() TObjectPtr<UPointLightComponent> BodyCueLight;
    UPROPERTY() TObjectPtr<AActor> GrabFrame;
    UPROPERTY() TObjectPtr<AMinedakiPlayer> Player;
    UPROPERTY() TArray<TObjectPtr<AKakonActor>> Kakons;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> RouteMarkers;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> CoreMarkers;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> LeftArm;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> RightArm;
    EMinedakiActionState ActionState=EMinedakiActionState::Grounded;
    FTransform SpawnPose;
    float ClimbTime=0, TransitionTime=0, CalmTime=0;
    bool bShakeApplied=false, bTransitionShakeApplied=false;
};
