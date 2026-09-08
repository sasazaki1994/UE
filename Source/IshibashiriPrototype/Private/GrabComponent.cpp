#include "GrabComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UGrabComponent::UGrabComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

bool UGrabComponent::TryGrab(AActor* Candidate, float MaximumDistance)
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (IsGrabbing() || !Character || !IsValid(Candidate)
        || FVector::Dist(Character->GetActorLocation(), Candidate->GetActorLocation()) > MaximumDistance)
        return false;

    GrabTarget = Candidate;
    bGrabbing = true;
    GrabStartWorldLocation = Character->GetActorLocation();
    RelativeGrabTransform = Character->GetActorTransform().GetRelativeTransform(Candidate->GetActorTransform());
    AddTickPrerequisiteActor(Candidate);
    Character->StopJumping();
    Character->GetCharacterMovement()->ClearAccumulatedForces();
    Character->GetCharacterMovement()->StopMovementImmediately();
    Character->GetCharacterMovement()->DisableMovement();
    return true;
}

void UGrabComponent::Release()
{
    if (!bGrabbing) return;
    AActor* PreviousTarget = GrabTarget;
    bGrabbing = false;
    GrabTarget = nullptr;
    if (PreviousTarget) RemoveTickPrerequisiteActor(PreviousTarget);
    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        Character->GetCharacterMovement()->ClearAccumulatedForces();
        Character->GetCharacterMovement()->StopMovementImmediately();
        // Falling works both in mid-air and on the ground, where floor detection
        // returns the character to walking on the following movement update.
        Character->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
    }
}

void UGrabComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!IsValid(GrabTarget) || !Character)
    {
        if (bGrabbing) Release();
        return;
    }

    const FTransform DesiredWorld = RelativeGrabTransform * GrabTarget->GetActorTransform();
    Character->SetActorTransform(DesiredWorld, false, nullptr, ETeleportType::TeleportPhysics);
}
