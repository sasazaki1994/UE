#pragma once
#include "CoreMinimal.h"
#include "NushiBase.h"
#include "MinedakiBoss.generated.h"
class AMinedakiPlayer;
class UStaticMeshComponent;

UENUM()
enum class EMinedakiActionState : uint8 { Grounded, PreparingClimb, ClimbingWall, Shaking, LedgeTransition, UpperPlatform };

struct FMinedakiTelemetry
{
    int32 GrabAttempts=0, GrabSuccesses=0, NodesReached=0, WallStarts=0, Shakes=0, Falls=0, Exhaustions=0, UpperReached=0, KakonPurified=0;
    float Elapsed=0, ClingSeconds=0, CompletionSeconds=0;
};

// A moving stage: translation belongs to the actor, posture to BodyRoot.
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
    bool TryPurifyFirstKakon();
    bool IsSliceComplete() const;
    bool IsWallMoving() const;
    EMinedakiActionState GetActionState() const { return ActionState; }
    FString GetActionLabel() const;
    AActor* GetGrabFrame() const { return GrabFrame; }
    USceneComponent* GetBodyRoot() const { return BodyRoot; }
    AKakonActor* GetKakon(int32 Index) const;
    FVector GetRouteLocal(int32 Node) const;
    FVector GetRouteWorld(int32 Node) const;
    float GetClimbTime() const { return ClimbTime; }
    FMinedakiTelemetry Telemetry;
    void LogTelemetry(const TCHAR* Event) const;
    static constexpr int32 RouteNodeCount=8;
    static constexpr int32 WallStartNode=4;
    static constexpr int32 KakonNode=6;
    UPROPERTY(EditAnywhere, Category="Minedaki|Timing", meta=(ClampMin="0.1", Units="s")) float PrepareSeconds=2.f;
    UPROPERTY(EditAnywhere, Category="Minedaki|Timing", meta=(ClampMin="0.1", Units="s")) float WallSeconds=7.f;
    UPROPERTY(EditAnywhere, Category="Minedaki|Timing", meta=(ClampMin="0.1", Units="s")) float LedgeSeconds=3.f;
    UPROPERTY(EditAnywhere, Category="Minedaki|Pose", meta=(Units="cm")) float PlateauHeight=2000.f;
    UPROPERTY(EditAnywhere, Category="Minedaki|Pose") float MaximumPitch=70.f;
    UPROPERTY(EditAnywhere, Category="Minedaki|Pose") float MaximumYaw=20.f;
    UPROPERTY(EditAnywhere, Category="Minedaki|Pose") float ShakeRoll=24.f;
private:
    UPROPERTY() TObjectPtr<USceneComponent> BodyRoot;
    UPROPERTY() TObjectPtr<AActor> GrabFrame;
    UPROPERTY() TObjectPtr<AMinedakiPlayer> Player;
    UPROPERTY() TArray<TObjectPtr<AKakonActor>> Kakons;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> RouteMarkers;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> CoreMarker;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> LeftArm;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> RightArm;
    EMinedakiActionState ActionState=EMinedakiActionState::Grounded;
    FTransform SpawnPose;
    float ClimbTime=0;
    bool bShakeApplied=false;
    void UpdateArmHolds();
};
