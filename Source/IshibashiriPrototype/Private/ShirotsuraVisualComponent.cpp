#include "ShirotsuraVisualComponent.h"
#include "ProductionVisuals.h"
#include "Animation/AnimSequence.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
    const TCHAR* const ClipNames[] = {TEXT("Idle"), TEXT("Walk"), TEXT("Run"), TEXT("Slash"), TEXT("Dodge"), TEXT("Climb"), TEXT("Hang"),
        TEXT("Grip"), TEXT("Jump"), TEXT("Death")};
    static_assert(UE_ARRAY_COUNT(ClipNames) == static_cast<int32>(EShirotsuraVisualState::Count));
}

UShirotsuraVisualComponent::UShirotsuraVisualComponent() { PrimaryComponentTick.bCanEverTick = true; }

void UShirotsuraVisualComponent::Configure(USkeletalMeshComponent* InMesh, UPrimitiveComponent* InFallback,
    UPrimitiveComponent* InFallbackWeapon, UPrimitiveComponent* InAdditionalFallback, ECampaignState InStandaloneEncounter)
{
    Mesh = InMesh;
    Fallback = InFallback;
    FallbackWeapon = InFallbackWeapon;
    AdditionalFallback = InAdditionalFallback;
    StandaloneEncounter = InStandaloneEncounter;
    if (Mesh)
    {
        Mesh->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Rigged/Shirotsura/SK_Shirotsura")));
        Mesh->SetRelativeLocation(FVector(0, 0, -88));
        Mesh->SetRelativeRotation(FRotator(0, 90, 0));
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    }
    Animations.Reset();
    for (const TCHAR* Clip : ClipNames)
        Animations.Add(
            LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("/Game/Characters/Rigged/Shirotsura/AN_Shirotsura_%s"), Clip)));
    bUsingRig = Mesh && Mesh->GetSkeletalMeshAsset() && Animations.Num() == UE_ARRAY_COUNT(ClipNames);
    for (const UAnimSequence* Animation : Animations) bUsingRig &= Animation != nullptr;
    RefreshFallbackVisibility();
}

void UShirotsuraVisualComponent::BeginPlay()
{
    Super::BeginPlay();
    if (bUsingRig)
    {
        ProductionVisuals::ApplyAtBeginPlay(Mesh, TEXT("Shirotsura"));
        bUsingRig = Mesh && Mesh->GetSkeletalMeshAsset();
        ApplyCorruptionAppearance();
    }
    RefreshFallbackVisibility();
    ResetPresentation();
}

void UShirotsuraVisualComponent::ApplyCorruptionAppearance()
{
    bCorruptionStageResolved = false;
    bCorruptionAppearanceApplied = false;
    CorruptionMaterials.Reset();
    if (!Mesh || !Mesh->GetSkeletalMeshAsset()) return;

    const UCampaignGameInstance* Campaign = GetWorld() ? GetWorld()->GetGameInstance<UCampaignGameInstance>() : nullptr;
    EShirotsuraCorruptionStage Stage;
    if (!Campaign || !Campaign->GetCorruptionStageForEncounter(StandaloneEncounter, Stage))
    {
        UE_LOG(LogTemp, Verbose, TEXT("SHIROTSURA_CORRUPTION_SKIPPED stage unavailable"));
        return;
    }
    bCorruptionStageResolved = true;

    // The audited rig has a dedicated left-arm corruption section. Head and
    // neck share the skin atlas with the right hand, so changing that slot
    // would violate the unaffected-region contract and is deliberately skipped.
    static const FName CorruptionSlot(TEXT("09 • petrified corruption"));
    static const FName IntensityParameter(TEXT("ShirotsuraCorruptionIntensity"));
    const int32 SlotIndex = Mesh->GetMaterialIndex(CorruptionSlot);
    if (SlotIndex == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("SHIROTSURA_CORRUPTION_UNSUPPORTED mesh=%s missing_slot=%s"),
            *GetNameSafe(Mesh->GetSkeletalMeshAsset()), *CorruptionSlot.ToString());
        return;
    }
    UMaterialInterface* Source = Mesh->GetMaterial(SlotIndex);
    float ExistingValue = 0.f;
    if (!Source || !Source->GetScalarParameterValue(FMaterialParameterInfo(IntensityParameter), ExistingValue))
    {
        UE_LOG(LogTemp, Warning, TEXT("SHIROTSURA_CORRUPTION_UNSUPPORTED mesh=%s slot=%s missing_parameter=%s"),
            *GetNameSafe(Mesh->GetSkeletalMeshAsset()), *CorruptionSlot.ToString(), *IntensityParameter.ToString());
        return;
    }
    UMaterialInstanceDynamic* Instance = Mesh->CreateDynamicMaterialInstance(SlotIndex, Source);
    if (!Instance) return;
    Instance->SetScalarParameterValue(IntensityParameter, Stage == EShirotsuraCorruptionStage::Early ? .28f : 1.f);
    CorruptionMaterials.Add(Instance);
    bCorruptionAppearanceApplied = true;
    UE_LOG(LogTemp, Display, TEXT("SHIROTSURA_CORRUPTION_APPLIED stage=%s slot=%s"),
        Stage == EShirotsuraCorruptionStage::Early ? TEXT("Early") : TEXT("Advanced"), *CorruptionSlot.ToString());
}

