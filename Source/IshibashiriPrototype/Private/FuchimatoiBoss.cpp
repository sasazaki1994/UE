#include "FuchimatoiBoss.h"
#include "FuchimatoiArena.h"
#include "FuchimatoiPlayer.h"
#include "PlayerSenseComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "FuchimatoiRouteAnchor.h"
#include "FuchimatoiSimulationComponent.h"
#include "KakonActor.h"
#include "NushiProgressComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PrimitiveAppearance.h"
#include "DrawDebugHelpers.h"

namespace
{
    const FVector BodyOffsets[] = {
        {0,0,0},{-250,0,130},{-500,0,270},{-780,0,390},
        {-1080,0,340},{-1350,0,250},{-1620,0,170},{-1880,0,90},
        {-2100,0,-20},{-2330,0,-100},{-2550,0,-140}
    };
    // Coordinates relative to the arena's origin. Converted to boss local space
    // so actor transforms carry the serpent while the rock nodes stay on terrain.
    const FVector CoiledBody[] = {
        {-120,-280,1822},{250,-260,1602},{430,100,1372},{-620,80,670},
        {-460,300,700},{-120,570,972},{-30,800,620},{-420,700,280},
        {-900,400,100},{-1350,100,60},{-1650,-200,60}
    };
    const int32 BodyForNode[] = {0,1,2,3,-1,5,-1,2,1,0};
    const int32 KakonNodes[] = {3,7,9};
}

AFuchimatoiBoss::AFuchimatoiBoss()
{
    PrimaryActorTick.bCanEverTick = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SerpentRoot")));
    Simulation=CreateDefaultSubobject<UFuchimatoiSimulationComponent>(TEXT("PlayableSimulation"));
}

void AFuchimatoiBoss::BeginBiteWindup()
{
    TryTransition(EFuchimatoiActionState::Submerged, EFuchimatoiActionState::BiteWindup);
}

void AFuchimatoiBoss::BeginBiteLunge()
{
    TryTransition(EFuchimatoiActionState::BiteWindup, EFuchimatoiActionState::BiteLunge);
}

void AFuchimatoiBoss::NotifyHeadSnagged()
{
    TryTransition(EFuchimatoiActionState::BiteLunge, EFuchimatoiActionState::Snagged);
}

void AFuchimatoiBoss::BeginCoiling()
{
    if (ActionState == EFuchimatoiActionState::Snagged)
    {
        CoilingProgress = 0.f;
    }
    TryTransition(EFuchimatoiActionState::Snagged, EFuchimatoiActionState::Coiling);
}

void AFuchimatoiBoss::ReturnToSubmerged()
{
    TryTransition(EFuchimatoiActionState::Coiling, EFuchimatoiActionState::Submerged);
}

void AFuchimatoiBoss::ResetFuchimatoi()
{
    ResetNushi();
}

void AFuchimatoiBoss::ResetNushi()
{
    if (Telemetry.Elapsed>0) LogTelemetry(TEXT("Retry"));
    Telemetry=FFuchimatoiTelemetry();
    Super::ResetNushi();
    CoilingProgress = 0.f;
    SetActionState(EFuchimatoiActionState::Submerged);
    HeadProxyLocalLocation = FVector::ZeroVector;
    BiteTargetLocalLocation = FVector::ZeroVector;
    bHasBiteTarget = false;
    ActionTimeRemaining = SubmergedDuration;
    if (bPlayable)
    {
        SetActorTransform(EncounterSpawn);
        for (UStaticMeshComponent* Marker : KakonMarkers) Marker->SetVisibility(true);
        UpdateBody();
    }
}

void AFuchimatoiBoss::SetBiteTargetLocalLocation(const FVector& NewTargetLocalLocation)
{
    BiteTargetLocalLocation = NewTargetLocalLocation;
    bHasBiteTarget = true;
}

