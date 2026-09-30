#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CommonSfxSubsystem.generated.h"

class UAudioComponent;
class UPlayerSenseComponent;
class USoundAttenuation;
class USoundWave;

UENUM()
enum class ECommonSfx : uint8
{
    BoundaryReading, ArmWarning, MountOpen, ClingWarning, KakonPurified, EncounterCalmed
};

// Presentation only: never changes progression or waits for an audio tail.
// Lifetime is one world; Reset also terminates sounds when an encounter retries.
UCLASS()
class ISHIBASHIRIPROTOTYPE_API UCommonSfxSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    UCommonSfxSubsystem();
    static UCommonSfxSubsystem* Get(const UObject* WorldContext);
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    bool Play(ECommonSfx Event, const UObject* Source = nullptr, const FVector& Location = FVector::ZeroVector);
    bool PlayOnce(ECommonSfx Event, const UObject* Source, const FVector& Location = FVector::ZeroVector);
    void ForgetSource(const UObject* Source);
    void Stop(ECommonSfx Event);
    void Reset();
    void UpdateSense(const UPlayerSenseComponent* Sense);

protected:
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
    bool StartSound(ECommonSfx Event, const FVector& Location);
    UPROPERTY() TArray<TObjectPtr<USoundWave>> Sounds;
    UPROPERTY() TObjectPtr<USoundAttenuation> WorldAttenuation;
    TArray<TWeakObjectPtr<UAudioComponent>> ActiveSounds;
    TMap<ECommonSfx, TWeakObjectPtr<UAudioComponent>> LastSound;
    TMap<ECommonSfx, double> LastPlayed;
    TMap<ECommonSfx, TSet<TWeakObjectPtr<const UObject>>> PlayedOnce;
    double NextBoundaryPulse = 0.0;
    uint8 PreviousArmWarning = 0;
};
