#include "BasinPrototypeArena.h"
#include "PrimitiveAppearance.h"
#include "PrototypeGameMode.h"
#include "IshibashiriBoss.h"
#include "Components/AudioComponent.h"
#include "Components/DecalComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "IshibashiriEnvironment.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ABasinPrototypeArena::ABasinPrototypeArena()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    CubeMesh = Cube.Object; SphereMesh = Sphere.Object; BaseMaterial = Material.Object;
}

void ABasinPrototypeArena::ClearGeneratedComponents()
{
    for (UActorComponent* Component : GeneratedComponents)
        if (IsValid(Component)) Component->DestroyComponent();
    GeneratedComponents.Reset(); BasinFloor = nullptr; GroundFog = nullptr; KeyLight = nullptr;
    VisualRockCount = BoundaryCount = AccentCount = TreeCount = 0;
    ChargeDust = nullptr; ChargePebbles = nullptr; ForestAudio = nullptr; RumbleAudio = nullptr;
    DustAges.Reset(); DustOrigins.Reset(); DustDirections.Reset();
    DustSpawnRemaining = 0.f; NextDust = 0; bCalmPresentation = false;
}

void ABasinPrototypeArena::AddTree(const TCHAR* Name, const FVector& Location, float Height, float Width)
{
    if (CedarMesh)
    {
        UStaticMesh* Art = IshibashiriEnvironment::Variant(Name, CedarMesh, CedarBMesh, 3);
        const float S = (1800.f + Height * .3f) / FMath::Max(1.f, Art->GetBounds().BoxExtent.Z * 2.f);
        GeneratedComponents.Add(IshibashiriEnvironment::Visual(this, Root, Name, Art, Location,
            FVector(S), FRotator(0, FCrc::StrCrc32(Name) % 360, 0)));
        ++TreeCount;
        return;
    }
    AddAccent(*FString::Printf(TEXT("%sTrunk"),Name),Location+FVector(0,0,Height*.5f),
        FVector(Width/100.f,Width/100.f,Height/100.f),FRotator::ZeroRotator,FLinearColor(.075f,.052f,.035f));
    AddRock(*FString::Printf(TEXT("%sCrown"),Name),Location+FVector(0,0,Height*.82f),
        FVector(Width*.028f,Width*.028f,Height*.008f),FRotator(0,17,0),FLinearColor(.065f,.095f,.055f));
    ++TreeCount;
}

void ABasinPrototypeArena::SetCalmPresentation(bool bCalm)
{
    if (GroundFog) { GroundFog->SetFogDensity(bCalm ? .008f : .012f); GroundFog->SetFogMaxOpacity(bCalm ? .16f : .22f); }
    bCalmPresentation = bCalm;
    if (KeyLight)
    {
        KeyLight->SetIntensity(bCalm ? 3.55f : 3.2f);
        KeyLight->SetLightColor(bCalm ? FLinearColor(1.f, .89f, .72f) : FLinearColor(.72f, .79f, .85f));
    }
    ResetChargePresentation();
    if (ForestAudio) ForestAudio->SetVolumeMultiplier(bCalm ? .45f : .12f);
}

void ABasinPrototypeArena::AddAccent(const TCHAR* Name, const FVector& Location, const FVector& Scale,
    const FRotator& Rotation, const FLinearColor& Color)
{
    UStaticMeshComponent* Accent = NewObject<UStaticMeshComponent>(this, Name);
    Accent->SetupAttachment(Root); Accent->SetStaticMesh(CubeMesh); Accent->SetRelativeLocation(Location);
    Accent->SetRelativeRotation(Rotation); Accent->SetRelativeScale3D(Scale);
    Accent->SetCollisionEnabled(ECollisionEnabled::NoCollision); Accent->SetMaterial(0, BaseMaterial); Accent->RegisterComponent();
    if (UMaterialInstanceDynamic* Mat = Accent->CreateDynamicMaterialInstance(0)) SetPrimitiveColor(Mat, Color);
    GeneratedComponents.Add(Accent); ++AccentCount;
}

void ABasinPrototypeArena::AddFloor()
{
    UStaticMeshComponent* Floor = NewObject<UStaticMeshComponent>(this, TEXT("BasinFloor"));
    Floor->SetupAttachment(Root); Floor->SetStaticMesh(CubeMesh);
    Floor->SetRelativeLocation(FVector::ZeroVector + FVector(0,0,-50));
    Floor->SetRelativeScale3D(FVector((ClearingHalfExtent * 2.f + 600.f) / 100.f, (ClearingHalfExtent * 2.f + 600.f) / 100.f, 1.f));
    Floor->SetCollisionProfileName(TEXT("BlockAll")); Floor->SetMaterial(0, BaseMaterial); Floor->RegisterComponent();
    if (UMaterialInstanceDynamic* Mat = Floor->CreateDynamicMaterialInstance(0)) SetPrimitiveColor(Mat, FLinearColor(.25f,.23f,.19f));
    if (GroundMaterial) Floor->SetMaterial(0, GroundMaterial);
    GeneratedComponents.Add(Floor);
    BasinFloor = Floor;
}

