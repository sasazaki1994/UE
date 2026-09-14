#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerSenseComponent.generated.h"

UENUM()
enum class ESenseDistanceBand : uint8 { None, Near, Medium, Far };

UENUM()
enum class ECorruptionWarning : uint8 { None, Danger, Safe, Transition };

USTRUCT()
struct FBoundarySenseReading
{
    GENERATED_BODY()
    FVector Direction=FVector::ZeroVector;
    float Strength=0.f;
    ESenseDistanceBand Distance=ESenseDistanceBand::None;
    TWeakObjectPtr<AActor> Target;
};

struct FRegisteredSenseTarget
{
    TWeakObjectPtr<AActor> Actor;
    bool bAvailable=false;
};

// Player-owned, read-only sensing state. Encounter progression remains in Nushi/Kakon.
UCLASS(ClassGroup=(Player),meta=(BlueprintSpawnableComponent))
class ISHIBASHIRIPROTOTYPE_API UPlayerSenseComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPlayerSenseComponent();
    virtual void TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* ThisTick) override;
    void RegisterBoundaryTarget(AActor* Target,bool bAvailable=true);
    void SetBoundaryTargetAvailable(AActor* Target,bool bAvailable);
    void ClearBoundaryTargets();
    void BeginBoundarySense();
    void EndBoundarySense();
    void BeginCorruptionSense();
    void EndCorruptionSense();
    void SetCorruptionWarning(ECorruptionWarning Value) { Warning=Value; }
    void ResetSense();
    void NotifyPurifyAfterSense();
    bool IsBoundarySenseActive() const { return bBoundaryActive; }
    bool IsCorruptionSenseActive() const { return bCorruptionActive; }
    bool IsRecoveryRiskActive() const { return bCorruptionActive||RiskRemaining>0.f; }
    float GetRecoveryMultiplier() const { return IsRecoveryRiskActive()?RecoveryMultiplier:1.f; }
    float GetRiskRemaining() const { return RiskRemaining; }
    ECorruptionWarning GetCorruptionWarning() const { return bCorruptionActive?Warning:ECorruptionWarning::None; }
    const FBoundarySenseReading& GetBoundaryReading() const { return Reading; }
    FString GetBoundaryStrengthLabel() const;
    FString GetCorruptionWarningLabel() const;
    UPROPERTY(EditAnywhere,Category="Sense|Boundary",meta=(ClampMin="1")) float NearDistance=700.f;
    UPROPERTY(EditAnywhere,Category="Sense|Boundary",meta=(ClampMin="1")) float MediumDistance=1800.f;
    UPROPERTY(EditAnywhere,Category="Sense|Boundary",meta=(ClampMin="1")) float MaximumDistance=4000.f;
    UPROPERTY(EditAnywhere,Category="Sense|Corruption",meta=(ClampMin="0",ClampMax="1")) float RecoveryMultiplier=.5f;
    UPROPERTY(EditAnywhere,Category="Sense|Corruption",meta=(ClampMin="0")) float RiskTailSeconds=2.f;
private:
    TArray<FRegisteredSenseTarget> Targets;
    FBoundarySenseReading Reading;
    ECorruptionWarning Warning=ECorruptionWarning::None;
    float BoundaryDuration=0,CorruptionDuration=0,RiskRemaining=0;
    bool bBoundaryActive=false,bCorruptionActive=false,bStrongLogged=false,bDangerLogged=false;
    bool bSenseUsedSincePurify=false;
};
