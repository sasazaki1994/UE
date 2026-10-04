#include "IshibashiriApproachArena.h"
#include "PrimitiveAppearance.h"
#include "IshibashiriEnvironment.h"
#include "Animation/AnimSequence.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectGlobals.h"

AIshibashiriApproachArena::AIshibashiriApproachArena()
{
    PrimaryActorTick.bCanEverTick = true;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    CubeMesh = Cube.Object;
    SphereMesh = Sphere.Object;
    BaseMaterial = Material.Object;
    // 1.31 km of bends; at the 6 m/s walking speed this is about three minutes without stopping.
    PathPoints = {{0, 0, 0}, {11000, 0, 0}, {20500, 6500, 100}, {30500, 8500, 180}, {39500, 1000, 260}, {49000, -6500, 300},
        {58500, -7200, 380}, {67500, -500, 450}, {76000, 7200, 500}, {85000, 7600, 420}, {94500, 500, 300}, {103500, -6000, 180},
        {112000, 0, 0}};
    for (int32 I = 1; I < PathPoints.Num(); ++I) PathLengthMeters += FVector::Distance(PathPoints[I - 1], PathPoints[I]) / 100.f;
}

void AIshibashiriApproachArena::ClearGenerated()
{
    for (UActorComponent* C : Generated)
        if (IsValid(C)) C->DestroyComponent();
    Generated.Reset();
    RevealParts.Reset();
}

UStaticMeshComponent* AIshibashiriApproachArena::AddPrimitive(const TCHAR* Name, UStaticMesh* MeshAsset, const FVector& Location,
    const FVector& Scale, const FRotator& Rotation, const FLinearColor& Color, bool bCollision)
{
    auto* C = NewObject<UStaticMeshComponent>(this, Name);
    C->SetupAttachment(Root);
    C->SetStaticMesh(MeshAsset);
    C->SetRelativeLocation(Location);
    C->SetRelativeScale3D(Scale);
    C->SetRelativeRotation(Rotation);
    C->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    if (bCollision) C->SetCollisionProfileName(TEXT("BlockAll"));
    const bool bBlockoutMesh = MeshAsset == CubeMesh || MeshAsset == SphereMesh;
    if (bBlockoutMesh) C->SetMaterial(0, BaseMaterial);
    C->RegisterComponent();
    if (bBlockoutMesh)
        if (auto* M = C->CreateDynamicMaterialInstance(0)) SetPrimitiveColor(M, Color);
    Generated.Add(C);
    if (MeshAsset == SphereMesh && RockMesh && (FString(Name).StartsWith(TEXT("Cliff")) || FString(Name) == TEXT("GougedRock")))
    {
        C->SetVisibility(false);
        const FVector Size = Scale * 100.f;
        UStaticMesh* Art = IshibashiriEnvironment::Variant(Name, RockMesh, RockBMesh, 2);
        Generated.Add(IshibashiriEnvironment::Visual(this, Root, *FString::Printf(TEXT("%sArt"), Name), Art,
            Location - Rotation.RotateVector(FVector(0, 0, Size.Z * .5f)), IshibashiriEnvironment::Fit(Art, Size), Rotation));
    }
    else if ((FString(Name).StartsWith(TEXT("WetEarth")) || FString(Name).StartsWith(TEXT("ForestGround"))) && GroundMaterial)
        C->SetMaterial(0, GroundMaterial);
    return C;
}

