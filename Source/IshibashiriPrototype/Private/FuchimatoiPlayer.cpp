#include "FuchimatoiPlayer.h"
#include "FuchimatoiBoss.h"
#include "FuchimatoiGameMode.h"
#include "FuchimatoiRouteAnchor.h"
#include "FuchimatoiSimulationComponent.h"
#include "GrabComponent.h"
#include "StaminaComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "PrimitiveAppearance.h"
#include "UObject/ConstructorHelpers.h"

AFuchimatoiPlayer::AFuchimatoiPlayer()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickGroup=TG_PostPhysics;
    GetCapsuleComponent()->InitCapsuleSize(34,88);
    bUseControllerRotationYaw=false;
    GetCharacterMovement()->bOrientRotationToMovement=true;
    GetCharacterMovement()->RotationRate=FRotator(0,720,0);
    GetCharacterMovement()->MaxWalkSpeed=480;
    GetCharacterMovement()->JumpZVelocity=520;
    Grab=CreateDefaultSubobject<UGrabComponent>(TEXT("SharedGrab"));
    Stamina=CreateDefaultSubobject<UStaminaComponent>(TEXT("SharedStamina"));
    Arm=CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
    Arm->SetupAttachment(RootComponent); Arm->TargetArmLength=1050;
    Arm->TargetOffset=FVector(0,0,110); Arm->bUsePawnControlRotation=true;
    Arm->bDoCollisionTest=true; Arm->ProbeSize=18;
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Arm); Camera->FieldOfView=80;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    UStaticMeshComponent* Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrimitiveClimber"));
    Body->SetupAttachment(RootComponent); Body->SetStaticMesh(Sphere.Object);
    Body->SetMaterial(0,Material.Object);
    Body->SetRelativeScale3D(FVector(.65,.65,1.35)); Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    UStaticMeshComponent* Head=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClimberHead"));
    Head->SetupAttachment(RootComponent); Head->SetStaticMesh(Sphere.Object);
    Head->SetMaterial(0,Material.Object);
    Head->SetRelativeLocation(FVector(0,0,65)); Head->SetRelativeScale3D(FVector(.45));
    Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void AFuchimatoiPlayer::ConfigureBoss(AFuchimatoiBoss* InBoss)
{
    Boss=InBoss;
    AddTickPrerequisiteComponent(Boss->FindComponentByClass<UFuchimatoiSimulationComponent>());
    Grab->AddTickPrerequisiteActor(this);
    TArray<UStaticMeshComponent*> Parts; GetComponents(Parts);
    for (UStaticMeshComponent* Part:Parts) SetPrimitiveColor(Part->CreateDynamicMaterialInstance(0),FLinearColor(.93,.76,.34));
}
bool AFuchimatoiPlayer::CanAct() const
{
    const AFuchimatoiGameMode* Mode=GetWorld()->GetAuthGameMode<AFuchimatoiGameMode>();
    return Mode && Mode->IsEncounterActive();
}
bool AFuchimatoiPlayer::IsMounted() const { return Grab->IsGrabbing(); }
void AFuchimatoiPlayer::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("MoveForward"),this,&AFuchimatoiPlayer::MoveForward);
    Input->BindAxis(TEXT("MoveRight"),this,&AFuchimatoiPlayer::MoveRight);
    Input->BindAxis(TEXT("Turn"),this,&AFuchimatoiPlayer::Turn);
    Input->BindAxis(TEXT("LookUp"),this,&AFuchimatoiPlayer::Look);
    Input->BindAxis(TEXT("TurnRate"),this,&AFuchimatoiPlayer::TurnRate);
    Input->BindAxis(TEXT("LookUpRate"),this,&AFuchimatoiPlayer::LookRate);
    Input->BindAction(TEXT("Grab"),IE_Pressed,this,&AFuchimatoiPlayer::GrabPressed);
    Input->BindAction(TEXT("Jump"),IE_Pressed,this,&AFuchimatoiPlayer::JumpPressed);
    Input->BindAction(TEXT("Dodge"),IE_Pressed,this,&AFuchimatoiPlayer::DodgePressed);
    Input->BindAction(TEXT("Attack"),IE_Pressed,this,&AFuchimatoiPlayer::AttackPressed);
    Input->BindAction(TEXT("Retry"),IE_Pressed,this,&AFuchimatoiPlayer::RetryPressed);
}
void AFuchimatoiPlayer::MoveForward(float Value)
{
    ForwardInput=Value;
    if (CanAct() && !IsMounted() && !IsDodging()) AddMovementInput(FRotator(0,GetControlRotation().Yaw,0).Vector(),Value);
}
void AFuchimatoiPlayer::MoveRight(float Value)
{
    RightInput=Value;
    if (CanAct() && !IsMounted() && !IsDodging()) AddMovementInput(FRotationMatrix(FRotator(0,GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::Y),Value);
}
void AFuchimatoiPlayer::Turn(float Value) { if(CanAct()) AddControllerYawInput(Value); }
void AFuchimatoiPlayer::Look(float Value) { if(CanAct()) AddControllerPitchInput(Value); }
void AFuchimatoiPlayer::TurnRate(float Value) { Turn(Value*55*GetWorld()->GetDeltaSeconds()); }
void AFuchimatoiPlayer::LookRate(float Value) { Look(Value*45*GetWorld()->GetDeltaSeconds()); }
void AFuchimatoiPlayer::GrabPressed()
{
    if (!CanAct() || IsMounted() || IsDodging() || !Boss || !Boss->CanMount() || Stamina->GetCurrentStamina()<25) return;
    AFuchimatoiRouteAnchor* Anchor=Boss->GetRouteAnchor(0);
    if (Grab->TryGrab(Anchor,GrabRange))
    {
        Node=0; Destination=INDEX_NONE; RouteProgress=0; RouteDelay=.2f;
        Grab->SetRelativeGrabTransform(FTransform::Identity);
        GetCharacterMovement()->bOrientRotationToMovement=false;
    }
}
void AFuchimatoiPlayer::JumpPressed()
{
    if (!CanAct()) return;
    if (IsMounted())
    {
        Grab->Release(); Node=Destination=INDEX_NONE;
        GetCharacterMovement()->bOrientRotationToMovement=true;
        LaunchCharacter(FVector(0,-180,260),true,true);
    }
    else Jump();
}
void AFuchimatoiPlayer::DodgePressed()
{
    if (!CanAct() || IsMounted() || IsDodging() || DodgeCooldown>0 || GetCharacterMovement()->IsFalling()) return;
    const FRotationMatrix Rotation(FRotator(0,GetControlRotation().Yaw,0));
    DodgeDirection=(Rotation.GetUnitAxis(EAxis::X)*ForwardInput+Rotation.GetUnitAxis(EAxis::Y)*RightInput).GetSafeNormal();
    if (DodgeDirection.IsNearlyZero()) DodgeDirection=GetActorRightVector();
    DodgeRemaining=.38f; DodgeCooldown=.85f;
    GetCharacterMovement()->Velocity=DodgeDirection*1100.f;
}
void AFuchimatoiPlayer::AttackPressed()
{
    if (CanAct() && Boss) Boss->TryPurifyAtNode(Node);
}
void AFuchimatoiPlayer::RetryPressed()
{
    if (AFuchimatoiGameMode* Mode=GetWorld()->GetAuthGameMode<AFuchimatoiGameMode>()) Mode->RetryEncounter();
}
bool AFuchimatoiPlayer::ReceiveBite()
{
    if (!CanAct() || IsDodging() || HitImmunity>0) return false;
    Health=FMath::Max(0,Health-1); HitImmunity=1.f;
    if (Health==0) GetWorld()->GetAuthGameMode<AFuchimatoiGameMode>()->Defeat();
    else LaunchCharacter(FVector(0,-300,180),true,true);
    return true;
}
void AFuchimatoiPlayer::AdvanceRoute(float Dt)
{
    if (Boss->IsCoiling() && !Boss->IsCoilingComplete())
    {
        Stamina->ConsumeStamina(6.f*Dt);
        if (Stamina->IsDepleted()) { Grab->Release(); Node=Destination=INDEX_NONE; }
        return;
    }
    RouteDelay=FMath::Max(0.f,RouteDelay-Dt);
    AFuchimatoiRouteAnchor* Current=Boss->GetRouteAnchor(Node);
    if (!Current) { Grab->Release(); return; }
    if (Current->IsRock() && Destination==INDEX_NONE) Stamina->RestoreStamina(28.f*Dt);
    else Stamina->ConsumeStamina((IsRouteMoving()?7.f:3.f)*Dt);
    if (Stamina->IsDepleted())
    {
        Grab->Release(); Node=Destination=INDEX_NONE; return;
    }
    if (Destination==INDEX_NONE && RouteDelay<=0 && FMath::Abs(ForwardInput)>.4f)
    {
        const int32 Next=Node+(ForwardInput>0?1:-1);
        const int32 MaxNode=Boss->IsCoilingComplete()?9:3;
        if (Next>=0 && Next<=MaxNode) { Destination=Next; RouteProgress=0; }
    }
    if (Destination==INDEX_NONE) return;
    AFuchimatoiRouteAnchor* Next=Boss->GetRouteAnchor(Destination);
    const float Distance=FVector::Dist(Current->GetActorLocation(),Next->GetActorLocation());
    RouteProgress=FMath::Min(1.f,RouteProgress+RouteSpeed*Dt/FMath::Max(1.f,Distance));
    const FVector Position=FMath::Lerp(Current->GetActorLocation(),Next->GetActorLocation(),RouteProgress);
    const FTransform WorldPose((Next->GetActorLocation()-Current->GetActorLocation()).Rotation(),Position);
    Grab->SetRelativeGrabTransform(WorldPose.GetRelativeTransform(Current->GetActorTransform()));
    if (RouteProgress>=1)
    {
        // Reuse the same grab component for the adjacent rock or serpent anchor.
        Grab->Release();
        if (Grab->TryGrab(Next,Distance+100.f))
        {
            Grab->SetRelativeGrabTransform(FTransform::Identity); Node=Destination;
        }
        else Node=INDEX_NONE;
        Destination=INDEX_NONE; RouteDelay=.18f;
    }
}
void AFuchimatoiPlayer::Tick(float Dt)
{
    Super::Tick(Dt);
    if (!CanAct()) return;
    HitImmunity=FMath::Max(0.f,HitImmunity-Dt); DodgeCooldown=FMath::Max(0.f,DodgeCooldown-Dt);
    if (IsDodging())
    {
        GetCharacterMovement()->Velocity=DodgeDirection*1100.f;
        DodgeRemaining=FMath::Max(0.f,DodgeRemaining-Dt);
        if (!IsDodging()) GetCharacterMovement()->StopMovementImmediately();
    }
    if (IsMounted()) AdvanceRoute(Dt); else Stamina->RestoreStamina(18.f*Dt);
    Arm->TargetArmLength=FMath::FInterpTo(Arm->TargetArmLength,IsMounted()?1400.f:1050.f,Dt,3.f);
    if (GetActorLocation().Z < -400) GetWorld()->GetAuthGameMode<AFuchimatoiGameMode>()->Defeat();
}
void AFuchimatoiPlayer::StopEncounter()
{
    ForwardInput=RightInput=DodgeRemaining=0;
    GetCharacterMovement()->StopMovementImmediately();
    ConsumeMovementInputVector();
}
void AFuchimatoiPlayer::ResetForEncounter(const FTransform& Spawn)
{
    Grab->Release(); StopJumping(); StopEncounter();
    Destination=Node=INDEX_NONE; RouteProgress=RouteDelay=0;
    DodgeCooldown=HitImmunity=0; Health=3; Stamina->ResetStamina();
    GetCharacterMovement()->ClearAccumulatedForces();
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    GetCharacterMovement()->bOrientRotationToMovement=true;
    SetActorTransform(Spawn,false,nullptr,ETeleportType::TeleportPhysics);
    // Side view keeps the spring arm clear of the bait rock when mounting the head.
    if (Controller) Controller->SetControlRotation(FRotator(-22,110,0));
    Arm->TargetArmLength=1050;
}
