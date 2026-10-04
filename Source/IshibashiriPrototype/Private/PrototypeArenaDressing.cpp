#include "PrototypeArenaDressing.h"
#include "IshibashiriEnvironment.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

APrototypeArenaDressing::APrototypeArenaDressing()
{
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    CubeMesh = Cube.Object;
}

void APrototypeArenaDressing::Dress(float H, UPrimitiveComponent* Floor, const TArray<UPrimitiveComponent*>& Walls)
{
    using IshibashiriEnvironment::Load;
    UStaticMesh* Cedar = Load<UStaticMesh>(TEXT("SM_Ishibashiri_OldCedar_A"));
    UStaticMesh* CedarB = Load<UStaticMesh>(TEXT("SM_Ishibashiri_OldCedar_B"));
    UStaticMesh* Rock = Load<UStaticMesh>(TEXT("SM_Ishibashiri_Rock_A"));
    UStaticMesh* RockB = Load<UStaticMesh>(TEXT("SM_Ishibashiri_Rock_B"));
    UStaticMesh* Fern = Load<UStaticMesh>(TEXT("SM_Ishibashiri_Fern_A"));
    UStaticMesh* FallenCedar = Load<UStaticMesh>(TEXT("SM_Ishibashiri_FallenCedar_A"));
    UMaterialInterface* Ground = Load<UMaterialInterface>(TEXT("M_Ishibashiri_Ground"));
    UE_LOG(LogTemp, Display, TEXT("ENVIRONMENT_KIT arena cedar=%d rock=%d ground=%d fern=%d cedarB=%d rockB=%d fallenCedar=%d"),
        Cedar != nullptr, Rock != nullptr, Ground != nullptr, Fern != nullptr, CedarB != nullptr, RockB != nullptr,
        FallenCedar != nullptr);
    // -PrimitiveEnvironment and missing art keep the original blockout for matched-camera comparisons.
    if (!Cedar && !Rock && !Ground) return;
    bDressed = true;

    if (Ground && Floor) Floor->SetMaterial(0, Ground);
    if (Ground && CubeMesh)
    {
        // Sits below the playable floor top and outside it, so it never becomes a walkable or camera surface.
        auto* ForestGround = NewObject<UStaticMeshComponent>(this, TEXT("ArenaForestGround"));
        ForestGround->SetupAttachment(Root);
        ForestGround->SetStaticMesh(CubeMesh);
        ForestGround->SetRelativeLocation(FVector(0.f, 0.f, -80.f));
        ForestGround->SetRelativeScale3D(FVector((H * 2.f + 16000.f) / 100.f, (H * 2.f + 16000.f) / 100.f, 1.f));
        ForestGround->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ForestGround->SetMaterial(0, Ground);
        ForestGround->RegisterComponent();
    }
    if (Rock)
    {
        // The walls stay as invisible blockers; only their grey slabs are replaced by the rock rim.
        for (UPrimitiveComponent* Wall : Walls)
            if (Wall) Wall->SetVisibility(false);
        AddRockRim(H, Rock, RockB);
    }
    AddForest(H, Cedar, CedarB, Fern, FallenCedar);
    AddAtmosphere();
}

void APrototypeArenaDressing::AddRockRim(float H, UStaticMesh* Rock, UStaticMesh* RockB)
{
    using IshibashiriEnvironment::Hash01;
    // Centres sit outside the wall's inner face so the 80 m square, dodge space and camera sweep stay unchanged.
    const float Span = H * 2.f + 400.f;
    const int32 Count = FMath::CeilToInt(Span / 700.f) + 1;
    const float Step = Span / (Count - 1);
    for (int32 Side = 0; Side < 4; ++Side)
    {
        const float Yaw = Side * 90.f;
        const FVector Out = FRotator(0.f, Yaw, 0.f).Vector();
        const FVector Along = FRotator(0.f, Yaw + 90.f, 0.f).Vector();
        // A taller back row, offset by half a step, breaks the front row's even skyline.
        for (int32 Row = 0; Row < 2; ++Row)
            for (int32 I = 0; I < Count - Row; ++I)
            {
                const FString Name = FString::Printf(TEXT("ArenaRimRock%d_%d_%02d"), Side, Row, I);
                UStaticMesh* Art = IshibashiriEnvironment::Variant(*Name, Rock, RockB, 2);
                const float Height = Row ? 1000.f + Hash01(Side + 12, I) * 700.f : 500.f + Hash01(Side, I) * 450.f;
                const FVector Size = Row ? FVector(900.f, 1200.f, Height) : FVector(700.f, 950.f, Height);
                const float Inset = (Row ? H + 1250.f : H + 550.f) + Hash01(I, Side + 4 * (Row + 1)) * 180.f;
                const FVector Base = Out * Inset + Along * (-Span * .5f + (I + Row * .5f) * Step) + FVector(0.f, 0.f, -60.f);
                IshibashiriEnvironment::Visual(this, Root, *Name, Art, Base, IshibashiriEnvironment::Fit(Art, Size),
                    FRotator(0.f, Yaw + Hash01(Side + 8, I + Row * 31) * 360.f, 0.f));
                ++RockCount;
            }
    }
}