void AIshibashiriApproachArena::AddTree(const TCHAR* Name, const FVector& L, float H, bool bFallen)
{
    const FRotator R = bFallen ? FRotator(0, 25, 88) : FRotator(0, 0, 0);
    auto* Trunk = AddPrimitive(*FString::Printf(TEXT("%sTrunk"), Name), CubeMesh, L + FVector(0, 0, bFallen ? 70 : H * .5f),
        FVector(.55f, .55f, H / 100.f), R, FLinearColor(.075f, .052f, .035f), true);
    if (FallenCedarMesh && bFallen)
    {
        Trunk->SetVisibility(false);
        // The log lies along local X; the hidden cube keeps the original blocking volume.
        const float S = H / FMath::Max(1.f, FallenCedarMesh->GetBounds().BoxExtent.X * 2.f);
        Generated.Add(IshibashiriEnvironment::Visual(this, Root, *FString::Printf(TEXT("%sArt"), Name), FallenCedarMesh,
            L + FVector(0, 0, -15), FVector(S, S * 1.15f, S * 1.15f), FRotator(0, R.Yaw + 90.f, 0)));
        return;
    }
    if (CedarMesh && !bFallen)
    {
        Trunk->SetVisibility(false);
        UStaticMesh* Art = IshibashiriEnvironment::Variant(Name, CedarMesh, CedarBMesh, 3);
        const float ArtHeight = 1800.f + H * .32f;
        const float S = ArtHeight / FMath::Max(1.f, Art->GetBounds().BoxExtent.Z * 2.f);
        Generated.Add(IshibashiriEnvironment::Visual(this, Root, *FString::Printf(TEXT("%sArt"), Name), Art,
            L, FVector(S), FRotator(0, FCrc::StrCrc32(Name) % 360, 0)));
        return;
    }
    if (!bFallen)
        AddPrimitive(*FString::Printf(TEXT("%sCrown"), Name), SphereMesh, L + FVector(0, 0, H * .82f), FVector(3.8f, 3.4f, 4.8f),
            FRotator::ZeroRotator, FLinearColor(.07f, .11f, .055f));
}

void AIshibashiriApproachArena::AddGroundDressing()
{
    using IshibashiriEnvironment::Hash01;
    TArray<FTransform> Ferns, Stones, Distant, DistantB;
    const float CedarHeight = CedarMesh ? FMath::Max(1.f, CedarMesh->GetBounds().BoxExtent.Z * 2.f) : 1.f;
    const float CedarBHeight = CedarBMesh ? FMath::Max(1.f, CedarBMesh->GetBounds().BoxExtent.Z * 2.f) : 1.f;
    for (int32 I = 0; I < PathPoints.Num() - 1; ++I)
    {
        const FVector A = PathPoints[I], B = PathPoints[I + 1];
        const FVector Side = FVector::CrossProduct((B - A).GetSafeNormal(), FVector::UpVector);
        // A deeper forest beyond the landmark trees hides the edge of the ground slabs.
        for (int32 S : {-1, 1})
            for (int32 K = 0; K < 6; ++K)
            {
                const int32 Seed = 50000 + (I * 2 + (S > 0)) * 100 + K;
                const FVector Base = FMath::Lerp(A, B, (K + Hash01(Seed, 1)) / 6.f);
                const bool bVariant = CedarBMesh && K % 3 == 1;
                (bVariant ? DistantB : Distant).Add(FTransform(FRotator(0, Hash01(Seed, 2) * 360.f, 0),
                    Base + Side * (S * (3000.f + Hash01(Seed, 3) * 6500.f)) + FVector(0, 0, -40),
                    FVector((1900.f + Hash01(Seed, 4) * 900.f) / (bVariant ? CedarBHeight : CedarHeight))));
            }
        for (int32 S : {-1, 1})
            for (int32 K = 0; K < 14; ++K)
            {
                const int32 Seed = (I * 2 + (S > 0)) * 100 + K;
                const FVector Base = FMath::Lerp(A, B, (K + Hash01(Seed, 1)) / 14.f);
                // The 18 m road keeps its full width; dressing starts just outside its edge.
                const float Lateral = 960.f + Hash01(Seed, 2) * 900.f;
                Ferns.Add(FTransform(FRotator(0, Hash01(Seed, 3) * 360.f, 0), Base + Side * (S * Lateral) + FVector(0, 0, -32),
                    FVector(1.2f + Hash01(Seed, 4) * 1.1f)));
                if (K % 3 == 0)
                    Stones.Add(FTransform(FRotator(0, Hash01(Seed, 5) * 360.f, Hash01(Seed, 6) * 20.f - 10.f),
                        Base + Side * (S * (930.f + Hash01(Seed, 7) * 260.f)) + FVector(0, 0, -45),
                        FVector(.12f + Hash01(Seed, 8) * .22f)));
            }
    }
    if (CedarMesh) Generated.Add(IshibashiriEnvironment::Scatter(this, Root, TEXT("DistantCedars"), CedarMesh, Distant, 30000.f));
    if (CedarBMesh)
        Generated.Add(IshibashiriEnvironment::Scatter(this, Root, TEXT("DistantCedarsB"), CedarBMesh, DistantB, 30000.f));
    if (FernMesh) Generated.Add(IshibashiriEnvironment::Scatter(this, Root, TEXT("RoadsideFerns"), FernMesh, Ferns, 9000.f));
    if (RockMesh) Generated.Add(IshibashiriEnvironment::Scatter(this, Root, TEXT("RoadsideStones"), RockMesh, Stones, 12000.f));
}