void AFuchimatoiBoss::AdvanceBiteLunge(float DeltaSeconds)
{
    if (ActionState != EFuchimatoiActionState::BiteLunge
        || !bHasBiteTarget
        || DeltaSeconds <= 0.f
        || BiteLungeSpeed <= 0.f
        || !FMath::IsFinite(DeltaSeconds)
        || !FMath::IsFinite(BiteLungeSpeed))
    {
        return;
    }

    HeadProxyLocalLocation = FMath::VInterpConstantTo(
        HeadProxyLocalLocation,
        BiteTargetLocalLocation,
        DeltaSeconds,
        BiteLungeSpeed);

    if (HeadProxyLocalLocation.Equals(BiteTargetLocalLocation, KINDA_SMALL_NUMBER))
    {
        HeadProxyLocalLocation = BiteTargetLocalLocation;
    }
}

void AFuchimatoiBoss::AdvanceCoiling(float DeltaSeconds)
{
    if (ActionState != EFuchimatoiActionState::Coiling
        || DeltaSeconds <= 0.f
        || CoilingDuration <= 0.f
        || !FMath::IsFinite(DeltaSeconds)
        || !FMath::IsFinite(CoilingDuration))
    {
        return;
    }

    // Divide in double precision so even extreme finite float inputs cannot overflow.
    const double ProgressDelta = static_cast<double>(DeltaSeconds) / CoilingDuration;
    CoilingProgress = static_cast<float>(FMath::Clamp(CoilingProgress + ProgressDelta, 0.0, 1.0));
}

bool AFuchimatoiBoss::IsBiteTargetReached() const
{
    return bHasBiteTarget
        && HeadProxyLocalLocation.Equals(BiteTargetLocalLocation, KINDA_SMALL_NUMBER);
}

void AFuchimatoiBoss::TryTransition(
    EFuchimatoiActionState ExpectedState,
    EFuchimatoiActionState NewState)
{
    if (ActionState == ExpectedState)
    {
        SetActionState(NewState);
    }
}

void AFuchimatoiBoss::SetActionState(EFuchimatoiActionState NewState)
{
    if (ActionState == NewState)
    {
        return;
    }

    ActionState = NewState;
    OnFuchimatoiActionStateChanged.Broadcast();
}

void AFuchimatoiBoss::ConfigureEncounter(AFuchimatoiArena* InArena, AFuchimatoiPlayer* InPlayer)
{
    if (bPlayable || !InArena || !InPlayer) return;
    Arena=InArena; Player=InPlayer; EncounterSpawn=GetActorTransform();
    bPlayable=true;
    Simulation->AddTickPrerequisiteComponent(Player->GetCharacterMovement());
    CreatePrimitiveBody();
    UpdateBody();
}

