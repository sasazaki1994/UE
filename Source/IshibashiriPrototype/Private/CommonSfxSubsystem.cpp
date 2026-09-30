#include "CommonSfxSubsystem.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerSenseComponent.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWave.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    const TCHAR* Names[] = {TEXT("BoundaryReading"), TEXT("ArmWarning"), TEXT("MountOpen"),
        TEXT("ClingWarning"), TEXT("KakonPurified"), TEXT("EncounterCalmed")};
    constexpr float Volumes[] = {.42f, .58f, .85f, .72f, .85f, .70f};
    constexpr double Cooldowns[] = {.40, .35, .60, 1.0, .10, 1.5};
    TAutoConsoleVariable<float> CommonSfxVolume(TEXT("prototype.CommonSfxVolume"), 1.f,
        TEXT("Common cue volume, 0 (mute) to 1 (default). Does not affect progression."));
}

UCommonSfxSubsystem::UCommonSfxSubsystem()
{
    // Native hard references make the six SoundWaves discoverable by the cooker.
    for (const TCHAR* Name : Names)
    {
        const FString Path = FString::Printf(TEXT("/Game/Audio/CommonSfx/%s.%s"), Name, Name);
        ConstructorHelpers::FObjectFinderOptional<USoundWave> Sound(*Path);
        Sounds.Add(Sound.Get());
    }
}

UCommonSfxSubsystem* UCommonSfxSubsystem::Get(const UObject* WorldContext)
{
    UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
    return World ? World->GetSubsystem<UCommonSfxSubsystem>() : nullptr;
}

bool UCommonSfxSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{
    return Type == EWorldType::Game || Type == EWorldType::PIE || Type == EWorldType::GamePreview;
}

void UCommonSfxSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    WorldAttenuation = NewObject<USoundAttenuation>(this);
    auto& Settings = WorldAttenuation->Attenuation;
    Settings.bAttenuate = true;
    Settings.bSpatialize = true;
    Settings.DistanceAlgorithm = EAttenuationDistanceModel::Linear;
    Settings.AttenuationShape = EAttenuationShape::Sphere;
    Settings.AttenuationShapeExtents = FVector(1200.f, 0.f, 0.f);
    Settings.FalloffDistance = 6000.f;
    int32 Available = 0;
    for (USoundWave* Sound : Sounds) { Available += Sound != nullptr; }
    UE_LOG(LogTemp, Log, TEXT("COMMON_SFX assets=%d/6 world=%s"), Available, *GetWorld()->GetName());
}

void UCommonSfxSubsystem::Deinitialize()
{
    Reset();
    Super::Deinitialize();
}

bool UCommonSfxSubsystem::StartSound(ECommonSfx Event, const FVector& Location)
{
    const int32 Index = static_cast<int32>(Event);
    if (!GetWorld() || !Sounds.IsValidIndex(Index) || !Sounds[Index] || GetWorld()->GetNetMode() == NM_DedicatedServer)
    {
        return false;
    }
    const float Volume = Volumes[Index] * FMath::Clamp(CommonSfxVolume.GetValueOnGameThread(), 0.f, 1.f);
    if (Volume <= 0.f) { return false; }
    // Warnings belong to the player. Mount/purification identify a world source.
    const bool bSpatial = Event == ECommonSfx::MountOpen || Event == ECommonSfx::KakonPurified;
    UAudioComponent* Component = bSpatial
        ? UGameplayStatics::SpawnSoundAtLocation(this, Sounds[Index], Location, FRotator::ZeroRotator,
            Volume, 1.f, 0.f, WorldAttenuation, nullptr, true)
        : UGameplayStatics::SpawnSound2D(this, Sounds[Index], Volume, 1.f, 0.f, nullptr, false, true);
    UE_LOG(LogTemp, Log, TEXT("COMMON_SFX event=%s started=%d spatial=%d"), Names[Index], Component != nullptr, bSpatial);
    if (!Component) { return false; }
    // SpawnSound2D marks sounds as UI by default; gameplay cues should pause with the world.
    Component->bIsUISound = false;
    ActiveSounds.RemoveAll([](const TWeakObjectPtr<UAudioComponent>& Sound) { return !Sound.IsValid(); });
    ActiveSounds.Add(Component);
    LastSound.Add(Event, Component);
    return true;
}

bool UCommonSfxSubsystem::Play(ECommonSfx Event, const UObject* Source, const FVector& Location)
{
    const int32 Index = static_cast<int32>(Event);
    if (!GetWorld() || Index < 0 || Index >= UE_ARRAY_COUNT(Cooldowns)) { return false; }
    const double Now = GetWorld()->GetTimeSeconds();
    const double* Previous = LastPlayed.Find(Event);
    if (Previous && Now - *Previous < Cooldowns[Index]) { return false; }
    LastPlayed.Add(Event, Now);
    return StartSound(Event, Location);
}

bool UCommonSfxSubsystem::PlayOnce(ECommonSfx Event, const UObject* Source, const FVector& Location)
{
    if (!IsValid(Source)) { return false; }
    auto& Sources = PlayedOnce.FindOrAdd(Event);
    const TWeakObjectPtr<const UObject> Key(Source);
    if (Sources.Contains(Key)) { return false; }
    // Consume the presentation event even when muted or running without an audio device.
    Sources.Add(Key);
    return StartSound(Event, Location);
}

void UCommonSfxSubsystem::ForgetSource(const UObject* Source)
{
    if (!Source) { return; }
    const TWeakObjectPtr<const UObject> Key(Source);
    for (auto& Entry : PlayedOnce) { Entry.Value.Remove(Key); }
}

void UCommonSfxSubsystem::Stop(ECommonSfx Event)
{
    if (const auto* WeakSound = LastSound.Find(Event))
    {
        if (UAudioComponent* Sound = WeakSound->Get()) { Sound->Stop(); }
    }
    LastSound.Remove(Event);
}

void UCommonSfxSubsystem::Reset()
{
    for (const auto& WeakSound : ActiveSounds)
    {
        if (UAudioComponent* Sound = WeakSound.Get()) { Sound->Stop(); }
    }
    ActiveSounds.Reset();
    LastSound.Reset();
    LastPlayed.Reset();
    PlayedOnce.Reset();
    NextBoundaryPulse = 0.0;
    PreviousArmWarning = 0;
}

void UCommonSfxSubsystem::UpdateSense(const UPlayerSenseComponent* Sense)
{
    if (!GetWorld()) { return; }
    const double Now = GetWorld()->GetTimeSeconds();
    if (Sense && Sense->IsBoundarySenseActive() && Sense->GetBoundaryReading().Target.IsValid()
        && Sense->GetBoundaryReading().Strength > 0.f)
    {
        if (Now >= NextBoundaryPulse)
        {
            Play(ECommonSfx::BoundaryReading, Sense);
            NextBoundaryPulse = Now + FMath::Lerp(1.50, .48,
                static_cast<double>(FMath::Clamp(Sense->GetBoundaryReading().Strength, 0.f, 1.f)));
        }
    }
    else
    {
        Stop(ECommonSfx::BoundaryReading);
        NextBoundaryPulse = 0.0;
    }
    const uint8 Warning = Sense && Sense->IsCorruptionSenseActive()
        ? static_cast<uint8>(Sense->GetCorruptionWarning()) : 0;
    if (Warning != PreviousArmWarning)
    {
        Stop(ECommonSfx::ArmWarning);
        if (Warning != 0) { Play(ECommonSfx::ArmWarning, Sense); }
        PreviousArmWarning = Warning;
    }
}