void ABasinPrototypeArena::AddRock(const TCHAR* Name, const FVector& Location, const FVector& Scale,
    const FRotator& Rotation, const FLinearColor& Color)
{
    if (RockMesh && !FString(Name).EndsWith(TEXT("Crown")))
    {
        const FVector Size = Scale * 100.f;
        // One scanned rock repeats along the wall, so each copy gets its own facing.
        const FRotator ArtRotation = Rotation + FRotator(0, FCrc::StrCrc32(Name) % 360, 0);
        UStaticMesh* Art = IshibashiriEnvironment::Variant(Name, RockMesh, RockBMesh, 2);
        auto* Visual = IshibashiriEnvironment::Visual(this, Root, Name, Art,
            Location - Rotation.RotateVector(FVector(0, 0, Size.Z * .5f)), IshibashiriEnvironment::Fit(Art, Size), ArtRotation);
        for (int32 Slot = 0; Slot < Visual->GetNumMaterials(); ++Slot)
            if (auto* Mat = Visual->CreateDynamicMaterialInstance(Slot))
                Mat->SetVectorParameterValue(TEXT("Tint"), FLinearColor(.62f, .72f, .65f));
        GeneratedComponents.Add(Visual);
        ++VisualRockCount;
        return;
    }
    UStaticMeshComponent* Rock = NewObject<UStaticMeshComponent>(this, Name);
    Rock->SetupAttachment(Root); Rock->SetStaticMesh(SphereMesh); Rock->SetRelativeLocation(Location);
    Rock->SetRelativeRotation(Rotation); Rock->SetRelativeScale3D(Scale); Rock->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Rock->SetMaterial(0, BaseMaterial); Rock->RegisterComponent();
    if (UMaterialInstanceDynamic* Mat = Rock->CreateDynamicMaterialInstance(0)) SetPrimitiveColor(Mat, Color);
    GeneratedComponents.Add(Rock); ++VisualRockCount;
}

void ABasinPrototypeArena::AddBoundary(const TCHAR* Name, const FVector& Location, const FVector& Scale, const FRotator& Rotation)
{
    UStaticMeshComponent* Boundary = NewObject<UStaticMeshComponent>(this, Name);
    Boundary->SetupAttachment(Root); Boundary->SetStaticMesh(CubeMesh); Boundary->SetRelativeLocation(Location);
    Boundary->SetRelativeRotation(Rotation); Boundary->SetRelativeScale3D(Scale); Boundary->SetVisibility(false);
    Boundary->SetCollisionProfileName(TEXT("BlockAll")); Boundary->SetCollisionObjectType(ECC_WorldStatic);
    Boundary->RegisterComponent(); GeneratedComponents.Add(Boundary); ++BoundaryCount;
}

void ABasinPrototypeArena::AddGroundDressing()
{
    using IshibashiriEnvironment::Hash01;
    const float H = ClearingHalfExtent;
    TArray<FTransform> Ferns, Stones;
    // Dressing stays outside the clearing radius so the dodge space and camera sweeps are untouched.
    for (int32 I = 0; I < 140; ++I)
    {
        const FVector Out = FRotator(0, (I + Hash01(I, 1)) * 360.f / 140.f, 0).Vector();
        Ferns.Add(FTransform(FRotator(0, Hash01(I, 2) * 360.f, 0), Out * (H + 150.f + Hash01(I, 3) * 650.f) + FVector(0, 0, -28),
            FVector(1.3f + Hash01(I, 4) * 1.2f)));
        if (I % 4 == 0)
            Stones.Add(FTransform(FRotator(0, Hash01(I, 5) * 360.f, Hash01(I, 6) * 16.f - 8.f),
                Out * (H + 80.f + Hash01(I, 7) * 500.f) + FVector(0, 0, -30), FVector(.18f + Hash01(I, 8) * .3f)));
    }
    if (CedarMesh)
    {
        TArray<FTransform> Distant, DistantB;
        for (int32 I = 0; I < 72; ++I)
        {
            const FVector Out = FRotator(0, (I + Hash01(I, 9)) * 5.f, 0).Vector();
            const float Height = 1900.f + Hash01(I, 10) * 900.f;
            UStaticMesh* Mesh = CedarBMesh && I % 3 == 1 ? CedarBMesh.Get() : CedarMesh.Get();
            (Mesh == CedarMesh ? Distant : DistantB).Add(FTransform(FRotator(0, Hash01(I, 11) * 360.f, 0),
                Out * (H + 1900.f + Hash01(I, 12) * 3600.f) + FVector(0, 0, -30),
                FVector(Height / FMath::Max(1.f, Mesh->GetBounds().BoxExtent.Z * 2.f))));
        }
        GeneratedComponents.Add(IshibashiriEnvironment::Scatter(this, Root, TEXT("BasinDistantCedars"), CedarMesh, Distant, 30000.f));
        if (CedarBMesh)
            GeneratedComponents.Add(
                IshibashiriEnvironment::Scatter(this, Root, TEXT("BasinDistantCedarsB"), CedarBMesh, DistantB, 30000.f));
    }
    if (FernMesh) GeneratedComponents.Add(IshibashiriEnvironment::Scatter(this, Root, TEXT("BasinFerns"), FernMesh, Ferns, 9000.f));
    if (RockMesh) GeneratedComponents.Add(IshibashiriEnvironment::Scatter(this, Root, TEXT("BasinStones"), RockMesh, Stones, 12000.f));
    if (!FallenCedarMesh) return;
    const float Yaws[] = {28.f, 152.f, 241.f};
    for (int32 I = 0; I < UE_ARRAY_COUNT(Yaws); ++I)
    {
        const FVector Out = FRotator(0, Yaws[I], 0).Vector();
        GeneratedComponents.Add(IshibashiriEnvironment::Visual(this, Root, *FString::Printf(TEXT("BasinFallenCedar%02d"), I),
            FallenCedarMesh, Out * (H + 520.f) + FVector(0, 0, -20), FVector(.9f + I * .1f), FRotator(0, Yaws[I] + 90.f, 0)));
    }
}