void AFuchimatoiBoss::CreatePrimitiveBody()
{
    UStaticMesh* Sphere=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    UStaticMesh* Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UMaterialInterface* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    auto Mesh=[&](const FString& Name,UStaticMesh* Shape,FLinearColor Color)
    {
        UStaticMeshComponent* Part=NewObject<UStaticMeshComponent>(this,*Name);
        AddInstanceComponent(Part); Part->SetupAttachment(RootComponent);
        Part->SetStaticMesh(Shape); Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetMaterial(0,Material);
        Part->SetCanEverAffectNavigation(false); Part->RegisterComponent();
        SetPrimitiveColor(Part->CreateDynamicMaterialInstance(0),Color);
        return Part;
    };
    for (int32 I=0; I<UE_ARRAY_COUNT(BodyOffsets); ++I)
    {
        UStaticMeshComponent* Joint=Mesh(FString::Printf(TEXT("SerpentJoint%d"),I),Sphere,FLinearColor(.08,.36,.27));
        BodyJoints.Add(Joint);
        if (I>0) BodySegments.Add(Mesh(FString::Printf(TEXT("SerpentSegment%d"),I),Cylinder,
            I%2==0?FLinearColor(.13,.43,.29):FLinearColor(.23,.50,.32)));
    }
    Head=BodyJoints[0];
    for (int32 Side : {-1,1})
    {
        UStaticMeshComponent* Eye=Mesh(FString::Printf(TEXT("Eye%d"),Side),Sphere,FLinearColor(1,.65,.04));
        Eye->AttachToComponent(Head,FAttachmentTransformRules::KeepRelativeTransform);
        Eye->SetRelativeLocation(FVector(25,Side*35,24)); Eye->SetRelativeScale3D(FVector(.12));
    }
    for (int32 Node=0; Node<RouteNodeCount; ++Node)
    {
        AFuchimatoiRouteAnchor* Anchor=Arena->GetRockAnchor(Node);
        if (!Anchor)
        {
            Anchor=GetWorld()->SpawnActor<AFuchimatoiRouteAnchor>();
            Anchor->AttachToActor(this,FAttachmentTransformRules::KeepWorldTransform);
            Anchor->Configure(false,Node);
        }
        RouteAnchors.Add(Anchor);
    }
    RecoveryAnchor=GetWorld()->SpawnActor<AFuchimatoiRouteAnchor>();
    RecoveryAnchor->AttachToActor(this,FAttachmentTransformRules::KeepWorldTransform);
    // Clear floor south of the first coiled ascent, away from rock collision.
    RecoveryAnchor->SetActorRelativeLocation(EncounterSpawn.InverseTransformPosition(FVector(-850,-180,190)));
    RecoveryAnchor->Configure(false,INDEX_NONE);
    RecoveryAnchor->SetGuidance(false,true,true);
    for (int32 I=0; I<3; ++I)
    {
        AKakonActor* Kakon=GetWorld()->SpawnActorDeferred<AKakonActor>(AKakonActor::StaticClass(),FTransform::Identity,this);
        Kakon->MaxShellHealth=0;
        Kakon->FinishSpawning(FTransform::Identity);
        Kakon->AttachToActor(RouteAnchors[KakonNodes[I]],FAttachmentTransformRules::KeepRelativeTransform);
        Kakon->SetActorRelativeLocation(FVector(0,0,-30));
        RegisterKakon(Kakon);
        Kakon->OnPurified.AddUniqueDynamic(this,&AFuchimatoiBoss::HandleKakonPurified);
        KakonActors.Add(Kakon);
        UStaticMeshComponent* Marker=Mesh(FString::Printf(TEXT("Kakon%d"),I),Sphere,FLinearColor(1,.03,.04));
        Marker->AttachToComponent(Kakon->GetRootComponent(),FAttachmentTransformRules::KeepRelativeTransform);
        Marker->SetRelativeLocation(FVector::ZeroVector); Marker->SetRelativeScale3D(FVector(.75));
        KakonMarkers.Add(Marker);
    }
}

void AFuchimatoiBoss::UpdateBody()
{
    if (!bPlayable) return;
    Arena->ShowBaitSpot(!IsCoiling() && GetNushiState()!=ENushiState::Calm);
    TArray<FVector> Points;
    for (int32 I=0; I<BodyJoints.Num(); ++I)
    {
        const FVector CoilLocal=EncounterSpawn.InverseTransformPosition(CoiledBody[I]);
        const FVector Local=FMath::Lerp(HeadProxyLocalLocation+BodyOffsets[I],CoilLocal,CoilingProgress);
        Points.Add(Local);
        BodyJoints[I]->SetRelativeLocation(Local);
        const float Width=I==0?2.25f:FMath::Lerp(1.9f,.5f,float(I)/10.f);
        BodyJoints[I]->SetRelativeScale3D(FVector(Width));
        if (I>0)
        {
            UStaticMeshComponent* Segment=BodySegments[I-1];
            const FVector Along=Local-Points[I-1];
            Segment->SetRelativeLocation((Local+Points[I-1])*.5f);
            Segment->SetRelativeRotation(FRotationMatrix::MakeFromZ(Along).Rotator());
            Segment->SetRelativeScale3D(FVector(Width,Width,Along.Size()/100.f));
        }
    }
    for (int32 Node=0; Node<RouteNodeCount; ++Node)
    {
        if (BodyForNode[Node]>=0) RouteAnchors[Node]->SetActorRelativeLocation(Points[BodyForNode[Node]]+FVector(0,0,178));
        const int32 Current=Player?Player->GetRouteNode():INDEX_NONE;
        const bool bMounted=Player && Player->IsMounted();
        const bool bHeadGrab=Node==0 && CanMount() && !bMounted;
        const int32 Next=Player?Player->GetGuidanceNode():INDEX_NONE;
        const bool SenseReveal=FParse::Param(FCommandLine::Get(),TEXT("DebugGuidance"))||(Player&&Player->GetSense()->IsBoundarySenseActive());
        RouteAnchors[Node]->SetGuidance(bHeadGrab || (SenseReveal && bMounted && (Node==Current || Node==Next)
            && (Node<=3 || IsCoilingComplete())),bHeadGrab || Node==Next,bHeadGrab);
    }
    RecoveryAnchor->SetGuidance(CanRecover(),true,true);
    for (int32 I=0; I<KakonMarkers.Num(); ++I)
    {
        const bool bPurified=KakonActors[I]->GetState()==EKakonState::Purified;
        const bool bActive=!bPurified && (I==0 || (IsCoilingComplete()
            && KakonActors[I-1]->GetState()==EKakonState::Purified));
        const bool SenseReveal=FParse::Param(FCommandLine::Get(),TEXT("DebugGuidance"))||(Player&&Player->GetSense()->IsBoundarySenseActive());
        KakonMarkers[I]->SetVisibility(SenseReveal&&!bPurified);
        KakonMarkers[I]->SetRelativeScale3D(FVector(bActive?.85f:.4f));
        SetPrimitiveColor(Cast<UMaterialInstanceDynamic>(KakonMarkers[I]->GetMaterial(0)),
            bActive?FLinearColor(1,.03,.04):FLinearColor(.09,.04,.04));
    }
    if (ActionState==EFuchimatoiActionState::BiteWindup && Player)
        Head->SetWorldRotation((Player->GetActorLocation()-Head->GetComponentLocation()).Rotation());
    const FLinearColor Color=GetNushiState()==ENushiState::Calm?FLinearColor(.3,.6,.95)
        :IsSnagged()?FLinearColor(.7,.95,.2):ActionState==EFuchimatoiActionState::BiteWindup?FLinearColor(1,.25,.03):FLinearColor(.06,.3,.25);
    SetPrimitiveColor(Cast<UMaterialInstanceDynamic>(Head->GetMaterial(0)),Color);
}

