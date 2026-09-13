#include "MinedakiBoss.h"
#include "MinedakiPlayer.h"
#include "KakonActor.h"
#include "NushiProgressComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "PrimitiveAppearance.h"

AMinedakiBoss::AMinedakiBoss()
{
    PrimaryActorTick.bCanEverTick=true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("MinedakiRoot")));
    BodyRoot=CreateDefaultSubobject<USceneComponent>(TEXT("BodyRoot")); BodyRoot->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    auto Part=[&](const TCHAR* Name,FVector Position,FVector Scale)
    {
        auto* Mesh=CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Mesh->SetupAttachment(BodyRoot); Mesh->SetStaticMesh(Sphere.Object); Mesh->SetMaterial(0,Material.Object);
        Mesh->SetRelativeLocation(Position); Mesh->SetRelativeScale3D(Scale);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mesh->SetCanEverAffectNavigation(false);
        return Mesh;
    };
    Part(TEXT("Pelvis"),FVector(0,0,720),FVector(4,5,4));
    Part(TEXT("Torso"),FVector(0,0,1100),FVector(5,6.5,8));
    Part(TEXT("Head"),FVector(-40,0,1640),FVector(3.5,3.8,3.8));
    Part(TEXT("Muzzle"),FVector(-205,0,1580),FVector(1.5,2.7,1.7));
    LeftArm=Part(TEXT("LeftArm"),FVector(-70,-390,1020),FVector(2.2,2.2,9));
    RightArm=Part(TEXT("RightArm"),FVector(-70,390,1020),FVector(2.2,2.2,9));
    Part(TEXT("LeftLeg"),FVector(0,-205,350),FVector(2.5,2.5,7));
    Part(TEXT("RightLeg"),FVector(0,205,350),FVector(2.5,2.5,7));
    for(int32 I=0;I<RouteNodeCount;++I)
        RouteMarkers.Add(Part(*FString::Printf(TEXT("Route%d"),I),GetRouteLocal(I),FVector(.45)));
    CoreMarker=Part(TEXT("FirstKakonMarker"),GetRouteLocal(KakonNode)+FVector(-55,0,0),FVector(1.2));
}
FVector AMinedakiBoss::GetRouteLocal(int32 Node) const
{
    static const FVector Nodes[]={ {185,-205,180},{205,-205,450},{255,-150,740},{310,-80,970},
        {325,0,1190},{280,-175,1380},{230,-90,1500},{200,0,1660} };
    return Nodes[FMath::Clamp(Node,0,RouteNodeCount-1)];
}
FVector AMinedakiBoss::GetRouteWorld(int32 Node) const { return BodyRoot->GetComponentTransform().TransformPosition(GetRouteLocal(Node)); }
void AMinedakiBoss::BeginPlay()
{
    Super::BeginPlay(); SpawnPose=GetActorTransform();
    GrabFrame=GetWorld()->SpawnActor<AActor>();
    auto* FrameRoot=NewObject<USceneComponent>(GrabFrame,TEXT("BodyGrabFrame"));
    GrabFrame->SetRootComponent(FrameRoot); FrameRoot->RegisterComponent();
    GrabFrame->AttachToComponent(BodyRoot,FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    GrabFrame->SetOwner(this);
    for(int32 I=0;I<3;++I)
    {
        auto* Core=GetWorld()->SpawnActor<AKakonActor>(); Core->SetOwner(this);
        Core->AttachToComponent(BodyRoot,FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        // Two covered, hidden, collision-free placeholders reserve common progress slots.
        Core->SetActorRelativeLocation(I==0?GetRouteLocal(KakonNode):FVector(0,0,-100000));
        Core->SetActorHiddenInGame(I!=0); Core->SetActorEnableCollision(false);
        Kakons.Add(Core); RegisterKakon(Core);
    }
    TArray<UStaticMeshComponent*> Parts; GetComponents(Parts);
    for(auto* Part:Parts) SetPrimitiveColor(Part->CreateDynamicMaterialInstance(0),FLinearColor(.28,.32,.38));
    for(UStaticMeshComponent* Marker:RouteMarkers) SetPrimitiveColor(Cast<UMaterialInstanceDynamic>(Marker->GetMaterial(0)),FLinearColor(.1,1,.65));
    SetPrimitiveColor(Cast<UMaterialInstanceDynamic>(CoreMarker->GetMaterial(0)),FLinearColor(1,.07,.12));
    ResetNushi();
}
void AMinedakiBoss::EndPlay(const EEndPlayReason::Type Reason)
{
    if(IsValid(GrabFrame)) GrabFrame->Destroy();
    for(AKakonActor* Core:Kakons) if(IsValid(Core)) Core->Destroy();
    Super::EndPlay(Reason);
}
void AMinedakiBoss::ResetNushi()
{
    Super::ResetNushi(); ActionState=EMinedakiActionState::Grounded; ClimbTime=0; bShakeApplied=false;
    Telemetry=FMinedakiTelemetry(); SetActorTransform(SpawnPose); BodyRoot->SetRelativeRotation(FRotator::ZeroRotator);
    LeftArm->SetRelativeRotation(FRotator::ZeroRotator); RightArm->SetRelativeRotation(FRotator::ZeroRotator);
    LeftArm->SetRelativeLocation(FVector(-70,-390,1020)); RightArm->SetRelativeLocation(FVector(-70,390,1020));
    LeftArm->SetRelativeScale3D(FVector(2.2,2.2,9)); RightArm->SetRelativeScale3D(FVector(2.2,2.2,9));
    for(int32 I=0;I<RouteMarkers.Num();++I) RouteMarkers[I]->SetVisibility(I<=WallStartNode);
    if(CoreMarker) { CoreMarker->SetVisibility(false); SetPrimitiveColor(Cast<UMaterialInstanceDynamic>(CoreMarker->GetMaterial(0)),FLinearColor(1,.07,.12)); }
}
AKakonActor* AMinedakiBoss::GetKakon(int32 Index) const { return Kakons.IsValidIndex(Index)?Kakons[Index].Get():nullptr; }
bool AMinedakiBoss::IsSliceComplete() const { return GetKakon(0) && GetKakon(0)->GetState()==EKakonState::Purified; }
bool AMinedakiBoss::IsWallMoving() const { return ActionState!=EMinedakiActionState::Grounded && ActionState!=EMinedakiActionState::UpperPlatform; }
FString AMinedakiBoss::GetActionLabel() const { return StaticEnum<EMinedakiActionState>()->GetNameStringByValue(static_cast<int64>(ActionState)); }
void AMinedakiBoss::NotifyRouteNode(int32 Node)
{
    ++Telemetry.NodesReached;
    if(Node==WallStartNode && ActionState==EMinedakiActionState::Grounded && GetNushiState()==ENushiState::Active)
    { ActionState=EMinedakiActionState::PreparingClimb; ++Telemetry.WallStarts; LogTelemetry(TEXT("WallClimbStarted")); }
}
void AMinedakiBoss::Tick(float Dt) { Super::Tick(Dt); AdvanceWallClimb(Dt); }
void AMinedakiBoss::AdvanceWallClimb(float Dt)
{
    if(!FMath::IsFinite(Dt) || Dt<=0 || !FMath::IsFinite(PrepareSeconds) || PrepareSeconds<=0
        || !FMath::IsFinite(WallSeconds) || WallSeconds<=0 || !FMath::IsFinite(LedgeSeconds) || LedgeSeconds<=0
        || GetNushiState()!=ENushiState::Active || IsSliceComplete()) return;
    Telemetry.Elapsed+=Dt;
    if(!IsWallMoving()) return;
    const float Total=PrepareSeconds+WallSeconds+LedgeSeconds;
    ClimbTime=FMath::Min(ClimbTime+Dt,Total);
    FVector Position; FRotator Rotation;
    const auto Smooth=[](float T){ return T*T*(3-2*T); };
    if(ClimbTime<PrepareSeconds)
    {
        float T=Smooth(ClimbTime/PrepareSeconds);
        Position=FMath::Lerp(FVector::ZeroVector,FVector(-350,0,250),T);
        Rotation=FRotator(35*T,8*T,0); ActionState=EMinedakiActionState::PreparingClimb;
    }
    else if(ClimbTime<PrepareSeconds+WallSeconds)
    {
        float T=(ClimbTime-PrepareSeconds)/WallSeconds;
        float Lift=Smooth(FMath::Fmod(T*2,1.f));
        // Two distinct pulls, with a change of supporting arm between them.
        float Height=T<.5f?FMath::Lerp(250.f,1150.f,Lift):FMath::Lerp(1150.f,PlateauHeight+150.f,Lift);
        Position=FVector(FMath::Lerp(-350.f,-650.f,Smooth(T)),80*FMath::Sin(T*PI),Height);
        Rotation=FRotator(FMath::Lerp(35.f,MaximumPitch,FMath::Min(T*3,1.f)),FMath::Lerp(8.f,MaximumYaw,Smooth(T)),0);
        ActionState=EMinedakiActionState::ClimbingWall;
        if(T>=.4f && T<.65f)
        { ActionState=EMinedakiActionState::Shaking; Rotation.Roll=ShakeRoll*FMath::Sin((T-.4f)/.25f*PI); }
    }
    else
    {
        float T=Smooth((ClimbTime-PrepareSeconds-WallSeconds)/LedgeSeconds);
        Position=FMath::Lerp(FVector(-650,0,PlateauHeight+150),FVector(-2950,0,PlateauHeight),T);
        Rotation=FRotator(MaximumPitch*(1-T),MaximumYaw*(1-T),0);
        ActionState=EMinedakiActionState::LedgeTransition;
    }
    SetActorLocation(SpawnPose.TransformPosition(Position)); BodyRoot->SetRelativeRotation(Rotation);
    UpdateArmHolds();
    // Crossing check cannot skip the shake at low FPS or a large explicit advance.
    if(!bShakeApplied && ClimbTime>=PrepareSeconds+WallSeconds*.5f)
    {
        bShakeApplied=true; ++Telemetry.Shakes; LogTelemetry(TEXT("ArmRegripShake"));
        if(Player) Player->ResolveShake();
    }
    if(ClimbTime>=Total)
    {
        ActionState=EMinedakiActionState::UpperPlatform; ++Telemetry.UpperReached;
        LeftArm->SetRelativeRotation(FRotator::ZeroRotator); RightArm->SetRelativeRotation(FRotator::ZeroRotator);
        LeftArm->SetRelativeLocation(FVector(-70,-390,1020)); RightArm->SetRelativeLocation(FVector(-70,390,1020));
        LeftArm->SetRelativeScale3D(FVector(2.2,2.2,9)); RightArm->SetRelativeScale3D(FVector(2.2,2.2,9));
        for(UStaticMeshComponent* Marker:RouteMarkers) Marker->SetVisibility(true);
        GetKakon(0)->ApplyShellDamage(GetKakon(0)->MaxShellHealth); CoreMarker->SetVisibility(true);
        LogTelemetry(TEXT("UpperLedgeReached"));
    }
}
void AMinedakiBoss::UpdateArmHolds()
{
    // Stretch/aim the primitive arms between a body-local shoulder and simple
    // authored holds. No physics or skeletal IK; the GrabFrame is unaffected.
    const float WallT=FMath::Clamp((ClimbTime-PrepareSeconds)/WallSeconds,0.f,1.f);
    const float PrepareT=FMath::Clamp(ClimbTime/PrepareSeconds,0.f,1.f);
    const float LedgeT=FMath::Clamp((ClimbTime-PrepareSeconds-WallSeconds)/LedgeSeconds,0.f,1.f);
    for(int32 Side=0;Side<2;++Side)
    {
        const float Sign=Side==0?-1.f:1.f;
        const FVector Shoulder(-30,Sign*325,1350), RestHand(-70,Sign*390,570);
        // Left reaches high first; right changes support during the shake.
        const float Reach=Side==0?FMath::Clamp(WallT/.35f,0.f,1.f):FMath::Clamp((WallT-.4f)/.25f,0.f,1.f);
        const FVector Hold=SpawnPose.TransformPosition(FVector(-2040,Sign*480,FMath::Lerp(Side==0?600.f:1020.f,Side==0?1440.f:1860.f,Reach)));
        FVector LocalHand=BodyRoot->GetComponentTransform().InverseTransformPosition(Hold);
        LocalHand=FMath::Lerp(RestHand,LocalHand,PrepareT);
        LocalHand=FMath::Lerp(LocalHand,RestHand,LedgeT);
        const FVector Axis=LocalHand-Shoulder;
        UStaticMeshComponent* Arm=Side==0?LeftArm.Get():RightArm.Get();
        Arm->SetRelativeLocation((Shoulder+LocalHand)*.5f);
        Arm->SetRelativeRotation(FQuat::FindBetweenNormals(FVector::UpVector,Axis.GetSafeNormal()));
        Arm->SetRelativeScale3D(FVector(1.9,1.9,Axis.Size()/100.f));
    }
}
bool AMinedakiBoss::TryPurifyFirstKakon()
{
    if(GetNushiState()!=ENushiState::Active || ActionState!=EMinedakiActionState::UpperPlatform || !Player
        || !Player->IsMounted() || Player->IsRouteMoving() || Player->GetRouteNode()!=KakonNode
        || FVector::Dist(Player->GetActorLocation(),GetRouteWorld(KakonNode))>100.f || !GetKakon(0)->Purify()) return false;
    Telemetry.KakonPurified=1; Telemetry.CompletionSeconds=Telemetry.Elapsed;
    SetPrimitiveColor(Cast<UMaterialInstanceDynamic>(CoreMarker->GetMaterial(0)),FLinearColor(.3,1,.85));
    LogTelemetry(TEXT("SliceComplete")); return true;
}
void AMinedakiBoss::LogTelemetry(const TCHAR* Event) const
{
    UE_LOG(LogTemp,Display,TEXT("MINEDAKI_TELEMETRY %s grab_attempts=%d grab_success=%d nodes=%d wall_starts=%d cling_seconds=%.3f shakes=%d falls=%d exhaustion=%d upper=%d kakon1=%d elapsed=%.3f completion=%.3f"),
        Event,Telemetry.GrabAttempts,Telemetry.GrabSuccesses,Telemetry.NodesReached,Telemetry.WallStarts,Telemetry.ClingSeconds,
        Telemetry.Shakes,Telemetry.Falls,Telemetry.Exhaustions,Telemetry.UpperReached,Telemetry.KakonPurified,Telemetry.Elapsed,Telemetry.CompletionSeconds);
}