void ABasinPrototypeArena::AddGroundHistory()
{
    using IshibashiriEnvironment::Hash01;
    // Thin projections touch only Z=0; no raised geometry, collision or navigation edits.
    for (int32 I = 0; I < 10; ++I)
    {
        const FVector Print(-2650.f + I * 530.f, -850.f + ((I & 1) ? 240.f : -240.f), 0.f);
        if (FootprintDecal)
            GeneratedComponents.Add(IshibashiriEnvironment::Decal(this, Root, *FString::Printf(TEXT("BasinHoofprint%02d"), I),
                FootprintDecal, Print, FVector(4.f, 145.f, 240.f), 8.f + I * 2.f));
    }
    for (int32 I = 0; I < 18; ++I)
    {
        const FVector Out = FRotator(0, I * 20.f + 6.f, 0).Vector();
        const FVector Location = Out * (ClearingHalfExtent - 420.f) + FVector(0, 0, .5f);
        UMaterialInterface* Material = I % 3 == 0 ? WetSoilDecal.Get() : MossDecal.Get();
        if (Material)
            GeneratedComponents.Add(IshibashiriEnvironment::Decal(this, Root, *FString::Printf(TEXT("BasinSoilPatch%02d"), I), Material,
                Location, FVector(4.f, 350.f + Hash01(I, 41) * 250.f, 520.f), I * 20.f));
        if (CrackDecal && I % 3 == 0)
            GeneratedComponents.Add(IshibashiriEnvironment::Decal(this, Root, *FString::Printf(TEXT("BasinCorruption%02d"), I), CrackDecal,
                Out * (ClearingHalfExtent - 180.f), FVector(4.f, 190.f, 260.f), I * 20.f + 37.f));
    }
    for (int32 I = 0; I < 5; ++I)
        if (WetSoilDecal)
            GeneratedComponents.Add(IshibashiriEnvironment::Decal(this, Root, *FString::Printf(TEXT("BasinTrampledSoil%02d"), I),
                WetSoilDecal, FVector(-1900.f + I * 850.f, -800.f + (I & 1) * 400.f, 0), FVector(4.f, 310.f, 700.f), 14.f));
}

void ABasinPrototypeArena::AddWallDressing()
{
    using IshibashiriEnvironment::Hash01;
    // Low talus binds the rock silhouettes outside the ground-camera retreat band.
    TArray<FTransform> Talus, TalusB, WallFerns;
    for (int32 I = 0; I < 64; ++I)
    {
        const float Yaw = I * 360.f / 64.f + Hash01(I, 51) * 3.f;
        const FVector Out = FRotator(0, Yaw, 0).Vector();
        const FVector Base = Out * (ClearingHalfExtent + 1300.f + Hash01(I, 52) * 620.f) + FVector(0, 0, -55.f);
        (RockBMesh && I % 3 == 0 ? TalusB : Talus)
            .Add(FTransform(FRotator(Hash01(I, 53) * 18.f, Yaw + Hash01(I, 54) * 90.f, Hash01(I, 55) * 15.f), Base,
                FVector(1.4f + Hash01(I, 56) * 1.5f)));
        WallFerns.Add(FTransform(
            FRotator(0, Yaw + 90.f, 0), Out * (ClearingHalfExtent + 240.f + Hash01(I, 57) * 250.f), FVector(1.2f + Hash01(I, 58))));
    }
    if (RockMesh) GeneratedComponents.Add(IshibashiriEnvironment::Scatter(this, Root, TEXT("BasinTalus"), RockMesh, Talus, 16000.f));
    if (RockBMesh) GeneratedComponents.Add(IshibashiriEnvironment::Scatter(this, Root, TEXT("BasinTalusB"), RockBMesh, TalusB, 16000.f));
    if (FernMesh)
        GeneratedComponents.Add(IshibashiriEnvironment::Scatter(this, Root, TEXT("BasinWallFerns"), FernMesh, WallFerns, 10000.f));
    // A second, lower ridge breaks the exposed horizon. It adds scenery, never playable land.
    for (int32 I = 0; I < 16; ++I)
    {
        const float Yaw = I * 22.5f + 9.f;
        AddRock(*FString::Printf(TEXT("BasinDistantRidge%02d"), I),
            FRotator(0, Yaw, 0).Vector() * (ClearingHalfExtent + 3300.f) + FVector(0, 0, 740.f),
            FVector(24.f, 18.f, 18.f + Hash01(I, 59) * 9.f), FRotator(4.f, Yaw, -7.f), FLinearColor(.20f, .25f, .22f));
    }
}