void AFuchimatoiBoss::WithdrawHead()
{
    HeadProxyLocalLocation=FVector::ZeroVector;
    BiteTargetLocalLocation=FVector::ZeroVector; bHasBiteTarget=false;
    SetActionState(EFuchimatoiActionState::Submerged);
    ActionTimeRemaining=SubmergedDuration;
}

void AFuchimatoiBoss::AdvanceEncounter(float Dt)
{
    if (!bPlayable || !Player || !Player->CanAct() || GetNushiState()!=ENushiState::Active) return;
    Telemetry.Elapsed+=FMath::Max(0.f,Dt);
    // Bound sweeps independently of render FPS and preserve phase time at boundaries.
    float Remaining=FMath::Max(0.f,Dt);
    while (Remaining>KINDA_SMALL_NUMBER)
    {
        float Step=FMath::Min(Remaining,1.f/120.f);
        if (ActionTimeRemaining>0) Step=FMath::Min(Step,ActionTimeRemaining);
        Remaining-=Step; ActionTimeRemaining=FMath::Max(0.f,ActionTimeRemaining-Step);
        switch (ActionState)
        {
        case EFuchimatoiActionState::Submerged:
            if (ActionTimeRemaining<=0)
            {
                BeginBiteWindup(); ActionTimeRemaining=WindupDuration;
                ++Telemetry.BiteAttempts; LogTelemetry(TEXT("Bite"));
            }
            break;
        case EFuchimatoiActionState::BiteWindup:
            BiteAimWorldLocation=Player->GetActorLocation();
            if (ActionTimeRemaining<=0)
            {
                FVector Aim=Player->GetActorLocation(); Aim.Z=GetActorLocation().Z;
                const FVector Direction=(Aim-GetHeadWorldLocation()).GetSafeNormal2D();
                // Continue through the locked player point toward any rock behind it.
                SetBiteTargetLocalLocation(GetActorTransform().InverseTransformPosition(Aim+Direction*750.f));
                BeginBiteLunge();
            }
            break;
        case EFuchimatoiActionState::BiteLunge:
        {
            const FVector Start=GetActorTransform().TransformPosition(HeadProxyLocalLocation);
            AdvanceBiteLunge(Step);
            const FVector End=GetActorTransform().TransformPosition(HeadProxyLocalLocation);
            FHitResult Hit;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(FuchimatoiBite),false,this);
            FCollisionObjectQueryParams Objects; Objects.AddObjectTypesToQuery(ECC_WorldStatic);
            GetWorld()->SweepSingleByObjectType(Hit,Start,End,FQuat::Identity,Objects,FCollisionShape::MakeSphere(105.f),Params);
            const FVector ContactEnd=Hit.bBlockingHit?Hit.Location:End;
            const FVector PlayerPoint=Player->GetActorLocation();
            const float Distance=FMath::PointDistToSegment(FVector(PlayerPoint.X,PlayerPoint.Y,Start.Z),Start,ContactEnd);
            if (!Player->IsMounted() && Distance<145.f && FMath::Abs(PlayerPoint.Z-Start.Z)<210.f && Player->ReceiveBite())
            {
                ++Telemetry.PlayerHits; LogTelemetry(TEXT("PlayerHit"));
                WithdrawHead(); break;
            }
            if (Hit.bBlockingHit)
            {
                if (Hit.GetComponent()==Arena->GetBaitRock())
                {
                    HeadProxyLocalLocation=GetActorTransform().InverseTransformPosition(Hit.Location);
                    NotifyHeadSnagged(); ActionTimeRemaining=SnaggedDuration;
                    ++Telemetry.RockLures; ++Telemetry.Snags; LogTelemetry(TEXT("Snag"));
                }
                else WithdrawHead();
            }
            else if (IsBiteTargetReached()) WithdrawHead();
            break;
        }
        case EFuchimatoiActionState::Snagged:
            if (!Player->IsMounted() && ActionTimeRemaining<=0) { LogTelemetry(TEXT("MissedGrab")); WithdrawHead(); }
            break;
        case EFuchimatoiActionState::Coiling: AdvanceCoiling(Step); break;
        }
    }
    UpdateBody();
    if (ActionState==EFuchimatoiActionState::BiteWindup || ActionState==EFuchimatoiActionState::BiteLunge)
    {
        const bool bLocked=ActionState==EFuchimatoiActionState::BiteLunge;
        FVector Target=BiteAimWorldLocation; Target.Z=8;
        const FColor Color=bLocked?FColor::Red:FColor::Orange;
        DrawDebugCircle(GetWorld(),Target,115,32,Color,false,-1,0,8,FVector::ForwardVector,FVector::RightVector,false);
        DrawDebugDirectionalArrow(GetWorld(),GetHeadWorldLocation(),Target,60,Color,false,-1,0,5);
        DrawDebugDirectionalArrow(GetWorld(),Arena->GetBaitPosition(),Arena->GetBaitRock()->GetComponentLocation(),60,FColor::Yellow,false,-1,0,6);
    }
    if (CanRecover())
    {
        const FVector Point=RecoveryAnchor->GetActorLocation();
        DrawDebugLine(GetWorld(),Point-FVector(0,0,180),Point+FVector(0,0,220),FColor(255,215,70),false,-1,0,10);
    }
    if (Player->IsMounted())
    {
        const int32 Node=Player->GetGuidanceNode();
        const AFuchimatoiRouteAnchor* From=Player->IsRecovering()?RecoveryAnchor.Get():GetRouteAnchor(Player->GetRouteNode());
        if (From && Node>=0 && Node<(IsCoilingComplete()?RouteNodeCount:4))
        {
            DrawDebugLine(GetWorld(),From->GetActorLocation()-FVector(0,0,65),
                RouteAnchors[Node]->GetActorLocation()-FVector(0,0,65),Node==4||Node==6?FColor::Yellow:FColor::Cyan,false,-1,0,5);
        }
    }
}

