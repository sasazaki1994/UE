#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerSenseComponent.h"
#include "MagatsunePlayer.generated.h"
class AMagatsuneBoss; class UGrabComponent; class UStaminaComponent;
UCLASS()
class ISHIBASHIRIPROTOTYPE_API AMagatsunePlayer : public ACharacter
{
    GENERATED_BODY()
public:
    AMagatsunePlayer();
    virtual void Tick(float Dt) override; virtual void CalcCamera(float Dt,FMinimalViewInfo& View) override; virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    void ConfigureBoss(AMagatsuneBoss* Value); void ResetForEncounter(); void ResolveLargePulse(); void Fall(bool Exhausted=false);
    bool IsMounted() const; bool IsClinging() const { return bGripHeld&&IsMounted(); } bool IsRouteMoving() const { return Destination!=INDEX_NONE; }
    bool IsRecovering() const { return bRecovering; } int32 GetRouteNode() const { return Node; }
    UGrabComponent* GetGrab() const { return Grab; } UStaminaComponent* GetStamina() const { return Stamina; }
    UPlayerSenseComponent* GetSense() const { return Sense; }
    static FTransform SpawnTransform() { return FTransform(FRotator(0,0,0),FVector(-1050,-100,90)); }
    UPROPERTY(EditAnywhere,Category="Magatsune") float GrabRange=260, RouteSpeed=330, MinimumGrabStamina=20, PulseCost=10;
private:
    void Forward(float V); void Right(float V); void Turn(float V); void Look(float V); void GrabPressed(); void GrabReleased(){bGripHeld=false;} void JumpPressed(); void PurifyPressed(); void RetryPressed();
    void BoundarySensePressed(); void BoundarySenseReleased(); void ArmSensePressed(); void ArmSenseReleased();
    ECorruptionWarning ComputeCorruptionWarning() const;
    void UpdateSenseFromBoss();
    UPROPERTY() TObjectPtr<AMagatsuneBoss> Boss; UPROPERTY(VisibleAnywhere) TObjectPtr<UGrabComponent> Grab; UPROPERTY(VisibleAnywhere) TObjectPtr<UStaminaComponent> Stamina;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPlayerSenseComponent> Sense;
    int32 Node=INDEX_NONE,Destination=INDEX_NONE; float ForwardInput=0,MoveProgress=0,RouteDelay=0,RecoveryGrace=0; bool bGripHeld=false,bFalling=false,bRecovering=false;
};
