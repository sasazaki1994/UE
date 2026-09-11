#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ClimbingIntegrationTest.generated.h"
class APrototypeGameMode;

UCLASS()
class ISHIBASHIRIPROTOTYPE_API AClimbingIntegrationTest : public AActor
{
    GENERATED_BODY()
public:
    AClimbingIntegrationTest();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
private:
    void Hold(const FKey& Key, bool Down);
    void Tap(const FKey& Key);
    void Next(int32 Value);
    void SetupGrab();
    bool Check(bool Condition, const TCHAR* Description);
    void Shot(const TCHAR* Name);
    void TickGrabMotionWarp(float Dt);
    UPROPERTY() TObjectPtr<APrototypeGameMode> Mode;
    TSet<FKey> Held;
    TArray<FKey> Release;
    int32 Phase = 0;
    float Time = 0.f;
    float Total = 0.f;
    float BeforeStamina = 0.f;
    FVector BeforeBoss = FVector::ZeroVector;
    FVector BeforeFoot = FVector::ZeroVector;
    FString RunId;
    bool bFinished = false;
    bool bCapture = false;
    bool bGamepad = false;
    bool bClimbingIK = false;
    bool bGrabMotionWarp = false;
    int32 GrabWarpPhase = 0;
    int32 IKContactShot = 0;
    bool bIKShakeShot = false;
};