void ABasinPrototypeArena::AddRitualPiece(
    const TCHAR* Name, UStaticMesh* Mesh, const FVector& Base, const FVector& Size, const FRotator& Rotation)
{
    if (!Mesh) return;
    GeneratedComponents.Add(
        IshibashiriEnvironment::Visual(this, Root, Name, Mesh, Base, IshibashiriEnvironment::Fit(Mesh, Size), Rotation));
    ++AccentCount;
}

void ABasinPrototypeArena::AddRitualRemnants()
{
    // Optional production meshes have independent base-centre pivots and NoCollision.
    // No Tripo models are supplied here. Existing art and explicitly named proxies remain usable.
    const float H = ClearingHalfExtent;
    UStaticMesh* Torii = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_CollapsedTorii_A"));
    UStaticMesh* Pillar = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_ShimenawaPillar_A"));
    UStaticMesh* Shrine = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_SmallShrine_A"));
    UStaticMesh* Sacred = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_SacredStone_A"));
    UStaticMesh* Lantern = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_StoneLantern_A"));
    UStaticMesh* Stele = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_Stele_A"));
    const FVector GateBase(-H - 200.f, 1750.f, 0.f);
    if (Torii) AddRitualPiece(TEXT("KoshiCollapsedTorii"), Torii, GateBase, FVector(1050, 380, 650), FRotator(0, -18, 0));
    else
    {
        AddRitualPiece(
            TEXT("KoshiToriiStandingPostProxy"), RitualPostMesh, GateBase + FVector(0, -420, 0), FVector(50, 50, 520), FRotator(0, 0, -11));
        AddRitualPiece(
            TEXT("KoshiToriiBrokenPostProxy"), RitualPostMesh, GateBase + FVector(0, 420, 0), FVector(50, 50, 280), FRotator(-12, 0, 18));
        AddRitualPiece(TEXT("KoshiToriiFallenBeamProxy"), FallenCedarMesh, GateBase + FVector(-200, -470, 0), FVector(1100, 100, 100),
            FRotator(0, 78, 0));
    }
    const FVector PillarBase(1650.f, H + 180.f, 0);
    if (Pillar) AddRitualPiece(TEXT("KoshiShimenawaPillar"), Pillar, PillarBase, FVector(180, 160, 420), FRotator(0, -20, 0));
    else
    {
        AddRitualPiece(TEXT("KoshiPillarStoneProxy"), BoundaryMesh, PillarBase, FVector(140, 110, 380), FRotator(0, -20, 8));
        // The existing rope is a sagging segment, not a wrapped pillar. Keep that distinction visible.
        AddRitualPiece(
            TEXT("KoshiPillarCompanionProxy"), BoundaryMesh, PillarBase + FVector(900, 0, 0), FVector(90, 70, 230), FRotator(0, 14, -10));
        if (RopeMesh)
            GeneratedComponents.Add(IshibashiriEnvironment::Visual(this, Root, TEXT("KoshiPillarRopeProxy"), RopeMesh,
                PillarBase + FVector(0, 0, 215), FVector(.56f, 1, 1), FRotator::ZeroRotator));
    }
    const FVector ShrineBase(H + 140.f, -1350.f, 0);
    if (Shrine) AddRitualPiece(TEXT("KoshiSmallShrine"), Shrine, ShrineBase, FVector(240, 210, 290), FRotator(0, 180, 0));
    else
    {
        AddRitualPiece(TEXT("KoshiShrinePlinthProxy"), SteppingStoneMesh, ShrineBase, FVector(330, 240, 25), FRotator(0, 90, 0));
        AddAccent(TEXT("KoshiShrineBodyProxy"), ShrineBase + FVector(0, 0, 115), FVector(1.5f, 1.6f, 1.8f), FRotator::ZeroRotator,
            FLinearColor(.14f, .17f, .12f));
        for (int32 Side : {-1, 1})
            AddAccent(*FString::Printf(TEXT("KoshiShrineRoofProxy%d"), Side), ShrineBase + FVector(0, Side * 55, 225),
                FVector(2.5f, 1.5f, .16f), FRotator(0, 0, Side * 25.f), FLinearColor(.10f, .13f, .09f));
    }
    AddRitualPiece(TEXT("KoshiSacredStone"), Sacred ? Sacred : RockBMesh.Get(), FVector(2950, -H - 150.f, 0), FVector(700, 520, 780),
        FRotator(0, 37, 0));
    if (Lantern)
        AddRitualPiece(TEXT("KoshiStoneLantern"), Lantern, FVector(-1650, -H - 120.f, 0), FVector(110, 110, 185), FRotator(0, 10, 0));
    // A fallen boundary stone is an honest stele proxy, without invented inscriptions.
    AddRitualPiece(TEXT("KoshiSteleProxy"), Stele ? Stele : BoundaryMesh.Get(), FVector(-2650, H + 160.f, 0), FVector(80, 55, 190),
        FRotator(0, 24, -18));
    UE_LOG(LogTemp, Display, TEXT("KOSHI_REMNANTS torii=%d pillar=%d shrine=%d sacred=%d lantern=%d stele=%d tripo=NOT_RUN"),
        Torii != nullptr, Pillar != nullptr, Shrine != nullptr, Sacred != nullptr, Lantern != nullptr, Stele != nullptr);
}

