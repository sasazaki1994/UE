#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "PrototypeSmokeTest.generated.h"

class APrototypeGameMode;

// Spawned only with -PrototypeSmokeTest in a non-Shipping build.
UCLASS(NotBlueprintable, Transient)
class APrototypeSmokeTest : public AActor
{
    GENERATED_BODY()

public:
    APrototypeSmokeTest();
    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;

private:
    enum class EPhase : uint8 { InputCamera, Movement, Dodge, InputJump, InputLanding, InputAltDodge,
        Guard, ImmediateRetry, Win, VictoryCapture, VictoryRetry, Lose, DefeatCapture, DefeatRetry,
        CameraWall, CameraBoss, CameraAim, CameraRestore, CameraReobstruct, CameraReset, Done };
    bool Require(bool bCondition, const TCHAR* Message);
    void Next(EPhase NewPhase);
    void AimAtBoss();
    void Finish(bool bSuccess);
    void Capture(const TCHAR* Name);
    void SendKey(const FKey& Key, EInputEvent Event, float Amount = 1.f);
    void TapKey(const FKey& Key);
    void PlaceAtWall();
    bool CheckRaisedCamera();
    bool CheckPlayerFraming();

    UPROPERTY() TObjectPtr<APrototypeGameMode> Mode;
    EPhase Phase = EPhase::InputCamera;
    float Elapsed = 0.f;
    float TotalElapsed = 0.f;
    FVector OffsetBeforeCameraReturn = FVector::ZeroVector;
    FVector StartPosition = FVector::ZeroVector;
    FVector LockedDirection = FVector::ZeroVector;
    int32 Counters = 0;
    int32 LastHealth = 3;
    int32 LastBossState = -1;
    int32 ActorsBeforeRetry = 0;
    int32 MovementIndex = 0;
    int32 WallIndex = 0;
    bool bInputSent = false;
    FRotator StartViewRotation = FRotator::ZeroRotator;
    TArray<FKey> PendingKeyReleases;
    bool bSawDodgeImmunity = false;
    bool bSawLockedCharge = false;
    bool bTestedExtraCounter = false;
    bool bTestedCameraReobstruction = false;
    bool bCaptureScreenshots = false;
    int32 CaptureFramesRemaining = 0;
    FString PendingCapture;
    FString RunId;
};