void APrototypeArenaDressing::AddForest(float H, UStaticMesh* Cedar, UStaticMesh* CedarB, UStaticMesh* Fern, UStaticMesh* FallenCedar)
{
    using IshibashiriEnvironment::Hash01;
    if (Cedar)
    {
        for (int32 I = 0; I < 24; ++I)
        {
            const FString Name = FString::Printf(TEXT("ArenaCedar%02d"), I);
            UStaticMesh* Art = IshibashiriEnvironment::Variant(*Name, Cedar, CedarB, 3);
            const float Scale = (1900.f + Hash01(I, 20) * 700.f) / FMath::Max(1.f, Art->GetBounds().BoxExtent.Z * 2.f);
            const FVector Out = FRotator(0.f, I * 15.f + Hash01(I, 21) * 8.f, 0.f).Vector();
            // A circle around the square keeps the corners from reading as a fence line.
            const FVector Base = Out * (H * 1.42f + 900.f + Hash01(I, 22) * 500.f) + FVector(0.f, 0.f, -30.f);
            IshibashiriEnvironment::Visual(this, Root, *Name, Art, Base, FVector(Scale), FRotator(0.f, Hash01(I, 23) * 360.f, 0.f));
            ++TreeCount;
        }
        TArray<FTransform> Distant, DistantB;
        for (int32 I = 0; I < 72; ++I)
        {
            UStaticMesh* Mesh = CedarB && I % 3 == 1 ? CedarB : Cedar;
            const FVector Out = FRotator(0.f, (I + Hash01(I, 24)) * 5.f, 0.f).Vector();
            const float Height = 1900.f + Hash01(I, 25) * 900.f;
            (Mesh == Cedar ? Distant : DistantB).Add(FTransform(FRotator(0.f, Hash01(I, 26) * 360.f, 0.f),
                Out * (H * 1.42f + 2200.f + Hash01(I, 27) * 3600.f) + FVector(0.f, 0.f, -30.f),
                FVector(Height / FMath::Max(1.f, Mesh->GetBounds().BoxExtent.Z * 2.f))));
        }
        IshibashiriEnvironment::Scatter(this, Root, TEXT("ArenaDistantCedars"), Cedar, Distant, 30000.f);
        if (CedarB) IshibashiriEnvironment::Scatter(this, Root, TEXT("ArenaDistantCedarsB"), CedarB, DistantB, 30000.f);
    }
    if (Fern)
    {
        TArray<FTransform> Ferns;
        for (int32 I = 0; I < 160; ++I)
        {
            const FVector Out = FRotator(0.f, (I + Hash01(I, 30)) * 360.f / 160.f, 0.f).Vector();
            // Radial distance is measured to the square's edge along this direction, so ferns stay outside the walls.
            const float Edge = H / FMath::Max(FMath::Abs(Out.X), FMath::Abs(Out.Y));
            const FVector Base = Out * (Edge + 150.f + Hash01(I, 32) * 900.f) + FVector(0.f, 0.f, -28.f);
            Ferns.Add(FTransform(FRotator(0.f, Hash01(I, 31) * 360.f, 0.f), Base, FVector(1.3f + Hash01(I, 33) * 1.2f)));
        }
        IshibashiriEnvironment::Scatter(this, Root, TEXT("ArenaFerns"), Fern, Ferns, 9000.f);
    }
    if (FallenCedar)
    {
        const float Yaws[] = {38.f, 221.f};
        for (int32 I = 0; I < UE_ARRAY_COUNT(Yaws); ++I)
            IshibashiriEnvironment::Visual(this, Root, *FString::Printf(TEXT("ArenaFallenCedar%02d"), I), FallenCedar,
                FRotator(0.f, Yaws[I], 0.f).Vector() * (H * 1.42f + 400.f) + FVector(0.f, 0.f, -20.f), FVector(.9f + I * .1f),
                FRotator(0.f, Yaws[I] + 90.f, 0.f));
    }
}

void APrototypeArenaDressing::AddAtmosphere()
{
    auto* Atmosphere = NewObject<USkyAtmosphereComponent>(this, TEXT("ArenaSkyAtmosphere"));
    Atmosphere->SetupAttachment(Root);
    Atmosphere->RegisterComponent();
    auto* Sky = NewObject<USkyLightComponent>(this, TEXT("ArenaAmbient"));
    Sky->SetupAttachment(Root);
    Sky->SetMobility(EComponentMobility::Movable);
    Sky->SetIntensity(.5f);
    Sky->SetLightColor(FLinearColor(.70f, .78f, .85f));
    Sky->RegisterComponent();
    // Starts beyond the boss so the encounter itself stays crisp; only the forest edge recedes.
    auto* Fog = NewObject<UExponentialHeightFogComponent>(this, TEXT("ArenaGroundHaze"));
    Fog->SetupAttachment(Root);
    Fog->SetRelativeLocation(FVector(0.f, 0.f, -180.f));
    Fog->SetFogDensity(.012f);
    Fog->SetFogHeightFalloff(.32f);
    Fog->SetFogInscatteringColor(FLinearColor(.34f, .39f, .42f));
    Fog->SetStartDistance(2500.f);
    Fog->SetFogMaxOpacity(.22f);
    Fog->RegisterComponent();
}