void ABasinPrototypeArena::CreateChargePresentation()
{
    auto* DustMaterial = IshibashiriEnvironment::Load<UMaterialInterface>(TEXT("M_Ishibashiri_BasinDust"));
    // A fixed pool avoids spawning actors/components every frame or Retry.
    TArray<FTransform> Hidden;
    Hidden.Init(FTransform(FQuat::Identity, FVector(0, 0, -100), FVector::ZeroVector), 16);
    if (DustMaterial)
    {
        ChargeDust = IshibashiriEnvironment::Scatter(this, Root, TEXT("BasinChargeDust"), SphereMesh, Hidden, 7000.f);
        ChargeDust->SetMobility(EComponentMobility::Movable);
        ChargeDust->SetMaterial(0, DustMaterial);
        ChargeDust->SetNumCustomDataFloats(1);
        GeneratedComponents.Add(ChargeDust);
    }
    if (RockMesh)
    {
        ChargePebbles = IshibashiriEnvironment::Scatter(this, Root, TEXT("BasinChargePebbles"), RockMesh, Hidden, 5000.f);
        ChargePebbles->SetMobility(EComponentMobility::Movable);
        GeneratedComponents.Add(ChargePebbles);
    }
    DustAges.Init(1.f, 16);
    DustOrigins.Init(FVector::ZeroVector, 16);
    DustDirections.Init(FVector::ZeroVector, 16);
    const TCHAR* Names[] = {TEXT("A_Ishibashiri_ForestAmbience"), TEXT("A_Ishibashiri_ChargeRumble")};
    for (int32 I = 0; I < UE_ARRAY_COUNT(Names); ++I)
    {
        auto* Sound = IshibashiriEnvironment::Load<USoundBase>(Names[I]);
        if (!Sound) continue;
        auto* Audio = NewObject<UAudioComponent>(this, Names[I]);
        Audio->SetupAttachment(Root);
        Audio->bAutoActivate = false;
        Audio->SetSound(Sound);
        Audio->SetVolumeMultiplier(I == 0 ? .12f : 0.f);
        Audio->RegisterComponent();
        GeneratedComponents.Add(Audio);
        if (I == 0) ForestAudio = Audio;
        else RumbleAudio = Audio;
    }
    UE_LOG(LogTemp, Display, TEXT("KOSHI_PRESENTATION dust=%d pebbles=%d forestAudio=%d rumbleAudio=%d audio_missing=ASSET_REQUIRED"),
        ChargeDust != nullptr, ChargePebbles != nullptr, ForestAudio != nullptr, RumbleAudio != nullptr);
    ResetChargePresentation();
}

void ABasinPrototypeArena::ResetChargePresentation()
{
    DustSpawnRemaining = 0.f;
    NextDust = 0;
    for (float& Age : DustAges) Age = 1.f;
    if (ChargeDust) ChargeDust->SetVisibility(false);
    if (ChargePebbles) ChargePebbles->SetVisibility(false);
    if (RumbleAudio) RumbleAudio->Stop();
}

