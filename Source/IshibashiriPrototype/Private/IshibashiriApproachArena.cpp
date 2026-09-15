#include "IshibashiriApproachArena.h"
#include "PrimitiveAppearance.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AIshibashiriApproachArena::AIshibashiriApproachArena()
{
    PrimaryActorTick.bCanEverTick=true;
    Root=CreateDefaultSubobject<USceneComponent>(TEXT("Root")); SetRootComponent(Root);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    CubeMesh=Cube.Object; SphereMesh=Sphere.Object; BaseMaterial=Material.Object;
    // 1.31 km of bends; at the 6 m/s walking speed this is about three minutes without stopping.
    PathPoints={{0,0,0},{11000,0,0},{20500,6500,100},{30500,8500,180},{39500,1000,260},
        {49000,-6500,300},{58500,-7200,380},{67500,-500,450},{76000,7200,500},{85000,7600,420},
        {94500,500,300},{103500,-6000,180},{112000,0,0}};
    for(int32 I=1;I<PathPoints.Num();++I) PathLengthMeters+=FVector::Distance(PathPoints[I-1],PathPoints[I])/100.f;
}

void AIshibashiriApproachArena::ClearGenerated(){for(UActorComponent* C:Generated)if(IsValid(C))C->DestroyComponent();Generated.Reset();RevealParts.Reset();}

