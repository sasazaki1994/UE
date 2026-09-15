#include "ProductionVisuals.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/UObjectGlobals.h"

bool ProductionVisuals::IsCompatible(const USkeletalMesh* Baseline, const USkeletalMesh* Candidate, FString& Reason)
{
    if (!Baseline || !Candidate) { Reason = TEXT("source/candidate missing"); return false; }
    if (!Candidate->GetSkeleton() || Candidate->GetSkeleton() != Baseline->GetSkeleton())
    { Reason = TEXT("candidate must reuse baseline skeleton and animations"); return false; }
    const FReferenceSkeleton& Original = Baseline->GetRefSkeleton();
    const FReferenceSkeleton& Replacement = Candidate->GetRefSkeleton();
    if (Original.GetNum() != Replacement.GetNum()) { Reason = TEXT("bone count changed"); return false; }
    for (int32 I = 0; I < Original.GetNum(); ++I)
    {
        if (Original.GetBoneName(I) != Replacement.GetBoneName(I) ||
            Original.GetParentIndex(I) != Replacement.GetParentIndex(I) ||
            !Original.GetRefBonePose()[I].Equals(Replacement.GetRefBonePose()[I], .01f))
        { Reason = TEXT("bone hierarchy/rest pose changed"); return false; }
    }
    const auto& Materials = Candidate->GetMaterials();
    if (Materials.IsEmpty() || Materials.Num() > 16) { Reason = TEXT("material budget/slots invalid"); return false; }
    for (const auto& Slot : Materials)
        if (!Slot.MaterialInterface) { Reason = TEXT("missing material"); return false; }
    const FBoxSphereBounds OldBounds = Baseline->GetBounds(), NewBounds = Candidate->GetBounds();
    if (NewBounds.Origin.ContainsNaN() || NewBounds.BoxExtent.ContainsNaN())
    { Reason = TEXT("invalid bounds"); return false; }
    for (int32 Axis = 0; Axis < 3; ++Axis)
    {
        const double Ratio = NewBounds.BoxExtent[Axis] / FMath::Max(OldBounds.BoxExtent[Axis], 1.0);
        if (Ratio < .75 || Ratio > 1.25 ||
            FMath::Abs(NewBounds.Origin[Axis] - OldBounds.Origin[Axis]) > OldBounds.BoxExtent[Axis] * .25)
        { Reason = TEXT("bounds/origin outside gameplay envelope"); return false; }
    }
    Reason.Reset();
    return true;
}

bool ProductionVisuals::ApplyCandidate(USkeletalMeshComponent* Component, USkeletalMesh* Candidate, FString& Reason)
{
    if (!Component || !IsCompatible(Component->GetSkeletalMeshAsset(), Candidate, Reason)) return false;
    // Component transform/children, collision, animation library and all gameplay owners stay intact.
    Component->SetSkeletalMesh(Candidate);
    Component->EmptyOverrideMaterials();
    return true;
}

void ProductionVisuals::ApplyAtBeginPlay(USkeletalMeshComponent* Component, const TCHAR* Character)
{
    int32 Enabled = 0;
    FParse::Value(FCommandLine::Get(), TEXT("ProductionVisuals="), Enabled);
    const FString PerCharacter = FString(Character) + TEXT("ProductionVisuals=");
    FParse::Value(FCommandLine::Get(), *PerCharacter, Enabled);
    if (Enabled != 1) return;
    // Fixed allowlist prevents later bosses from accidentally joining this intake.
    if (FCString::Strcmp(Character, TEXT("Shirotsura")) != 0 &&
        FCString::Strcmp(Character, TEXT("Ishibashiri")) != 0) return;
    const FString Path = FString::Printf(TEXT("/Game/Characters/Production/%s/SK_%s"), Character, Character);
    USkeletalMesh* Candidate = LoadObject<USkeletalMesh>(nullptr, *Path, nullptr, LOAD_NoWarn);
    FString Reason;
    if (ApplyCandidate(Component, Candidate, Reason))
    {
        UE_LOG(LogTemp, Display, TEXT("PRODUCTION_VISUAL_SELECTED %s %s"), Character, *Path);
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("PRODUCTION_VISUAL_FALLBACK %s: %s"), Character, *Reason);
    }
}
