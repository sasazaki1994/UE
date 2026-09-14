#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MinedakiPlayer.generated.h"
class AMinedakiBoss;
class UGrabComponent;
class UStaminaComponent;
class UPlayerSenseComponent;
UCLASS()
class ISHIBASHIRIPROTOTYPE_API AMinedakiPlayer : public ACharacter
{
    GENERATED_BODY()
public:
    AMinedakiPlayer();
    virtual void Tick(float Dt) override;
    virtual void CalcCamera(float Dt,FMinimalViewInfo& View) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    void ConfigureBoss(AMinedakiBoss* InBoss);
    void ResetForEncounter();
    void ResolveShake();
    void Fall(bool bExhausted=false);
    bool IsMounted() const;
    bool IsClinging() const { return bGripHeld && IsMounted(); }
    bool HasFallen() const { return bFallen; }
    bool IsRecovering() const { return bRecovering; }
    bool IsRouteMoving() const { return Destination!=INDEX_NONE; }
    int32 GetRouteNode() const { return Node; }
    UGrabComponent* GetGrab() const { return Grab; }
    UStaminaComponent* GetStamina() const { return Stamina; }
    UPlayerSenseComponent* GetSense() const { return Sense; }
    static FTransform SpawnTransform() { return FTransform(FRotator(0,180,0),FVector(1100,-205,90)); }
    UPROPERTY(EditAnywhere, Category="Minedaki", meta=(Units="cm")) float GrabRange=230.f;
    UPROPERTY(EditAnywhere, Category="Minedaki", meta=(Units="cm/s")) float RouteSpeed=300.f;
    UPROPERTY(EditAnywhere, Category="Minedaki|Stamina") float GrabDrain=2.f;
    UPROPERTY(EditAnywhere, Category="Minedaki|Stamina") float ClimbDrain=5.f;
    UPROPERTY(EditAnywhere, Category="Minedaki|Stamina") float WallDrain=3.f;
    UPROPERTY(EditAnywhere, Category="Minedaki|Stamina") float ClingExtraDrain=1.f;
    UPROPERTY(EditAnywhere, Category="Minedaki|Stamina") float RestRestore=22.f;
    UPROPERTY(EditAnywhere, Category="Minedaki|Stamina") float MinimumGrabStamina=25.f;
    UPROPERTY(EditAnywhere, Category="Minedaki|Stamina") float ShakeCost=8.f;
private:
    bool CanAct() const;
    void Forward(float Value);
    void Right(float Value);
    void Turn(float Value);
    void Look(float Value);
    void TurnRate(float Value);
    void LookRate(float Value);
    void GrabPressed();
    void GrabReleased() { bGripHeld=false; }
    void JumpPressed();
    void AttackPressed();
    void RetryPressed();
    void BoundarySensePressed(); void BoundarySenseReleased(); void ArmSensePressed(); void ArmSenseReleased();
    UPROPERTY() TObjectPtr<AMinedakiBoss> Boss;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UGrabComponent> Grab;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaminaComponent> Stamina;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPlayerSenseComponent> Sense;
    int32 Node=INDEX_NONE, Destination=INDEX_NONE;
    float ForwardInput=0, Progress=0, RouteDelay=0;
    bool bGripHeld=false, bFallen=false, bRecovering=false;
    float RecoveryGrace=0;
};
