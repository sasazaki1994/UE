#pragma once

#include "CoreMinimal.h"

class USkeletalMesh;
class USkeletalMeshComponent;

namespace ProductionVisuals
{
    // Startup selection only: no mid-action pose reset, gameplay state or timing writes.
    bool IsCompatible(const USkeletalMesh* Baseline, const USkeletalMesh* Candidate, FString& Reason);
    bool ApplyCandidate(USkeletalMeshComponent* Component, USkeletalMesh* Candidate, FString& Reason);
    void ApplyAtBeginPlay(USkeletalMeshComponent* Component, const TCHAR* Character);
}