void AIshibashiriApproachArena::OnConstruction(const FTransform& T)
{
    Super::OnConstruction(T);
    ClearGenerated();
    CedarMesh = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_OldCedar_A"));
    RockMesh = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_Rock_A"));
    BoundaryMesh = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_BoundaryStone_A"));
    FallenCedarMesh = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_FallenCedar_A"));
    FernMesh = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_Fern_A"));
    CedarBMesh = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_OldCedar_B"));
    RockBMesh = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_Rock_B"));
    RitualPostMesh = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_RitualPost_A"));
    RopeMesh = IshibashiriEnvironment::Load<UStaticMesh>(TEXT("SM_Ishibashiri_OldRope_A"));
    GroundMaterial = IshibashiriEnvironment::Load<UMaterialInterface>(TEXT("M_Ishibashiri_Ground"));
    FootprintDecal = IshibashiriEnvironment::Load<UMaterialInterface>(TEXT("D_Ishibashiri_Footprint_A"));
    CrackDecal = IshibashiriEnvironment::Load<UMaterialInterface>(TEXT("D_Ishibashiri_CorruptionCrack_A"));
    UE_LOG(LogTemp, Display,
        TEXT("ENVIRONMENT_KIT approach cedar=%d rock=%d boundary=%d ground=%d fallen=%d fern=%d cedarB=%d rockB=%d post=%d rope=%d "
             "footprint=%d crack=%d"),
        CedarMesh != nullptr, RockMesh != nullptr, BoundaryMesh != nullptr, GroundMaterial != nullptr,
        FallenCedarMesh != nullptr, FernMesh != nullptr, CedarBMesh != nullptr, RockBMesh != nullptr, RitualPostMesh != nullptr,
        RopeMesh != nullptr, FootprintDecal != nullptr, CrackDecal != nullptr);
    const FLinearColor Soil(.20f, .15f, .10f), Rock(.12f, .13f, .13f), Moss(.12f, .18f, .09f), Bone(.58f, .52f, .40f),
        Vermilion(.35f, .055f, .025f), Corruption(.008f, .006f, .009f);
    for (int32 I = 0; I < PathPoints.Num() - 1; ++I)
    {
        const FVector A = PathPoints[I], B = PathPoints[I + 1], D = B - A;
        // Follow the elevation between waypoints so the road is walkable at each join.
        // Extend each end by 50 cm to keep the collision surface continuous at bends.
        constexpr float RoadEndOverlap = 50.f;
        AddPrimitive(*FString::Printf(TEXT("WetEarth%02d"), I), CubeMesh, (A + B) * .5f + FVector(0, 0, -42),
            FVector((D.Size() + 2.f * RoadEndOverlap) / 100.f, 18.f, 1.f), D.Rotation(), Soil, true);
        AddPrimitive(*FString::Printf(TEXT("ForestGround%02d"), I), CubeMesh, (A + B) * .5f + FVector(0, 0, -80),
            FVector((D.Size() + 200.f) / 100.f, 240.f, 1.f), D.Rotation(), Soil, false);
        const FVector Side = FVector::CrossProduct(D.GetSafeNormal(), FVector::UpVector);
        for (int32 S : {-1, 1})
        {
            const FVector P = (A + B) * .5f + Side * (S * (1250 + (I % 3) * 260));
            AddPrimitive(*FString::Printf(TEXT("Cliff%02d_%d"), I, S), SphereMesh, P + FVector(0, 0, 420),
                FVector(9 + (I % 2) * 3, 6, 7 + (I % 3)), FRotator(I * 3, I * 19, S * 7), I % 3 ? Rock : Moss, true);
            AddTree(*FString::Printf(TEXT("OldTree%02d_%d"), I, S), P + Side * (S * 650), 760 + (I % 4) * 90);
            // Close gaps between the old landmarks while leaving the full 18 m walking corridor open.
            for (int32 TIndex = 1; TIndex <= 4; ++TIndex)
            {
                const FVector TreeBase = FMath::Lerp(A, B, TIndex / 5.f) + Side * (S * (2000.f + (TIndex % 2) * 380.f));
                AddTree(*FString::Printf(TEXT("Forest%02d_%d_%d"), I, S, TIndex), TreeBase, 680.f + TIndex * 95.f);
            }
        }
    }
    // Broken warning stone, weathered rope/pillars, restrained black cracks, and the damage trail.
    auto* Boundary = AddPrimitive(TEXT("BoundaryStone"), CubeMesh, PathPoints[3] + FVector(-300, 700, 150), FVector(1.0f, .45f, 3.1f), FRotator(0, -18, 13),
        Bone, true);
    if (BoundaryMesh)
    {
        Boundary->SetVisibility(false);
        Generated.Add(IshibashiriEnvironment::Visual(this, Root, TEXT("BoundaryStoneArt"), BoundaryMesh,
            PathPoints[3] + FVector(-300, 700, 0), FVector(1.f), FRotator(0, -18, 0)));
    }
    for (int32 S : {-1, 1})
    {
        auto* Post = AddPrimitive(*FString::Printf(TEXT("RitualPost%d"), S), CubeMesh, PathPoints[3] + FVector(700, S * 900, 240),
            FVector(.22f, .22f, 4.8f), FRotator(0, 0, S * 5), Vermilion, true);
        if (!RitualPostMesh) continue;
        Post->SetVisibility(false);
        const FVector Size = RitualPostMesh->GetBounds().BoxExtent * 2.f;
        Generated.Add(IshibashiriEnvironment::Visual(this, Root, *FString::Printf(TEXT("RitualPost%dArt"), S), RitualPostMesh,
            PathPoints[3] + FVector(700, S * 900, 0), FVector(22.f / Size.X, 22.f / Size.Y, 480.f / Size.Z), FRotator(0, 0, S * 5)));
    }
    auto* Shimenawa = AddPrimitive(
        TEXT("DecayedShimenawa"), CubeMesh, PathPoints[3] + FVector(700, 0, 430), FVector(.10f, 9.f, .10f), FRotator(0, 0, 7), Bone);
    if (RopeMesh)
    {
        // The rope pivots at its first end and runs along local X, so it spans post to post.
        Shimenawa->SetVisibility(false);
        Generated.Add(IshibashiriEnvironment::Visual(this, Root, TEXT("DecayedShimenawaArt"), RopeMesh,
            PathPoints[3] + FVector(700, -880, 440), FVector(1760.f / (RopeMesh->GetBounds().BoxExtent.X * 2.f), 1.f, 1.f),
            FRotator(0, 90, 0)));
    }
    for (int32 I = 0; I < 4; ++I)
    {
        const FVector Print = PathPoints[6] + FVector(I * 950, (I & 1) ? 430 : -430, 0);
        if (FootprintDecal)
            Generated.Add(IshibashiriEnvironment::Decal(this, Root, *FString::Printf(TEXT("GiantFootprint%02d"), I), FootprintDecal,
                Print, FVector(220, 170, 290), 20));
        else
            AddPrimitive(*FString::Printf(TEXT("GiantFootprint%02d"), I), SphereMesh, Print + FVector(0, 0, -28), FVector(5.2f, 2.5f, .22f),
                FRotator(0, 20, 0), Corruption);
    }
    AddPrimitive(
        TEXT("GougedRock"), SphereMesh, PathPoints[6] + FVector(3700, 1500, 580), FVector(7, 5, 8), FRotator(0, 0, -24), Rock, true);
    AddTree(TEXT("ShatteredCedar"), PathPoints[6] + FVector(2500, -1300, 0), 1050, true);
    for (int32 I = 0; I < 5; ++I)
    {
        const FVector Crack = PathPoints[8] + FVector(I * 350 - 700, I % 2 * 240 - 120, 4);
        if (CrackDecal)
            Generated.Add(IshibashiriEnvironment::Decal(this, Root, *FString::Printf(TEXT("CorruptionCrack%02d"), I), CrackDecal, Crack,
                FVector(200, 170, 170), 20 + I * 67));
        else
            AddPrimitive(*FString::Printf(TEXT("CorruptionCrack%02d"), I), CubeMesh, Crack, FVector(2.5f, .055f, .04f),
                FRotator(0, 20 + I * 14, 0), Corruption);
    }
    AddGroundDressing();
    // A deliberately inert full-body silhouette proxy: no AI, damage, Grab, Kakon, or collision.
    USkeletalMesh* Creature = FParse::Param(FCommandLine::Get(), TEXT("PrimitiveEnvironment"))
        ? nullptr
        : LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Rigged/Ishibashiri/SK_Ishibashiri.SK_Ishibashiri"), nullptr, LOAD_NoWarn);
    if (Creature)
    {
        auto* Silhouette = NewObject<USkeletalMeshComponent>(this, TEXT("DistantIshibashiri"));
        Silhouette->SetupAttachment(Root);
        Silhouette->SetSkeletalMesh(Creature);
        Silhouette->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        const FBoxSphereBounds Bounds = Creature->GetBounds();
        const float S = 1500.f / FMath::Max(1.f, 2.f * FMath::Max(Bounds.BoxExtent.X, Bounds.BoxExtent.Y));
        // Feet on the forest floor beside the road; the rig faces local -Y, so yaw 180 walks it along +Y.
        const float Floor = PathPoints[8].Z - 30.f;
        Silhouette->SetRelativeLocationAndRotation(
            FVector(RevealOrigin.X, RevealOrigin.Y, Floor - (Bounds.Origin.Z - Bounds.BoxExtent.Z) * S), FRotator(0, 180, 0));
        Silhouette->SetRelativeScale3D(FVector(S));
        Silhouette->RegisterComponent();
        if (UAnimSequence* Walk =
                LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Rigged/Ishibashiri/AN_Ishibashiri_Walk"), nullptr, LOAD_NoWarn))
            Silhouette->PlayAnimation(Walk, true);
        Generated.Add(Silhouette);
        RevealParts.Add(Silhouette);
    }
    else
    {
        RevealParts.Add(
            AddPrimitive(TEXT("DistantIshibashiriBody"), SphereMesh, RevealOrigin, FVector(15, 6, 8), FRotator(0, 90, 0), Rock));
        RevealParts.Add(AddPrimitive(
            TEXT("DistantIshibashiriBack"), SphereMesh, RevealOrigin + FVector(0, 0, 700), FVector(11, 5, 5), FRotator(0, 90, 0), Moss));
    }
    for (UPrimitiveComponent* C : RevealParts) C->SetVisibility(false);
    // This chapter uses a blank map, so it needs its own movable daylight.
    auto* Sun = NewObject<UDirectionalLightComponent>(this, TEXT("ApproachSun"));
    Sun->SetupAttachment(Root);
    Sun->SetMobility(EComponentMobility::Movable);
    Sun->SetRelativeRotation(FRotator(-48, -32, 0));
    Sun->SetIntensity(3.2f);
    Sun->SetAtmosphereSunLight(true);
    // The shadowless fill must never be picked for forward shading or volumetric fog.
    Sun->SetForwardShadingPriority(1);
    Sun->RegisterComponent();
    Generated.Add(Sun);
    auto* Fill = NewObject<UDirectionalLightComponent>(this, TEXT("ApproachFill"));
    Fill->SetupAttachment(Root);
    Fill->SetMobility(EComponentMobility::Movable);
    Fill->SetRelativeRotation(FRotator(-25, 145, 0));
    Fill->SetIntensity(1.1f);
    Fill->SetLightColor(FLinearColor(.66f, .75f, 1.f));
    Fill->SetCastShadows(false);
    Fill->RegisterComponent();
    Generated.Add(Fill);
    auto* Fog = NewObject<UExponentialHeightFogComponent>(this, TEXT("ApproachMist"));
    Fog->SetupAttachment(Root);
    Fog->SetRelativeLocation(FVector(55000, 0, -100));
    Fog->SetFogDensity(.009f);
    Fog->SetFogInscatteringColor(FLinearColor(.34f, .39f, .42f));
    Fog->SetFogMaxOpacity(.16f);
    Fog->SetStartDistance(1500);
    Fog->RegisterComponent();
    Generated.Add(Fog);
    auto* Atmosphere = NewObject<USkyAtmosphereComponent>(this, TEXT("ApproachSky"));
    Atmosphere->SetupAttachment(Root); Atmosphere->RegisterComponent(); Generated.Add(Atmosphere);
    auto* Sky = NewObject<USkyLightComponent>(this, TEXT("ApproachAmbient"));
    Sky->SetupAttachment(Root); Sky->SetMobility(EComponentMobility::Movable);
    Sky->SetIntensity(.65f); Sky->SetLightColor(FLinearColor(.68f, .78f, .85f));
    Sky->RegisterComponent(); Generated.Add(Sky);
}

void AIshibashiriApproachArena::SetStage(int32 NewStage)
{
    if (NewStage <= Stage) return;
    Stage = NewStage;
    if (Stage == 1)
    {
        Subtitle = TEXT("ここから先へ入るな");
        SubtitleRemaining = 4.f;
        UE_LOG(LogTemp, Display, TEXT("APPROACH_STAGE1 distant_rumble boundary_stone NOT_IMPLEMENTED — AUDIO ASSET REQUIRED"));
    }
    else if (Stage == 2) { UE_LOG(LogTemp, Display, TEXT("APPROACH_STAGE2 giant_footprints damage_trail")); }
    else if (Stage == 3)
    {
        bRevealVisible = true;
        RevealElapsed = 0;
        RevealCount++;
        for (UPrimitiveComponent* C : RevealParts) C->SetVisibility(true);
        UE_LOG(LogTemp, Display, TEXT("APPROACH_REVEAL count=%d duration=3.0 combat=false NOT_IMPLEMENTED — AUDIO ASSET REQUIRED"),
            RevealCount);
    }
    else if (Stage == 4)
    {
        UE_LOG(LogTemp, Display, TEXT("APPROACH_STAGE4 close_rumble camera_reaction_hook NOT_IMPLEMENTED — AUDIO ASSET REQUIRED"));
    }
}

void AIshibashiriApproachArena::ObservePlayer(const FVector& P)
{
    if (Stage < 1 && FVector::Dist2D(P, PathPoints[3]) < 1800) SetStage(1);
    if (Stage < 2 && FVector::Dist2D(P, PathPoints[6]) < 2200) SetStage(2);
    if (Stage < 3 && FVector::Dist2D(P, PathPoints[8]) < 2200) SetStage(3);
    if (Stage < 4 && FVector::Dist2D(P, PathPoints[10]) < 2200) SetStage(4);
    if (Stage >= 4 && FVector::Dist2D(P, PathPoints.Last()) < 900)
    {
        bGateReached = true;
        UE_LOG(LogTemp, Display, TEXT("APPROACH_BASIN_GATE reached"));
    }
}

void AIshibashiriApproachArena::Tick(float Dt)
{
    Super::Tick(Dt);
    if (SubtitleRemaining > 0 && ((SubtitleRemaining -= Dt) <= 0)) Subtitle.Empty();
    if (!bRevealVisible) return;
    RevealElapsed += Dt;
    for (UPrimitiveComponent* C : RevealParts) C->AddWorldOffset(FVector(0, Dt * 650, 0));
    if (RevealElapsed >= 3.f)
    {
        bRevealVisible = false;
        for (UPrimitiveComponent* C : RevealParts) C->SetVisibility(false);
        UE_LOG(LogTemp, Display, TEXT("APPROACH_REVEAL_END combat=false"));
    }
}
