#include "MinedakiBoss.h"
#include "MinedakiPlayer.h"
#include "PlayerSenseComponent.h"
#include "DebugGuidance.h"
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
        auto* Mesh=CreateDefaultSubobject<UStaticMeshComponent>(Name); Mesh->SetupAttachment(BodyRoot);
        Mesh->SetStaticMesh(Sphere.Object); Mesh->SetMaterial(0,Material.Object); Mesh->SetRelativeLocation(Position); Mesh->SetRelativeScale3D(Scale);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mesh->SetCanEverAffectNavigation(false); return Mesh;
    };
    Part(TEXT("Pelvis"),{0,0,720},{4,5,4}); Part(TEXT("Torso"),{0,0,1100},{5,6.5,8});
    Part(TEXT("Head"),{-40,0,1640},{3.5,3.8,3.8}); Part(TEXT("Muzzle"),{-205,0,1580},{1.5,2.7,1.7});
    LeftArm=Part(TEXT("LeftArm"),{-70,-390,1020},{2.2,2.2,9}); RightArm=Part(TEXT("RightArm"),{-70,390,1020},{2.2,2.2,9});
    Part(TEXT("LeftLeg"),{0,-205,350},{2.5,2.5,7}); Part(TEXT("RightLeg"),{0,205,350},{2.5,2.5,7});
    for(int32 I=0;I<RouteNodeCount;++I) RouteMarkers.Add(Part(*FString::Printf(TEXT("Route%d"),I),GetRouteLocal(I),{.45,.45,.45}));
    for(int32 I=0;I<3;++I) CoreMarkers.Add(Part(*FString::Printf(TEXT("KakonMarker%d"),I),GetRouteLocal(I==0?Kakon1Node:I==1?Kakon2Node:Kakon3Node)+FVector(-55,0,0),{1.2,1.2,1.2}));
}
FVector AMinedakiBoss::GetRouteLocal(int32 Node) const
{
    // 0-7 retain the slice route. 8-10 cross the lowered left arm; 11-13 climb the opposite shoulder to the crown.
    static const FVector Nodes[]={ {185,-205,180},{205,-205,450},{255,-150,740},{310,-80,970},{325,0,1190},{280,-175,1380},{230,-90,1500},{200,0,1660},
        {120,-330,1460},{-40,-570,1510},{-210,-760,1570},{120,250,1490},{40,180,1690},{-30,40,1860} };
    return Nodes[FMath::Clamp(Node,0,RouteNodeCount-1)];
}
FVector AMinedakiBoss::GetRouteWorld(int32 Node) const { return BodyRoot->GetComponentTransform().TransformPosition(GetRouteLocal(Node)); }
void AMinedakiBoss::BeginPlay()
{
    Super::BeginPlay(); SpawnPose=GetActorTransform();
    GrabFrame=GetWorld()->SpawnActor<AActor>(); auto* FrameRoot=NewObject<USceneComponent>(GrabFrame,TEXT("BodyGrabFrame"));
    GrabFrame->SetRootComponent(FrameRoot); FrameRoot->RegisterComponent(); GrabFrame->AttachToComponent(BodyRoot,FAttachmentTransformRules::SnapToTargetNotIncludingScale); GrabFrame->SetOwner(this);
    for(int32 I=0;I<3;++I)
    {
        auto* Core=GetWorld()->SpawnActor<AKakonActor>(); Core->SetOwner(this); Core->AttachToComponent(BodyRoot,FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        Core->SetActorRelativeLocation(GetRouteLocal(I==0?Kakon1Node:I==1?Kakon2Node:Kakon3Node)); Core->SetActorEnableCollision(false);
        Core->OnPurified.AddUniqueDynamic(this,&AMinedakiBoss::HandleKakonPurified); Kakons.Add(Core); RegisterKakon(Core);
    }
    TArray<UStaticMeshComponent*> Parts; GetComponents(Parts); for(auto* Part:Parts) SetPrimitiveColor(Part->CreateDynamicMaterialInstance(0),FLinearColor(.28,.32,.38));
    for(UStaticMeshComponent* Marker:RouteMarkers) SetPrimitiveColor(Cast<UMaterialInstanceDynamic>(Marker->GetMaterial(0)),FLinearColor(.1,1,.65));
    for(UStaticMeshComponent* Marker:CoreMarkers) SetPrimitiveColor(Cast<UMaterialInstanceDynamic>(Marker->GetMaterial(0)),FLinearColor(1,.07,.12));
    ResetNushi();
}
void AMinedakiBoss::EndPlay(const EEndPlayReason::Type Reason)
{
    if(IsValid(GrabFrame)) GrabFrame->Destroy(); for(AKakonActor* Core:Kakons) if(IsValid(Core)) Core->Destroy(); Super::EndPlay(Reason);
}
void AMinedakiBoss::ResetNushi()
{
    Super::ResetNushi(); ActionState=EMinedakiActionState::Grounded; ClimbTime=TransitionTime=CalmTime=0; bShakeApplied=bTransitionShakeApplied=false; Telemetry=FMinedakiTelemetry();
    SetActorTransform(SpawnPose); BodyRoot->SetRelativeRotation(FRotator::ZeroRotator);
    LeftArm->SetRelativeLocation({-70,-390,1020}); RightArm->SetRelativeLocation({-70,390,1020});
    LeftArm->SetRelativeRotation(FRotator::ZeroRotator); RightArm->SetRelativeRotation(FRotator::ZeroRotator); LeftArm->SetRelativeScale3D({2.2,2.2,9}); RightArm->SetRelativeScale3D({2.2,2.2,9});
    for(UStaticMeshComponent* Marker:CoreMarkers) { Marker->SetVisibility(false); SetPrimitiveColor(Cast<UMaterialInstanceDynamic>(Marker->GetMaterial(0)),FLinearColor(1,.07,.12)); }
    for(AKakonActor* Core:Kakons) if(Core) Core->SetActorHiddenInGame(true);
    SetRouteVisibility();
}
AKakonActor* AMinedakiBoss::GetKakon(int32 I) const { return Kakons.IsValidIndex(I)?Kakons[I].Get():nullptr; }
bool AMinedakiBoss::IsEncounterComplete() const { return GetNushiState()==ENushiState::Calm; }
bool AMinedakiBoss::IsSliceComplete() const { return GetNushiProgressComponent()->GetPurifiedCount()>=1; }
bool AMinedakiBoss::IsWallMoving() const { return ActionState==EMinedakiActionState::PreparingClimb||ActionState==EMinedakiActionState::ClimbingWall||ActionState==EMinedakiActionState::Shaking||ActionState==EMinedakiActionState::LedgeTransition; }
bool AMinedakiBoss::IsBodyTransitioning() const { return ActionState==EMinedakiActionState::ArmBridgeTransition||ActionState==EMinedakiActionState::FinalTransition; }
int32 AMinedakiBoss::GetMaximumRouteNode() const
{
    const int32 Count=GetNushiProgressComponent()->GetPurifiedCount();
    if(ActionState==EMinedakiActionState::Grounded||IsWallMoving()) return WallStartNode;
    if(Count==0||ActionState==EMinedakiActionState::ArmBridgeTransition) return 7;
    if(Count==1||ActionState==EMinedakiActionState::FinalTransition) return Kakon2Node;
    return Kakon3Node;
}
bool AMinedakiBoss::IsRouteNodeEnabled(int32 Node) const { return Node>=0 && Node<=GetMaximumRouteNode(); }
int32 AMinedakiBoss::GetRecoveryNode() const { const int32 C=GetNushiProgressComponent()->GetPurifiedCount(); return C>=2?11:C>=1?7:0; }
FVector AMinedakiBoss::GetRecoveryAnchorWorld() const
{
    const int32 C=GetNushiProgressComponent()->GetPurifiedCount();
    if(C==0) return AMinedakiPlayer::SpawnTransform().GetLocation();
    // Offset on the safe side of the authored rest node: recovery still requires
    // walking 180 cm and pressing the normal Grab action before attachment.
    return GetRouteWorld(GetRecoveryNode())+BodyRoot->GetRightVector()*(C>=2?180.f:-180.f);
}
FString AMinedakiBoss::GetActionLabel() const { return StaticEnum<EMinedakiActionState>()->GetNameStringByValue(static_cast<int64>(ActionState)); }
void AMinedakiBoss::NotifyRouteNode(int32 Node)
{
    ++Telemetry.NodesReached;
    if(Node==WallStartNode && ActionState==EMinedakiActionState::Grounded && GetNushiState()==ENushiState::Active)
    { ActionState=EMinedakiActionState::PreparingClimb; ++Telemetry.WallStarts; ++Telemetry.Phase1Starts; LogTelemetry(TEXT("Phase1Started")); }
}
void AMinedakiBoss::Tick(float Dt) { Super::Tick(Dt); if(GetNushiState()==ENushiState::Active) Telemetry.Elapsed+=Dt; AdvanceWallClimb(Dt); AdvancePostKakon(Dt); }
void AMinedakiBoss::AdvanceWallClimb(float Dt)
{
    if(!FMath::IsFinite(Dt)||Dt<=0||PrepareSeconds<=0||WallSeconds<=0||LedgeSeconds<=0||GetNushiState()!=ENushiState::Active||!IsWallMoving()) return;
    const float Total=PrepareSeconds+WallSeconds+LedgeSeconds; ClimbTime=FMath::Min(ClimbTime+Dt,Total); FVector Position; FRotator Rotation; const auto Smooth=[](float T){return T*T*(3-2*T);};
    if(ClimbTime<PrepareSeconds) { float T=Smooth(ClimbTime/PrepareSeconds); Position=FMath::Lerp(FVector::ZeroVector,FVector(-350,0,250),T); Rotation={35*T,8*T,0}; ActionState=EMinedakiActionState::PreparingClimb; }
    else if(ClimbTime<PrepareSeconds+WallSeconds)
    {
        float T=(ClimbTime-PrepareSeconds)/WallSeconds, Lift=Smooth(FMath::Fmod(T*2,1.f)); float Height=T<.5f?FMath::Lerp(250.f,1150.f,Lift):FMath::Lerp(1150.f,PlateauHeight+150.f,Lift);
        Position={FMath::Lerp(-350.f,-650.f,Smooth(T)),80*FMath::Sin(T*PI),Height}; Rotation={FMath::Lerp(35.f,MaximumPitch,FMath::Min(T*3,1.f)),FMath::Lerp(8.f,MaximumYaw,Smooth(T)),0}; ActionState=EMinedakiActionState::ClimbingWall;
        if(T>=.4f&&T<.65f) { ActionState=EMinedakiActionState::Shaking; Rotation.Roll=ShakeRoll*FMath::Sin((T-.4f)/.25f*PI); }
    }
    else { float T=Smooth((ClimbTime-PrepareSeconds-WallSeconds)/LedgeSeconds); Position=FMath::Lerp(FVector(-650,0,PlateauHeight+150),FVector(-2950,0,PlateauHeight),T); Rotation={MaximumPitch*(1-T),MaximumYaw*(1-T),0}; ActionState=EMinedakiActionState::LedgeTransition; }
    SetActorLocation(SpawnPose.TransformPosition(Position)); BodyRoot->SetRelativeRotation(Rotation); UpdateArmHolds();
    if(!bShakeApplied&&ClimbTime>=PrepareSeconds+WallSeconds*.5f) { bShakeApplied=true; ++Telemetry.Shakes; if(Player) Player->ResolveShake(); LogTelemetry(TEXT("Phase1Shake")); }
    if(ClimbTime>=Total)
    {
        ActionState=EMinedakiActionState::UpperPlatform; ++Telemetry.UpperReached; ++Telemetry.Phase1Completes;
        LeftArm->SetRelativeRotation(FRotator::ZeroRotator); RightArm->SetRelativeRotation(FRotator::ZeroRotator); LeftArm->SetRelativeLocation({-70,-390,1020}); RightArm->SetRelativeLocation({-70,390,1020}); LeftArm->SetRelativeScale3D({2.2,2.2,9}); RightArm->SetRelativeScale3D({2.2,2.2,9});
        GetKakon(0)->SetActorHiddenInGame(false); GetKakon(0)->ApplyShellDamage(GetKakon(0)->MaxShellHealth); CoreMarkers[0]->SetVisibility(true); SetRouteVisibility(); LogTelemetry(TEXT("Phase1Completed"));
    }
}
void AMinedakiBoss::AdvancePostKakon(float Dt)
{
    if(!FMath::IsFinite(Dt)||Dt<=0) return;
    if(IsBodyTransitioning())
    {
        if(RouteTransitionSeconds<=0) { FinishTransition(); return; }
        TransitionTime=FMath::Min(TransitionTime+Dt,RouteTransitionSeconds); const float T=FMath::SmoothStep(0.f,1.f,TransitionTime/RouteTransitionSeconds);
        if(ActionState==EMinedakiActionState::ArmBridgeTransition) { BodyRoot->SetRelativeRotation({-12*T,18*T,-20*T}); LeftArm->SetRelativeLocation(FMath::Lerp(FVector(-70,-390,1020),FVector(-30,-610,1450),T)); LeftArm->SetRelativeRotation({0,0,70*T}); LeftArm->SetRelativeScale3D({2.2,2.2,FMath::Lerp(9.f,12.f,T)}); }
        else { BodyRoot->SetRelativeRotation({FMath::Lerp(-12.f,28.f,T),FMath::Lerp(18.f,-16.f,T),FMath::Lerp(-20.f,30.f,T)}); RightArm->SetRelativeLocation(FMath::Lerp(FVector(-70,390,1020),FVector(-100,520,1510),T)); RightArm->SetRelativeRotation({0,0,-65*T}); }
        if(!bTransitionShakeApplied&&T>=.55f) { bTransitionShakeApplied=true; ++Telemetry.Shakes; if(Player) Player->ResolveShake(); LogTelemetry(ActionState==EMinedakiActionState::ArmBridgeTransition?TEXT("Phase2TiltShake"):TEXT("Phase3HeadRiseShake")); }
        if(TransitionTime>=RouteTransitionSeconds) FinishTransition();
    }
    if(ActionState==EMinedakiActionState::Calming)
    {
        if(CalmSeconds<=0) { BodyRoot->SetRelativeRotation(FRotator(8,0,0)); ActionState=EMinedakiActionState::Calm; LogTelemetry(TEXT("Calm")); return; }
        CalmTime=FMath::Min(CalmTime+Dt,CalmSeconds); const float T=FMath::SmoothStep(0.f,1.f,CalmTime/CalmSeconds); BodyRoot->SetRelativeRotation(FMath::Lerp(FRotator(28,-16,30),FRotator(8,0,0),T));
        if(CalmTime>=CalmSeconds) { ActionState=EMinedakiActionState::Calm; LogTelemetry(TEXT("Calm")); }
    }
}
void AMinedakiBoss::BeginPhase(int32 Phase)
{
    TransitionTime=0; bTransitionShakeApplied=false; ++Telemetry.BodyRouteTransitions;
    if(Phase==2) { ActionState=EMinedakiActionState::ArmBridgeTransition; ++Telemetry.Phase2Starts; LogTelemetry(TEXT("Phase2Started_BodyRouteTransition")); }
    else { ActionState=EMinedakiActionState::FinalTransition; ++Telemetry.Phase3Starts; LogTelemetry(TEXT("Phase3Started_BodyRouteTransition")); }
    SetRouteVisibility();
}
void AMinedakiBoss::FinishTransition()
{
    const bool bArmTransition=ActionState==EMinedakiActionState::ArmBridgeTransition;
    const bool bFinalTransition=ActionState==EMinedakiActionState::FinalTransition;
    if(!bArmTransition && !bFinalTransition) return;
    const int32 KakonIndex=bArmTransition?1:2;
    AKakonActor* Kakon=GetKakon(KakonIndex);
    if(!IsValid(Kakon) || !CoreMarkers.IsValidIndex(KakonIndex) || !IsValid(CoreMarkers[KakonIndex].Get())) return;
    ActionState=bArmTransition?EMinedakiActionState::ArmBridge:EMinedakiActionState::FinalRoute;
    Kakon->SetActorHiddenInGame(false); Kakon->ApplyShellDamage(Kakon->MaxShellHealth); CoreMarkers[KakonIndex]->SetVisibility(true);
    if(bArmTransition) { ++Telemetry.Phase2Completes; LogTelemetry(TEXT("Phase2Completed_ArmRouteOpen")); }
    else { ++Telemetry.Phase3Completes; LogTelemetry(TEXT("Phase3Completed_FinalRouteOpen")); }
    SetRouteVisibility();
}
void AMinedakiBoss::HandleKakonPurified(AKakonActor* Kakon)
{
    const int32 Index=Kakons.IndexOfByKey(Kakon); if(Index==INDEX_NONE) return; Telemetry.KakonPurified=Index+1;
    SetPrimitiveColor(Cast<UMaterialInstanceDynamic>(CoreMarkers[Index]->GetMaterial(0)),FLinearColor(.3,1,.85));
    if(Index==0) BeginPhase(2); else if(Index==1) BeginPhase(3); else { Telemetry.CompletionSeconds=Telemetry.Elapsed; ActionState=EMinedakiActionState::Calming; CalmTime=0; LogTelemetry(TEXT("Kakon3_EncounterCompleted_Victory")); }
    LogTelemetry(*FString::Printf(TEXT("Kakon%d"),Index+1));
}
bool AMinedakiBoss::TryPurifyKakon()
{
    if(GetNushiState()!=ENushiState::Active||!Player||!Player->IsMounted()||Player->IsRouteMoving()) return false;
    const int32 Index=GetNushiProgressComponent()->GetPurifiedCount(); const int32 Required[]={Kakon1Node,Kakon2Node,Kakon3Node};
    return Index<3&&Player->GetRouteNode()==Required[Index]&&FVector::Dist(Player->GetActorLocation(),GetRouteWorld(Required[Index]))<=110&&GetKakon(Index)->Purify();
}
void AMinedakiBoss::SetRouteVisibility()
{
    const bool Reveal=IsDebugGuidanceEnabled()||(Player&&Player->GetSense()->IsBoundarySenseActive());
    const int32 Max=GetMaximumRouteNode(); for(int32 I=0;I<RouteMarkers.Num();++I) RouteMarkers[I]->SetVisibility(Reveal&&I<=Max&&!(IsBodyTransitioning()&&I>Max));
    const int32 Active=GetNushiProgressComponent()->GetPurifiedCount(); for(int32 I=0;I<CoreMarkers.Num();++I) CoreMarkers[I]->SetVisibility(Reveal&&I==Active&&GetKakon(I)&&!GetKakon(I)->IsHidden());
}
void AMinedakiBoss::UpdateArmHolds()
{
    const float WallT=FMath::Clamp((ClimbTime-PrepareSeconds)/WallSeconds,0.f,1.f), PrepareT=FMath::Clamp(ClimbTime/PrepareSeconds,0.f,1.f), LedgeT=FMath::Clamp((ClimbTime-PrepareSeconds-WallSeconds)/LedgeSeconds,0.f,1.f);
    for(int32 Side=0;Side<2;++Side) { const float Sign=Side==0?-1.f:1.f; const FVector Shoulder(-30,Sign*325,1350),RestHand(-70,Sign*390,570); const float Reach=Side==0?FMath::Clamp(WallT/.35f,0.f,1.f):FMath::Clamp((WallT-.4f)/.25f,0.f,1.f); const FVector Hold=SpawnPose.TransformPosition(FVector(-2040,Sign*480,FMath::Lerp(Side==0?600.f:1020.f,Side==0?1440.f:1860.f,Reach))); FVector LocalHand=BodyRoot->GetComponentTransform().InverseTransformPosition(Hold); LocalHand=FMath::Lerp(RestHand,LocalHand,PrepareT); LocalHand=FMath::Lerp(LocalHand,RestHand,LedgeT); const FVector Axis=LocalHand-Shoulder; auto* Arm=Side==0?LeftArm.Get():RightArm.Get(); Arm->SetRelativeLocation((Shoulder+LocalHand)*.5f); Arm->SetRelativeRotation(FQuat::FindBetweenNormals(FVector::UpVector,Axis.GetSafeNormal())); Arm->SetRelativeScale3D({1.9,1.9,Axis.Size()/100.f}); }
}
void AMinedakiBoss::LogTelemetry(const TCHAR* Event) const
{
    UE_LOG(LogTemp,Display,TEXT("MINEDAKI_TELEMETRY %s grab=%d/%d phase=%d/%d/%d kakon=%d route_transitions=%d cling=%.2f shakes=%d success=%d failure=%d falls=%d recovery=%d/%d exhaustion=%d elapsed=%.2f clear=%.2f"),Event,Telemetry.GrabSuccesses,Telemetry.GrabAttempts,Telemetry.Phase1Completes,Telemetry.Phase2Completes,Telemetry.Phase3Completes,Telemetry.KakonPurified,Telemetry.BodyRouteTransitions,Telemetry.ClingSeconds,Telemetry.Shakes,Telemetry.ShakeSuccesses,Telemetry.ShakeFailures,Telemetry.Falls,Telemetry.RecoverySuccesses,Telemetry.RecoveryStarts,Telemetry.Exhaustions,Telemetry.Elapsed,Telemetry.CompletionSeconds);
}