void ABasinPrototypeArena::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const auto* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<APrototypeGameMode>() : nullptr;
    const auto* Boss = Mode ? Mode->GetBoss() : nullptr;
    const bool bCharging =
        !bCalmPresentation && Mode && Mode->IsEncounterActive() && IsValid(Boss) && Boss->GetState() == EIshibashiriState::Charge;
    if (ForestAudio && !ForestAudio->IsPlaying()) ForestAudio->Play();
    if (RumbleAudio)
    {
        if (bCharging && !RumbleAudio->IsPlaying()) RumbleAudio->Play();
        RumbleAudio->SetVolumeMultiplier(bCharging ? .35f : 0.f);
        if (!bCharging && RumbleAudio->IsPlaying()) RumbleAudio->Stop();
    }
    if ((!ChargeDust && !ChargePebbles) || DustAges.IsEmpty()) return;
    if (bCalmPresentation || !Mode || !Mode->IsEncounterActive())
    {
        ResetChargePresentation();
        return;
    }
    const float Step = FMath::Max(0.f, DeltaSeconds);
    for (float& Age : DustAges) Age = FMath::Min(1.f, Age + Step);
    if (!bCharging && !DustAges.ContainsByPredicate([](float Age) { return Age < .65f; }))
    {
        ResetChargePresentation();
        return;
    }
    DustSpawnRemaining -= Step;
    if (bCharging && DustSpawnRemaining <= 0.f)
    {
        // Read the charge pose and trail the imported boar's rear feet.
        // Only visual component transforms are written.
        DustSpawnRemaining = .06f;
        const FVector Direction = Boss->GetChargeDirection();
        const FVector Side(-Direction.Y, Direction.X, 0);
        DustOrigins[NextDust] = Boss->GetActorLocation() - Direction * 480.f + Side * ((NextDust & 1) ? 290.f : -290.f);
        DustOrigins[NextDust].Z = 35.f;
        DustDirections[NextDust] = -Direction * 170.f + Side * ((NextDust & 1) ? 90.f : -90.f);
        DustAges[NextDust] = 0.f;
        NextDust = (NextDust + 1) % DustAges.Num();
    }
    bool bAnyVisible = false;
    for (int32 I = 0; I < DustAges.Num(); ++I)
    {
        const float Age = DustAges[I];
        const bool bVisible = Age < .65f;
        bAnyVisible |= bVisible;
        if (ChargeDust)
        {
            const float Size = bVisible ? 1.1f + Age * 3.f : 0.f;
            ChargeDust->UpdateInstanceTransform(I,
                FTransform(FQuat::Identity, DustOrigins[I] + DustDirections[I] * Age + FVector(0, 0, Age * 95.f),
                    FVector(Size, Size, Size * .38f)),
                true, I == DustAges.Num() - 1, true);
            ChargeDust->SetCustomDataValue(I, 0, bVisible ? 1.f - Age / .65f : 0.f, I == DustAges.Num() - 1);
        }
        if (ChargePebbles)
        {
            const float Lift = FMath::Max(0.f, 170.f * Age - 420.f * Age * Age);
            ChargePebbles->UpdateInstanceTransform(I,
                FTransform(FRotator(Age * 180.f, I * 43.f, 0), DustOrigins[I] + DustDirections[I] * Age + FVector(0, 0, Lift - 28.f),
                    FVector(bVisible ? .022f : 0.f)),
                true, I == DustAges.Num() - 1, true);
        }
    }
    if (ChargeDust) ChargeDust->SetVisibility(bAnyVisible);
    if (ChargePebbles) ChargePebbles->SetVisibility(bAnyVisible);
}

