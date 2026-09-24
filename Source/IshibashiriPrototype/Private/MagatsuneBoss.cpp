#include "MagatsuneBoss.h"
#include "MagatsunePlayer.h"
#include "PlayerSenseComponent.h"
#include "DebugGuidance.h"
#include "KakonActor.h"
#include "NushiProgressComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "PrimitiveAppearance.h"

AMagatsuneBoss::AMagatsuneBoss()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("MagatsuneRoot")));
    MovingRoot = CreateDefaultSubobject<USceneComponent>(TEXT("LivingTerrainRoot"));
    MovingRoot->SetupAttachment(RootComponent);
    RootCueLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("RootPulseCue"));
    RootCueLight->SetupAttachment(MovingRoot);
    RootCueLight->SetRelativeLocation(FVector(500,0,720));
    RootCueLight->SetAttenuationRadius(950.f);
    RootCueLight->SetCastShadows(false);
    RootCueLight->SetLightColor(FLinearColor(.45f,.08f,.10f));
    RootCueLight->SetIntensity(0.f);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    BuildPrimitive(Sphere.Object, Material.Object, TEXT("CentralCore"), MovingRoot, {500, 0, 720}, {5.5, 5.5, 7});
    for (int32 I = 0; I < 9; ++I)
    {
        const float A = I * .72f;
        const FVector At(I * 170.f - 350, FMath::Sin(A) * 330.f, 110 + I * 65.f);
        const FName Name(*FString::Printf(TEXT("CorruptedRoot%d"), I));
        BuildPrimitive(Cube.Object, Material.Object, *Name.ToString(), MovingRoot, At, {2.4, 1.25, 3.1});
        RootVisuals.Add(Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(Name)));
        RootVisualBaseLocations.Add(At);
    }
    for (int32 I = 0; I < RouteNodeCount; ++I)
    {
        BuildPrimitive(Sphere.Object, Material.Object, *FString::Printf(TEXT("Route%d"), I), MovingRoot, GetRouteLocal(I), {.32, .32, .32});
        RouteMarkers.Add(Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(*FString::Printf(TEXT("Route%d"), I))));
    }
}

void AMagatsuneBoss::BuildPrimitive(
    UStaticMesh* Mesh, UMaterialInterface* Material, const TCHAR* Name, USceneComponent* Parent, FVector At, FVector Scale)
{
    auto* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
    Part->SetupAttachment(Parent);
    Part->SetStaticMesh(Mesh);
    Part->SetMaterial(0, Material);
    Part->SetRelativeLocation(At);
    Part->SetRelativeScale3D(Scale);
    Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Part->SetCanEverAffectNavigation(false);
}

FVector AMagatsuneBoss::GetRouteLocal(int32 N) const
{
    static const FVector P[] = {{-720, -100, 130}, {-480, -250, 250}, {-220, -330, 390}, {40, -280, 520}, {210, -20, 570}, {330, 260, 700},
        {510, 400, 850}, {650, 230, 1010}, {680, -40, 1180}, {570, -260, 1370}, {480, -120, 1550}, {500, 0, 1810}};
    return P[FMath::Clamp(N, 0, RouteNodeCount - 1)];
}

FVector AMagatsuneBoss::GetRouteWorld(int32 N) const { return MovingRoot->GetComponentTransform().TransformPosition(GetRouteLocal(N)); }