void UShirotsuraVisualComponent::RefreshFallbackVisibility()
{
    if (Mesh) Mesh->SetVisibility(bUsingRig, true);
    if (Fallback) Fallback->SetVisibility(!bUsingRig, true);
    if (AdditionalFallback) AdditionalFallback->SetVisibility(!bUsingRig, true);
    if (FallbackWeapon) FallbackWeapon->SetVisibility(!bUsingRig && !bWeaponHidden, true);
}

void UShirotsuraVisualComponent::SetState(EShirotsuraVisualState State, float PlayRate)
{
    if (OneShotRemaining <= 0.f) ApplyState(State, PlayRate);
}

void UShirotsuraVisualComponent::PlayOneShot(EShirotsuraVisualState State, float MinimumDuration)
{
    if (!bUsingRig) return;
    const int32 Index = static_cast<int32>(State);
    const float ClipDuration = Animations.IsValidIndex(Index) && Animations[Index] ? Animations[Index]->GetPlayLength() : 0.f;
    OneShotRemaining = FMath::Max(MinimumDuration, ClipDuration);
    ApplyState(State, 1.f);
}

void UShirotsuraVisualComponent::ApplyState(EShirotsuraVisualState State, float PlayRate)
{
    if (!bUsingRig || !Mesh || Mesh->GetAnimationMode() != EAnimationMode::AnimationSingleNode) return;
    const int32 Next = static_cast<int32>(State);
    if (Next == CurrentState || !Animations.IsValidIndex(Next) || !Animations[Next]) return;
    CurrentState = Next;
    const bool bLoop =
        State != EShirotsuraVisualState::Slash && State != EShirotsuraVisualState::Dodge && State != EShirotsuraVisualState::Death;
    Mesh->PlayAnimation(Animations[Next], bLoop);
    Mesh->SetPlayRate(PlayRate);
}

void UShirotsuraVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    OneShotRemaining = FMath::Max(0.f, OneShotRemaining - FMath::Max(0.f, DeltaTime));
}

void UShirotsuraVisualComponent::ResetPresentation()
{
    OneShotRemaining = 0.f;
    CurrentState = INDEX_NONE;
    SetWeaponHidden(false);
    if (Mesh && Mesh->GetAnimationMode() != EAnimationMode::AnimationSingleNode)
        Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    ApplyState(EShirotsuraVisualState::Idle, 1.f);
}

void UShirotsuraVisualComponent::SetWeaponHidden(bool bHidden)
{
    if (bWeaponHidden == bHidden) return;
    bWeaponHidden = bHidden;
    if (Mesh && bUsingRig)
    {
        if (bHidden) Mesh->HideBoneByName(TEXT("weapon"), EPhysBodyOp::PBO_None);
        else Mesh->UnHideBoneByName(TEXT("weapon"));
    }
    if (FallbackWeapon) FallbackWeapon->SetVisibility(!bUsingRig && !bHidden, true);
}