void ABasinPrototypeArena::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform); ClearGeneratedComponents();
    CedarMesh = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_OldCedar_A"));
    RockMesh = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_Rock_A"));
    BoundaryMesh = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_BoundaryStone_A"));
    FallenCedarMesh = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_FallenCedar_A"));
    FernMesh = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_Fern_A"));
    CedarBMesh = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_OldCedar_B"));
    RockBMesh = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_Rock_B"));
    RitualPostMesh = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_RitualPost_A"));
    RopeMesh = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_OldRope_A"));
    SteppingStoneMesh = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_SteppingStone_A"));
    GroundMaterial = IshibashiriEnvironment::Load<UMaterialInterface>(TEXT("M_Ishibashiri_Ground"));
    FootprintDecal = IshibashiriEnvironment::Load<UMaterialInterface>(TEXT("D_Ishibashiri_BasinFootprint"));
    CrackDecal = IshibashiriEnvironment::Load<UMaterialInterface>(TEXT("D_Ishibashiri_CorruptionCrack_A"));
    WetSoilDecal = IshibashiriEnvironment::Load<UMaterialInterface>(TEXT("D_Ishibashiri_BasinWetSoil"));
    MossDecal = IshibashiriEnvironment::Load<UMaterialInterface>(TEXT("D_Ishibashiri_BasinMoss"));
    AddFloor();
    if (GroundMaterial)
        if (auto* Floor = Cast<UStaticMeshComponent>(BasinFloor))
            if (auto* Mat = Floor->CreateDynamicMaterialInstance(0))
                Mat->SetVectorParameterValue(TEXT("Tint"), FLinearColor(.72f, .78f, .69f));
    // Decorative ground under the outer cedars; the playable floor stays unchanged.
    auto* ForestGround = NewObject<UStaticMeshComponent>(this, TEXT("BasinForestGround"));
    ForestGround->SetupAttachment(Root); ForestGround->SetStaticMesh(CubeMesh);
    ForestGround->SetRelativeLocation(FVector(0, 0, -80));
    ForestGround->SetRelativeScale3D(FVector((ClearingHalfExtent * 2.f + 16000.f) / 100.f,
        (ClearingHalfExtent * 2.f + 16000.f) / 100.f, 1.f));
    ForestGround->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ForestGround->SetMaterial(0, GroundMaterial ? GroundMaterial.Get() : BaseMaterial.Get());
    ForestGround->RegisterComponent(); GeneratedComponents.Add(ForestGround);
    if (!GroundMaterial)
        if (auto* Mat = ForestGround->CreateDynamicMaterialInstance(0)) SetPrimitiveColor(Mat, FLinearColor(.20f, .15f, .10f));
    UE_LOG(LogTemp, Display,
        TEXT("ENVIRONMENT_KIT basin cedar=%d rock=%d boundary=%d ground=%d cedarB=%d rockB=%d post=%d rope=%d steppingStone=%d"),
        CedarMesh != nullptr, RockMesh != nullptr, BoundaryMesh != nullptr, GroundMaterial != nullptr, CedarBMesh != nullptr,
        RockBMesh != nullptr, RitualPostMesh != nullptr, RopeMesh != nullptr, SteppingStoneMesh != nullptr);
    const float H = ClearingHalfExtent;
    // Eight overlapping collision slabs form a gap-free octagon. Visual rocks sit outside
    // their inner faces, keeping the playable 80 m clearing open and the camera sweep useful.
    for (int32 I=0; I<8; ++I)
    {
        const float Yaw = I * 45.f;
        const FVector Out = FRotator(0,Yaw,0).Vector();
        AddBoundary(*FString::Printf(TEXT("BasinBoundary%02d"),I), Out * (H + 125.f) + FVector(0,0,RockWallHeight*.45f),
            FVector(2.5f,38.f,RockWallHeight/100.f), FRotator(0,Yaw,0));
        for (int32 J=-2; J<=2; ++J)
        {
            const FVector Along = FRotator(0,Yaw+90.f,0).Vector();
            const float HeightFactor = 0.82f + 0.09f * ((I * 3 + J + 7) % 4);
            const FLinearColor Color = ((I+J)&1) ? FLinearColor(.28f,.29f,.27f) : FLinearColor(.34f,.35f,.32f);
            AddRock(*FString::Printf(TEXT("WeatheredRock%02d_%02d"),I,J+2), Out*(H+1650.f)+Along*(J*760.f)
                + FVector(0,0,RockWallHeight*.48f), FVector(8.8f,5.4f,RockWallHeight*HeightFactor/100.f),
                FRotator((J%2)*6.f,Yaw+J*11.f,(I%3-1)*7.f),Color);
        }
    }
    // The visual wall is 8 m farther out than main; the imported-boss camera
    // can retreat 7 m without entering a NoCollision rock. Slabs stay unchanged.
    // Low moss-toned shoulders frame, rather than block, the western entrance view.
    AddRock(TEXT("EntranceShoulderNorth"),FVector(-H+300,1500,430),FVector(8,7,5),FRotator(4,12,-8),FLinearColor(.25f,.29f,.20f));
    AddRock(TEXT("EntranceShoulderSouth"),FVector(-H+300,-1500,430),FVector(8,7,5),FRotator(-5,-8,7),FLinearColor(.25f,.29f,.20f));

    // A restrained sacred procession line gives the broad clearing a readable focal axis.
    // Everything is shallow and non-blocking so combat and the camera retain the original space.
    const FLinearColor PathColor(.46f,.30f,.16f);
    for (int32 I=0; I<7; ++I)
    {
        const float X = -2800.f + I * 440.f;
        const FRotator Rotation(0.f, (I & 1) ? 4.f : -3.f, 0.f);
        if (SteppingStoneMesh)
        {
            // Sunk slightly so the low slab never becomes a step for the player or the boss.
            const FVector Size = SteppingStoneMesh->GetBounds().BoxExtent * 2.f;
            GeneratedComponents.Add(IshibashiriEnvironment::Visual(this, Root, *FString::Printf(TEXT("SacredPath%02d"), I),
                SteppingStoneMesh, FVector(X, 0.f, -6.f), FVector(290.f / Size.X, 115.f / Size.Y, 1.f),
                Rotation + FRotator(0.f, (I % 3) * 180.f, 0.f)));
            ++AccentCount;
        }
        else
            AddAccent(*FString::Printf(TEXT("SacredPath%02d"), I), FVector(X, 0.f, 3.f), FVector(2.9f, 1.15f, .06f), Rotation, PathColor);
    }
    // Vermilion stakes and indigo crosspieces frame the destination without copying a literal shrine gate.
    for (int32 Side=-1; Side<=1; Side+=2)
    {
        if (RitualPostMesh)
        {
            const FVector Size = RitualPostMesh->GetBounds().BoxExtent * 2.f;
            GeneratedComponents.Add(IshibashiriEnvironment::Visual(this, Root, *FString::Printf(TEXT("RitualStake%d"), Side),
                RitualPostMesh, FVector(-650.f, Side * 720.f, 0.f), FVector(18.f / Size.X, 18.f / Size.Y, 310.f / Size.Z),
                FRotator::ZeroRotator));
            ++AccentCount;
        }
        else
            AddAccent(*FString::Printf(TEXT("RitualStake%d"), Side), FVector(-650.f, Side*720.f, 155.f),
                FVector(.18f,.18f,3.1f), FRotator::ZeroRotator, FLinearColor(.58f,.08f,.035f));
        if (!RopeMesh)
            AddAccent(*FString::Printf(TEXT("RitualCord%d"), Side), FVector(-650.f, Side*720.f, 315.f),
                FVector(.55f,.12f,.10f), FRotator(0.f,0.f,Side*8.f), FLinearColor(.08f,.12f,.20f));
    }
    if (RopeMesh)
    {
        // A slack rope between the stakes marks the threshold; it hangs well above head height.
        GeneratedComponents.Add(IshibashiriEnvironment::Visual(this, Root, TEXT("RitualRope"), RopeMesh,
            FVector(-650.f, -712.f, 292.f), FVector(1424.f / (RopeMesh->GetBounds().BoxExtent.X * 2.f), .8f, .8f), FRotator(0, 90, 0)));
        ++AccentCount;
    }
    const FVector Trees[]={{-2500,-1900,0},{-2050,2050,0},{850,-2550,0},{1750,2300,0},{2850,-1450,0},{3100,1050,0}};
    for(int32 I=0;I<UE_ARRAY_COUNT(Trees);++I)
        AddTree(*FString::Printf(TEXT("OldCedar%02d"),I),Trees[I].GetSafeNormal2D() * (H + 850.f),720.f+(I%3)*110.f,34.f+(I%2)*7.f);
    for (int32 I = 0; I < 20; ++I)
    {
        const float Yaw = I * 18.f + 7.f;
        AddTree(*FString::Printf(TEXT("PerimeterCedar%02d"), I), FRotator(0, Yaw, 0).Vector() * (H + 1150.f),
            740.f + (I % 4) * 90.f, 40.f);
    }
    if (BoundaryMesh)
        GeneratedComponents.Add(IshibashiriEnvironment::Visual(this, Root, TEXT("BasinBoundaryStoneArt"), BoundaryMesh,
            FVector(-H + 100.f, 700.f, 0.f), FVector(1.f), FRotator(0, 80, 0)));
    AddGroundDressing();
    AddGroundHistory();
    AddWallDressing();
    AddRitualRemnants();
    CreateChargePresentation();

    UDirectionalLightComponent* Sun = NewObject<UDirectionalLightComponent>(this,TEXT("BasinSun"));
    Sun->SetupAttachment(Root); Sun->SetRelativeRotation(FRotator(-48,-32,0)); Sun->SetIntensity(3.2f);
    Sun->SetLightColor(FLinearColor(.72f, .79f, .85f)); Sun->SetAtmosphereSunLight(true); Sun->SetMobility(EComponentMobility::Movable); Sun->RegisterComponent(); GeneratedComponents.Add(Sun);
    KeyLight = Sun;
    UPointLightComponent* Fill = NewObject<UPointLightComponent>(this,TEXT("BasinFill"));
    Fill->SetupAttachment(Root); Fill->SetRelativeLocation(FVector(0,0,1500)); Fill->SetIntensity(90000.f); Fill->SetAttenuationRadius(6200.f);
    Fill->SetCastShadows(false); Fill->SetLightColor(FLinearColor(.70f,.76f,.82f)); Fill->SetMobility(EComponentMobility::Movable); Fill->RegisterComponent(); GeneratedComponents.Add(Fill);

    UExponentialHeightFogComponent* Fog = NewObject<UExponentialHeightFogComponent>(this,TEXT("BasinGroundHaze"));
    Fog->SetupAttachment(Root); Fog->SetRelativeLocation(FVector(0,0,-180.f)); Fog->SetFogDensity(.012f);
    Fog->SetFogHeightFalloff(.32f); Fog->SetFogInscatteringColor(FLinearColor(.34f,.39f,.42f));
    Fog->SetStartDistance(1200.f); Fog->SetFogMaxOpacity(.22f); Fog->SetVolumetricFog(true);
    Fog->SetVolumetricFogScatteringDistribution(.35f); Fog->SetVolumetricFogExtinctionScale(.55f);
    Fog->SetVolumetricFogDistance(6000.f); Fog->RegisterComponent(); GeneratedComponents.Add(Fog);
    GroundFog = Fog;

    // Physically based aerial perspective is useful in both profiles; expensive lighting is selected by the launcher.
    USkyAtmosphereComponent* Atmosphere = NewObject<USkyAtmosphereComponent>(this,TEXT("BasinSkyAtmosphere"));
    Atmosphere->SetupAttachment(Root);
    Atmosphere->SetMieScatteringScale(.012f); Atmosphere->SetMieAnisotropy(.65f);
    Atmosphere->RegisterComponent(); GeneratedComponents.Add(Atmosphere);
    auto* Sky = NewObject<USkyLightComponent>(this, TEXT("BasinAmbient"));
    Sky->SetupAttachment(Root); Sky->SetMobility(EComponentMobility::Movable); Sky->SetIntensity(.65f);
    Sky->SetLightColor(FLinearColor(.70f,.78f,.85f)); Sky->RegisterComponent(); GeneratedComponents.Add(Sky);
}