AFuchimatoiRouteAnchor* AFuchimatoiBoss::GetRouteAnchor(int32 Node) const
{
    return RouteAnchors.IsValidIndex(Node)?RouteAnchors[Node].Get():nullptr;
}
AKakonActor* AFuchimatoiBoss::GetKakon(int32 Index) const
{
    return KakonActors.IsValidIndex(Index)?KakonActors[Index].Get():nullptr;
}
FVector AFuchimatoiBoss::GetHeadWorldLocation() const
{
    return Head ? Head->GetComponentLocation() : GetActorTransform().TransformPosition(HeadProxyLocalLocation);
}
bool AFuchimatoiBoss::CanMount() const
{
    return bPlayable && IsSnagged() && ActionTimeRemaining>0 && GetNushiState()==ENushiState::Active;
}
bool AFuchimatoiBoss::IsRecoveryUnlocked() const
{
    return bPlayable && IsCoilingComplete() && IsCoiling() && GetNushiState()==ENushiState::Active
        && Player && Player->CanAct();
}
bool AFuchimatoiBoss::CanRecover() const
{
    return IsRecoveryUnlocked() && Player->IsOnRecoveryGround();
}
void AFuchimatoiBoss::RecordGrab(bool bSuccess, bool bRecovery)
{
    ++Telemetry.GrabAttempts;
    if (bSuccess) { ++Telemetry.GrabSuccesses; if (bRecovery) ++Telemetry.RecoveryGrabs; }
    LogTelemetry(bSuccess?(bRecovery?TEXT("RecoveryGrab"):TEXT("Grab")):TEXT("GrabRejected"));
}
void AFuchimatoiBoss::RecordFall() { ++Telemetry.Falls; LogTelemetry(TEXT("Fall")); }
void AFuchimatoiBoss::LogTelemetry(const TCHAR* Event) const
{
    UE_LOG(LogTemp,Display,TEXT("FUCHIMATOI_TELEMETRY event=%s bite_attempts=%d player_hits=%d rock_lures=%d snags=%d grab_attempts=%d grab_successes=%d falls=%d recovery_grabs=%d kakon_purified=%d elapsed=%.2f completion_time=%.2f"),
        Event,Telemetry.BiteAttempts,Telemetry.PlayerHits,Telemetry.RockLures,Telemetry.Snags,
        Telemetry.GrabAttempts,Telemetry.GrabSuccesses,Telemetry.Falls,Telemetry.RecoveryGrabs,
        GetNushiProgressComponent()->GetPurifiedCount(),Telemetry.Elapsed,GetNushiState()==ENushiState::Calm?Telemetry.Elapsed:-1.f);
}
bool AFuchimatoiBoss::TryPurifyAtNode(int32 Node)
{
    if (!Player || !Player->CanAct() || !Player->IsMounted() || Player->IsRouteMoving() || Player->GetRouteNode()!=Node) return false;
    for (int32 I=0; I<3; ++I)
    {
        if (Node!=KakonNodes[I] || (I>0 && (!IsCoilingComplete() || KakonActors[I-1]->GetState()!=EKakonState::Purified))) continue;
        if (FVector::Dist(Player->GetActorLocation(),KakonActors[I]->GetActorLocation())>150.f) return false;
        return KakonActors[I]->Purify();
    }
    return false;
}
void AFuchimatoiBoss::HandleKakonPurified(AKakonActor* Kakon)
{
    if (KakonActors.Num()>0 && Kakon==KakonActors[0]) BeginCoiling();
    LogTelemetry(TEXT("Purified"));
    UpdateBody();
}
FString AFuchimatoiBoss::GetActionLabel() const
{
    if (GetNushiState()==ENushiState::Calm) return TEXT("CALM");
    switch (ActionState)
    {
    case EFuchimatoiActionState::Submerged: return TEXT("LURE THE BITE INTO THE ROCK - stand on the gold square");
    case EFuchimatoiActionState::BiteWindup: return TEXT("LURE THE BITE INTO THE ROCK - orange target follows you");
    case EFuchimatoiActionState::BiteLunge: return TEXT("TARGET LOCKED (RED) - DODGE SIDEWAYS NOW");
    case EFuchimatoiActionState::Snagged: return Player && Player->IsMounted()?TEXT("CLIMB TO THE RED CORE"):TEXT("GRAB NOW - E / RB at the gold head marker");
    case EFuchimatoiActionState::Coiling: return IsCoilingComplete()
        ?(Player && !Player->IsMounted()?TEXT("RETURN VIA THE GOLD BEACON - progress is kept"):TEXT("COILED - follow the next cyan / gold node"))
        :TEXT("COILING - cling to the moving serpent");
    }
    return FString();
}
float AFuchimatoiBoss::GetBodyLength() const
{
    float Length=0;
    for (int32 I=1; I<BodyJoints.Num(); ++I) Length+=FVector::Dist(BodyJoints[I-1]->GetComponentLocation(),BodyJoints[I]->GetComponentLocation());
    return Length;
}