void AMagatsuneBoss::BeginPlay()
{
    Super::BeginPlay();
    SpawnTransform = GetActorTransform();
    GrabFrame = GetWorld()->SpawnActor<AActor>();
    auto* Frame = NewObject<USceneComponent>(GrabFrame, TEXT("RootGrabFrame"));
    GrabFrame->SetRootComponent(Frame);
    Frame->RegisterComponent();
    GrabFrame->AttachToComponent(MovingRoot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    GrabFrame->SetOwner(this);
    for (int32 I = 0; I < 3; ++I)
    {
        auto* K = GetWorld()->SpawnActor<AKakonActor>();
        K->SetOwner(this);
        K->AttachToComponent(MovingRoot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        K->SetActorRelativeLocation(GetRouteLocal(KakonNodes[I]));
        K->SetActorEnableCollision(false);
        K->OnPurified.AddUniqueDynamic(this, &AMagatsuneBoss::HandlePurified);
        Kakons.Add(K);
        RegisterKakon(K);
    }
    TArray<UStaticMeshComponent*> Parts;
    GetComponents(Parts);
    for (auto* P : Parts)
    {
        const bool bRoutePart = P->GetName().StartsWith(TEXT("Route"));
        SetPrimitiveColor(P->CreateDynamicMaterialInstance(0), bRoutePart ? FLinearColor(.08, .75, .5) : FLinearColor(.025, .018, .03));
    }
    ResetNushi();
}

void AMagatsuneBoss::EndPlay(const EEndPlayReason::Type R)
{
    if (IsValid(GrabFrame)) GrabFrame->Destroy();
    for (AKakonActor* K : Kakons)
        if (IsValid(K)) K->Destroy();
    Super::EndPlay(R);
}

AKakonActor* AMagatsuneBoss::GetKakon(int32 I) const { return Kakons.IsValidIndex(I) ? Kakons[I].Get() : nullptr; }

FString AMagatsuneBoss::GetPhaseLabel() const { return StaticEnum<EMagatsunePhase>()->GetNameStringByValue(static_cast<int64>(Phase)); }

bool AMagatsuneBoss::IsRouteNodeEnabled(int32 N) const
{
    const int32 C = GetNushiProgressComponent()->GetPurifiedCount();
    return N >= 0 && N < RouteNodeCount &&
        N <= (C == 0 ? 3
                     : C == 1 ? 7
                              : 11) &&
        !(Phase == EMagatsunePhase::RootRockRoute && PhaseTime < TransitionSeconds) &&
        !(Phase == EMagatsunePhase::FinalRise && PhaseTime < TransitionSeconds);
}

int32 AMagatsuneBoss::GetRecoveryNode() const
{
    const int32 C = GetNushiProgressComponent()->GetPurifiedCount();
    return C >= 2 ? 8 : C >= 1 ? 4 : 0;
}

FVector AMagatsuneBoss::GetRecoveryWorld() const { return GetRouteWorld(GetRecoveryNode()) + MovingRoot->GetRightVector() * 170.f; }

void AMagatsuneBoss::ResetNushi()
{
    Super::ResetNushi();
    RootCueLight->SetIntensity(0.f);
    SetActorTransform(SpawnTransform);
    MovingRoot->SetRelativeTransform(FTransform::Identity);
    Phase = EMagatsunePhase::SurfaceRoot;
    PhaseTime = CalmTime = Anticipation = 0;
    bLargePulse = bPulseResolved = false;
    Telemetry = FMagatsuneTelemetry();
    for (int32 I = 0; I < Kakons.Num(); ++I)
    {
        Kakons[I]->SetActorHiddenInGame(I != 0);
        if (I == 0) Kakons[I]->ApplyShellDamage(Kakons[I]->MaxShellHealth);
    }
    RefreshRoutes();
}

void AMagatsuneBoss::Tick(float Dt)
{
    Super::Tick(Dt);
    if (!FMath::IsFinite(Dt) || Dt <= 0) return;
    AdvancePresentation(Dt);
    if (GetNushiState() == ENushiState::Active) Telemetry.Elapsed += Dt;
    if (Phase == EMagatsunePhase::SurfaceRoot)
    {
        PhaseTime += Dt;
        const float Wave = FMath::Sin(PhaseTime * (2 * PI / FMath::Max(.1f, PulsePeriod)));
        const float Cycle = FMath::Fmod(PhaseTime, FMath::Max(.1f, PulsePeriod)) / FMath::Max(.1f, PulsePeriod);
        // The cue precedes the unchanged Wave > .78 gameplay check below.
        Anticipation = FMath::SmoothStep(0.f, 1.f, FMath::Clamp((Cycle - .04f) / .10f, 0.f, 1.f)) * (bLargePulse ? 0.f : 1.f);
        MovingRoot->SetRelativeLocation({0, 0, 35 * Wave});
        MovingRoot->SetRelativeRotation({0, 3 * Wave, 7 * Wave});
        bLargePulse = Wave > .78f;
        if (bLargePulse && !bPulseResolved)
        {
            bPulseResolved = true;
            ++Telemetry.LargePulses;
            if (Player) Player->ResolveLargePulse();
            LogTelemetry(TEXT("LargePulse"));
        }
        if (Wave < 0) bPulseResolved = false;
    }
    else if (Phase == EMagatsunePhase::RootRockRoute || Phase == EMagatsunePhase::FinalRise)
    {
        PhaseTime = FMath::Min(PhaseTime + Dt, TransitionSeconds);
        const float T = FMath::SmoothStep(0.f, 1.f, PhaseTime / FMath::Max(.01f, TransitionSeconds));
        if (Phase == EMagatsunePhase::RootRockRoute)
            MovingRoot->SetRelativeTransform(FTransform(FRotator(-8 * T, 12 * T, 10 * T), FVector(0, 0, 120 * T)));
        else
        {
            MovingRoot->SetRelativeTransform(
                FTransform(FRotator(FMath::Lerp(-8.f, 38.f, T), FMath::Lerp(12.f, -18.f, T), FMath::Lerp(10.f, 31.f, T)),
                    FVector(0, 0, FMath::Lerp(120.f, 390.f, T))));
            bLargePulse = T > .42f && T < .72f;
            Anticipation = bLargePulse ? 0.f : FMath::SmoothStep(0.f, 1.f, FMath::Clamp(T / .38f, 0.f, 1.f));
            if (bLargePulse && !bPulseResolved)
            {
                bPulseResolved = true;
                ++Telemetry.LargePulses;
                if (Player) Player->ResolveLargePulse();
                LogTelemetry(TEXT("FinalLargePulse"));
            }
        }
        if (PhaseTime >= TransitionSeconds)
        {
            bLargePulse = false;
            Anticipation = 0.f;
            RefreshRoutes();
        }
    }
    else if (Phase == EMagatsunePhase::Calming)
    {
        Anticipation = 0.f;
        CalmTime = FMath::Min(CalmTime + Dt, CalmSeconds);
        const float T = FMath::SmoothStep(0.f, 1.f, CalmTime / FMath::Max(.01f, CalmSeconds));
        MovingRoot->SetRelativeTransform(
            FTransform(FMath::Lerp(FRotator(38, -18, 31), FRotator(5, 0, 0), T), FVector(0, 0, FMath::Lerp(390.f, 220.f, T))));
        if (CalmTime >= CalmSeconds)
        {
            Phase = EMagatsunePhase::Calm;
            LogTelemetry(TEXT("Calm_Completed_Victory"));
        }
    }
    // These offsets belong only to non-colliding art. GrabFrame, RouteMarkers and
    // Kakons remain direct children of the authoritative MovingRoot transform.
    const float VisualStrength = (1.f - GetCalmPresentation()) * (3.f + 7.f * Anticipation);
    for (int32 I = 0; I < RootVisuals.Num(); ++I)
    {
        const float LocalWave = FMath::Sin(PhaseTime * 2.f * PI / FMath::Max(.1f, PulsePeriod) - I * .31f);
        RootVisuals[I]->SetRelativeLocation(RootVisualBaseLocations[I] + FVector(0, 0, LocalWave * VisualStrength));
        RootVisuals[I]->SetRelativeRotation(FRotator(LocalWave * .35f, 0, LocalWave * .65f));
    }
    // The eruption is a landscape pulse, not an extra creature attack.
    const float Purify = GetPurificationPresentation();
    RootCueLight->SetLightColor(Purify > 0.f ? FLinearColor(.12f,.7f,.55f)
        : GetNushiState()==ENushiState::Calm ? FLinearColor(.18f,.36f,.42f) : FLinearColor(.45f,.08f,.10f));
    RootCueLight->SetIntensity(Purify > 0.f ? 22000.f * Purify
        : GetNushiState()==ENushiState::Active && (bLargePulse || Anticipation > 0.f)
            ? FMath::Lerp(3500.f,26000.f,bLargePulse?1.f:Anticipation)*(.9f+.1f*FMath::Sin(GetWorld()->GetTimeSeconds()*7.f))
            : GetNushiState()==ENushiState::Calm ? 1200.f*(1.f-GetCalmPresentation()) : 0.f);
}

float AMagatsuneBoss::GetTransitionAlpha() const
{
    return Phase == EMagatsunePhase::RootRockRoute || Phase == EMagatsunePhase::FinalRise
        ? FMath::Clamp(PhaseTime / FMath::Max(.01f, TransitionSeconds), 0.f, 1.f) : 0.f;
}

FVector AMagatsuneBoss::GetPresentationFocus() const
{
    const int32 Node = Phase == EMagatsunePhase::RootRockRoute ? 7 : Phase == EMagatsunePhase::FinalRise || Phase == EMagatsunePhase::Calming ? 11 : 3;
    return GetRouteWorld(Node);
}

bool AMagatsuneBoss::TryPurify()
{
    if (!Player || GetNushiState() != ENushiState::Active) return false;
    const int32 I = GetNushiProgressComponent()->GetPurifiedCount();
    return I < 3 && Player->IsMounted() && !Player->IsRouteMoving() && Player->GetRouteNode() == KakonNodes[I] && GetKakon(I)->Purify();
}

void AMagatsuneBoss::HandlePurified(AKakonActor* K)
{
    const int32 I = Kakons.IndexOfByKey(K);
    if (I == INDEX_NONE) return;
    Telemetry.KakonPurified = I + 1;
    NotifyPurificationPresentation();
    LogTelemetry(*FString::Printf(TEXT("Kakon%d"), I + 1));
    if (I < 2)
    {
        Phase = I == 0 ? EMagatsunePhase::RootRockRoute : EMagatsunePhase::FinalRise;
        PhaseTime = 0;
        bPulseResolved = false;
        ++Telemetry.PhaseTransitions;
        GetKakon(I + 1)->SetActorHiddenInGame(false);
        GetKakon(I + 1)->ApplyShellDamage(GetKakon(I + 1)->MaxShellHealth);
        RefreshRoutes();
    }
    else
    {
        Telemetry.ClearSeconds = Telemetry.Elapsed;
        Phase = EMagatsunePhase::Calming;
        CalmTime = 0;
    }
}

void AMagatsuneBoss::RefreshRoutes()
{
    const bool Reveal = IsDebugGuidanceEnabled() || (Player && Player->GetSense()->IsBoundarySenseActive());
    for (int32 I = 0; I < RouteMarkers.Num(); ++I) RouteMarkers[I]->SetVisibility(Reveal && IsRouteNodeEnabled(I));
}

void AMagatsuneBoss::LogTelemetry(const TCHAR* E) const
{
    UE_LOG(LogTemp, Display,
        TEXT(
            "MAGATSUNE_TELEMETRY %s grab=%d/%d transitions=%d kakon=%d cling=%.2f pulses=%d falls=%d recovery=%d exhaustion=%d retry=%d elapsed=%.2f clear=%.2f"),
        E, Telemetry.GrabSuccesses, Telemetry.GrabAttempts, Telemetry.PhaseTransitions, Telemetry.KakonPurified, Telemetry.ClingSeconds,
        Telemetry.LargePulses, Telemetry.Falls, Telemetry.Recoveries, Telemetry.Exhaustions, Telemetry.Retries, Telemetry.Elapsed,
        Telemetry.ClearSeconds);
}