UStaticMeshComponent* AIshibashiriApproachArena::AddPrimitive(const TCHAR* Name,UStaticMesh* MeshAsset,const FVector& Location,
    const FVector& Scale,const FRotator& Rotation,const FLinearColor& Color,bool bCollision)
{
    auto* C=NewObject<UStaticMeshComponent>(this,Name);C->SetupAttachment(Root);C->SetStaticMesh(MeshAsset);
    C->SetRelativeLocation(Location);C->SetRelativeScale3D(Scale);C->SetRelativeRotation(Rotation);
    C->SetCollisionEnabled(bCollision?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
    if(bCollision)C->SetCollisionProfileName(TEXT("BlockAll"));C->SetMaterial(0,BaseMaterial);C->RegisterComponent();
    if(auto* M=C->CreateDynamicMaterialInstance(0))SetPrimitiveColor(M,Color);Generated.Add(C);return C;
}

void AIshibashiriApproachArena::AddTree(const TCHAR* Name,const FVector& L,float H,bool bFallen)
{
    const FRotator R=bFallen?FRotator(0,25,88):FRotator(0,0,0);
    AddPrimitive(*FString::Printf(TEXT("%sTrunk"),Name),CubeMesh,L+FVector(0,0,bFallen?70:H*.5f),FVector(.55f,.55f,H/100.f),R,FLinearColor(.075f,.052f,.035f),true);
    if(!bFallen)AddPrimitive(*FString::Printf(TEXT("%sCrown"),Name),SphereMesh,L+FVector(0,0,H*.82f),FVector(3.8f,3.4f,4.8f),FRotator::ZeroRotator,FLinearColor(.07f,.11f,.055f));
}

void AIshibashiriApproachArena::OnConstruction(const FTransform& T)
{
    Super::OnConstruction(T);ClearGenerated();
    const FLinearColor Soil(.20f,.15f,.10f),Rock(.12f,.13f,.13f),Moss(.12f,.18f,.09f),Bone(.58f,.52f,.40f),Vermilion(.35f,.055f,.025f),Corruption(.008f,.006f,.009f);
    for(int32 I=0;I<PathPoints.Num()-1;++I){const FVector A=PathPoints[I],B=PathPoints[I+1],D=B-A;AddPrimitive(*FString::Printf(TEXT("WetEarth%02d"),I),CubeMesh,(A+B)*.5f+FVector(0,0,-42),FVector(D.Size()/100.f,18.f,1.f),FRotator(0,D.Rotation().Yaw,0),Soil,true);
        const FVector Side=FVector::CrossProduct(D.GetSafeNormal(),FVector::UpVector);
        for(int32 S:{-1,1}){const FVector P=(A+B)*.5f+Side*(S*(1250+(I%3)*260));AddPrimitive(*FString::Printf(TEXT("Cliff%02d_%d"),I,S),SphereMesh,P+FVector(0,0,420),FVector(9+(I%2)*3,6,7+(I%3)),FRotator(I*3,I*19,S*7),I%3?Rock:Moss,true);AddTree(*FString::Printf(TEXT("OldTree%02d_%d"),I,S),P+Side*(S*650),760+(I%4)*90);}}
    // Broken warning stone, weathered rope/pillars, restrained black cracks, and the damage trail.
    AddPrimitive(TEXT("BoundaryStone"),CubeMesh,PathPoints[3]+FVector(-300,700,150),FVector(1.0f,.45f,3.1f),FRotator(0,-18,13),Bone,true);
    for(int32 S:{-1,1})AddPrimitive(*FString::Printf(TEXT("RitualPost%d"),S),CubeMesh,PathPoints[3]+FVector(700,S*900,240),FVector(.22f,.22f,4.8f),FRotator(0,0,S*5),Vermilion,true);
    AddPrimitive(TEXT("DecayedShimenawa"),CubeMesh,PathPoints[3]+FVector(700,0,430),FVector(.10f,9.f,.10f),FRotator(0,0,7),Bone);
    for(int32 I=0;I<4;++I)AddPrimitive(*FString::Printf(TEXT("GiantFootprint%02d"),I),SphereMesh,PathPoints[6]+FVector(I*950,(I&1)?430:-430,-28),FVector(5.2f,2.5f,.22f),FRotator(0,20,0),Corruption);
    AddPrimitive(TEXT("GougedRock"),SphereMesh,PathPoints[6]+FVector(3700,1500,580),FVector(7,5,8),FRotator(0,0,-24),Rock,true);
    AddTree(TEXT("ShatteredCedar"),PathPoints[6]+FVector(2500,-1300,0),1050,true);
    for(int32 I=0;I<5;++I)AddPrimitive(*FString::Printf(TEXT("CorruptionCrack%02d"),I),CubeMesh,PathPoints[8]+FVector(I*350-700,I%2*240-120,4),FVector(2.5f,.055f,.04f),FRotator(0,20+I*14,0),Corruption);
    // A deliberately inert full-body silhouette proxy: no AI, damage, Grab, Kakon, or collision.
    RevealParts.Add(AddPrimitive(TEXT("DistantIshibashiriBody"),SphereMesh,RevealOrigin,FVector(15,6,8),FRotator(0,90,0),Rock));
    RevealParts.Add(AddPrimitive(TEXT("DistantIshibashiriBack"),SphereMesh,RevealOrigin+FVector(0,0,700),FVector(11,5,5),FRotator(0,90,0),Moss));
    for(UPrimitiveComponent* C:RevealParts)C->SetVisibility(false);
    auto* Fog=NewObject<UExponentialHeightFogComponent>(this,TEXT("ApproachMist"));Fog->SetupAttachment(Root);Fog->SetRelativeLocation(FVector(55000,0,-100));Fog->SetFogDensity(.009f);Fog->SetFogMaxOpacity(.16f);Fog->SetStartDistance(1500);Fog->RegisterComponent();Generated.Add(Fog);
}

void AIshibashiriApproachArena::SetStage(int32 NewStage)
{
    if(NewStage<=Stage)return;Stage=NewStage;
    if(Stage==1){Subtitle=TEXT("ここから先へ入るな");SubtitleRemaining=4.f;UE_LOG(LogTemp,Display,TEXT("APPROACH_STAGE1 distant_rumble boundary_stone NOT_IMPLEMENTED — AUDIO ASSET REQUIRED"));}
    else if(Stage==2)UE_LOG(LogTemp,Display,TEXT("APPROACH_STAGE2 giant_footprints damage_trail"));
    else if(Stage==3){bRevealVisible=true;RevealElapsed=0;RevealCount++;for(UPrimitiveComponent* C:RevealParts)C->SetVisibility(true);UE_LOG(LogTemp,Display,TEXT("APPROACH_REVEAL count=%d duration=3.0 combat=false NOT_IMPLEMENTED — AUDIO ASSET REQUIRED"),RevealCount);}
    else if(Stage==4)UE_LOG(LogTemp,Display,TEXT("APPROACH_STAGE4 close_rumble camera_reaction_hook NOT_IMPLEMENTED — AUDIO ASSET REQUIRED"));
}

void AIshibashiriApproachArena::ObservePlayer(const FVector& P)
{
    if(Stage<1&&FVector::Dist2D(P,PathPoints[3])<1800)SetStage(1);
    if(Stage<2&&FVector::Dist2D(P,PathPoints[6])<2200)SetStage(2);
    if(Stage<3&&FVector::Dist2D(P,PathPoints[8])<2200)SetStage(3);
    if(Stage<4&&FVector::Dist2D(P,PathPoints[10])<2200)SetStage(4);
    if(Stage>=4&&FVector::Dist2D(P,PathPoints.Last())<900){bGateReached=true;UE_LOG(LogTemp,Display,TEXT("APPROACH_BASIN_GATE reached"));}
}

void AIshibashiriApproachArena::Tick(float Dt)
{
    Super::Tick(Dt);if(SubtitleRemaining>0&&((SubtitleRemaining-=Dt)<=0))Subtitle.Empty();
    if(!bRevealVisible)return;RevealElapsed+=Dt;for(UPrimitiveComponent* C:RevealParts)C->AddWorldOffset(FVector(0,Dt*650,0));
    if(RevealElapsed>=3.f){bRevealVisible=false;for(UPrimitiveComponent* C:RevealParts)C->SetVisibility(false);UE_LOG(LogTemp,Display,TEXT("APPROACH_REVEAL_END combat=false"));}
}
