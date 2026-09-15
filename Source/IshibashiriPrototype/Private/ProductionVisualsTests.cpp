#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "ProductionVisuals.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/SkeletalMesh.h"
#include "PlayerSenseComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProductionVisualIsolation,
    "IshibashiriPrototype.ProductionVisuals.AssetOnlyContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProductionVisualIsolation::RunTest(const FString&)
{
    for (const TCHAR* Name : {TEXT("Shirotsura"), TEXT("Ishibashiri")})
    {
        USkeletalMesh* Baseline = LoadObject<USkeletalMesh>(nullptr,
            *FString::Printf(TEXT("/Game/Characters/Rigged/%s/SK_%s"), Name, Name));
        if (!TestNotNull(TEXT("Baseline fixture"), Baseline)) return false;
        auto* Component = NewObject<USkeletalMeshComponent>();
        Component->SetSkeletalMesh(Baseline);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        const FTransform Original(FRotator(0, 90, 0), FVector(0, 0, -350));
        Component->SetRelativeTransform(Original);
        auto* Anchor = NewObject<USceneComponent>();
        Anchor->SetupAttachment(Component);
        Anchor->SetRelativeLocation(FVector(218, -155, 611));
        auto* Sense = NewObject<UPlayerSenseComponent>();
        Sense->BeginBoundarySense();
        Sense->BeginCorruptionSense();
        Sense->SetCorruptionWarning(ECorruptionWarning::Danger);
        FString Reason;
        TestFalse(TEXT("Missing candidate rejected"), ProductionVisuals::ApplyCandidate(Component, nullptr, Reason));
        TestEqual(TEXT("Fallback retained"), Component->GetSkeletalMeshAsset(), Baseline);
        USkeletalMesh* Invalid = NewObject<USkeletalMesh>();
        TestFalse(TEXT("Unrigged candidate rejected"), ProductionVisuals::ApplyCandidate(Component, Invalid, Reason));
        // A transient duplicate exercises real replacement without publishing a fake candidate.
        USkeletalMesh* Fixture = DuplicateObject<USkeletalMesh>(Baseline, GetTransientPackage());
        TestTrue(TEXT("Compatible visual assignment"), ProductionVisuals::ApplyCandidate(Component, Fixture, Reason));
        TestEqual(TEXT("Mesh actually replaced"), Component->GetSkeletalMeshAsset(), Fixture);
        TestTrue(TEXT("Component transform preserved"), Component->GetRelativeTransform().Equals(Original));
        TestEqual(TEXT("Collision preserved"), Component->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
        TestTrue(TEXT("Anchor still attached"), Anchor->GetAttachParent() == Component);
        TestTrue(TEXT("Anchor local coordinates preserved"), Anchor->GetRelativeLocation().Equals(FVector(218, -155, 611)));
        TestTrue(TEXT("Boundary state preserved"), Sense->IsBoundarySenseActive());
        TestTrue(TEXT("Arm state preserved"), Sense->IsCorruptionSenseActive());
        TestEqual(TEXT("Warning preserved"), Sense->GetCorruptionWarning(), ECorruptionWarning::Danger);
    }
    return true;
}
#endif
