#pragma once

#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"

// Change clips only on state transitions; replaying every tick freezes the pose.
inline void PlayModelClip(USkeletalMeshComponent* Mesh, UAnimSequence* Clip, bool bLoop, float Rate = 1.f)
{
    if (!Mesh || !Mesh->GetSkeletalMeshAsset() || !Clip) return;
    UAnimSingleNodeInstance* Instance = Mesh->GetSingleNodeInstance();
    if (!Instance || Instance->GetCurrentAsset() != Clip)
        Mesh->PlayAnimation(Clip, bLoop);
    Mesh->SetPlayRate(Rate);
}
