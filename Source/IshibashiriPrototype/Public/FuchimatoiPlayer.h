#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FuchimatoiPlayer.generated.h"
class AFuchimatoiBoss;
class UGrabComponent;
class UStaminaComponent;
class USpringArmComponent;
class UCameraComponent;

// Primitive test pawn. Attachment and stamina use the existing shared components.
UCLASS()
class ISHIBASHIRIPROTOTYPE_API AFuchimatoiPlayer : public ACharacter
{
    GENERATED_BODY()
public:
    AFuchimatoiPlayer();
    virtual void Tick(float Dt) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    void ConfigureBoss(AFuchimatoiBoss* InBoss);
    void ResetForEncounter(const FTransform& Spawn);
    void StopEncounter();
    bool CanAct() const;
    bool ReceiveBite();
    bool IsMounted() const;
    bool IsDodging() const { return DodgeRemaining>0; }
    bool IsRouteMoving() const { return Destination!=INDEX_NONE; }
    int32 GetRouteNode() const { return Node; }
    int32 GetHealth() const { return Health; }
    UStaminaComponent* GetStamina() const { return Stamina; }
    UGrabComponent* GetGrab() const { return Grab; }
    UCameraComponent* GetCamera() const { return Camera; }
    UPROPERTY(EditAnywhere, Category="Fuchimatoi") float GrabRange = 440.f;
    UPROPERTY(EditAnywhere, Category="Fuchimatoi") float RouteSpeed = 300.f;
private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void Look(float Value);
    void TurnRate(float Value);
    void LookRate(float Value);
    void GrabPressed();
    void JumpPressed();
    void DodgePressed();
    void AttackPressed();
    void RetryPressed();
    void AdvanceRoute(float Dt);
    UPROPERTY() TObjectPtr<AFuchimatoiBoss> Boss;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UGrabComponent> Grab;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaminaComponent> Stamina;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> Arm;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    float ForwardInput=0, RightInput=0;
    float DodgeRemaining=0, DodgeCooldown=0, HitImmunity=0;
    FVector DodgeDirection=FVector::ZeroVector;
    int32 Health=3;
    int32 Node=INDEX_NONE, Destination=INDEX_NONE;
    float RouteProgress=0, RouteDelay=0;
};
