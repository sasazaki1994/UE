#include "MinedakiHUD.h"
#include "MinedakiGameMode.h"
#include "MinedakiBoss.h"
#include "MinedakiPlayer.h"
#include "PlayerSenseComponent.h"
#include "StaminaComponent.h"
#include "NushiProgressComponent.h"
#include "NushiEncounterManager.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "DebugGuidance.h"

void AMinedakiHUD::DrawHUD()
{
    Super::DrawHUD();
    auto* Mode = GetWorld()->GetAuthGameMode<AMinedakiGameMode>();
    if (!Canvas || !GEngine || !Mode || !Mode->GetBoss() || !Mode->GetPlayer() || !Mode->GetManager()) return;
    auto* B = Mode->GetBoss();
    auto* P = Mode->GetPlayer();
    const bool bDebugGuidance = IsDebugGuidanceEnabled();
    const float Scale = FMath::Clamp(Canvas->ClipY / 900.f, .7f, 1.4f);
    const int32 SenseLines = (P->GetSense()->IsBoundarySenseActive() ? 1 : 0) + (P->GetSense()->IsCorruptionSenseActive() ? 1 : 0);
    const int32 LineCount = 4 + SenseLines + (bDebugGuidance ? 2 : 0);
    DrawRect(FLinearColor(0, 0, 0, .8), 12, 12, FMath::Min(Canvas->ClipX - 24.f, (bDebugGuidance ? 1050.f : 820.f) * Scale), (16 + 25 * LineCount) * Scale);
    float Y = 20 * Scale;
    auto Line = [&](const FString& Text, FLinearColor Color = FLinearColor::White)
    {
        DrawText(Text, Color, 24 * Scale, Y, GEngine->GetMediumFont(), Scale);
        Y += 25 * Scale;
    };
    Line(TEXT("MINEDAKI / THE CLIMBING MOUNTAIN"), FLinearColor(.4, 1, .8));
    Line(FString::Printf(TEXT("KAKON %d/3 | STAMINA %.0f/100"),
        B->GetNushiProgressComponent()->GetPurifiedCount(), P->GetStamina()->GetCurrentStamina()));
    if (P->GetSense()->IsBoundarySenseActive())
        Line(FString::Printf(TEXT("BOUNDARY SENSE: %s"), *P->GetSense()->GetBoundaryStrengthLabel()), FLinearColor(.3f, .9f, 1));
    if (P->GetSense()->IsCorruptionSenseActive())
        Line(FString::Printf(TEXT("CORRUPTION SENSE: %s"), *P->GetSense()->GetCorruptionWarningLabel()),
            FLinearColor(1, .35f, .55f));
    const bool Victory = Mode->GetManager()->GetEncounterState() == ENushiEncounterState::Completed;
    Line(Victory                       ? TEXT("VICTORY - MINEDAKI CALMED / ENCOUNTER COMPLETED | R / Y: Retry")
            : P->HasFallen()           ? TEXT("FALL - recovery anchor will preserve KAKON progress")
            : P->IsRecovering()        ? TEXT("RECOVERY READY - reach the amber anchor and press E / RB")
            : B->IsBodyTransitioning() ? (P->IsClinging() ? TEXT("CLING - body route changing; wait for the next green anchors")
                                                          : TEXT("HOLD E / RB! The arm/head route is moving"))
            : B->IsWallMoving()        ? (P->IsClinging() ? TEXT("CLING - holding on. Keep E / RB held through the shake")
                                                          : TEXT("HOLD E / RB TO CLING! Arm regrip will throw you off"))
            : B->GetActionState() == EMinedakiActionState::UpperPlatform ? TEXT("REST HERE, THEN CLIMB TO THE FIRST KAKON")
            : B->GetActionState() == EMinedakiActionState::ArmBridge ? TEXT("ARM BRIDGE OPEN - CROSS TO THE NEXT KAKON")
            : B->GetActionState() == EMinedakiActionState::FinalRoute
            ? TEXT("FINAL HEAD ROUTE OPEN - REST, THEN CLIMB TO THE LAST KAKON")
            : P->IsMounted() ? TEXT("W / LS up: climb to the back. Hold E / RB when the mountain moves")
                             : TEXT("Approach the green LEFT LEG marker, then E / RB: Grab"),
        FLinearColor(1, .8, .25));
    Line(TEXT("WASD/LS move & climb | E/RB grab-cling | Space/A detach | R/Y retry"));
    if (bDebugGuidance)
    {
        Line(FString::Printf(TEXT("DEBUG State=%s | RouteNode=%d/%d | OpenThrough=%d"), *B->GetActionLabel(),
            P->GetRouteNode() + 1, AMinedakiBoss::RouteNodeCount, B->GetMaximumRouteNode() + 1), FLinearColor(.45f,.75f,1.f));
        Line(FString::Printf(TEXT("DEBUG Height=%.1fm | Cling=%.1fs | Shakes=%d | Falls=%d | Recoveries=%d | RecoveryMultiplier=%.1f"),
            P->GetActorLocation().Z / 100, B->Telemetry.ClingSeconds, B->Telemetry.Shakes, B->Telemetry.Falls,
            B->Telemetry.RecoverySuccesses, P->GetSense()->GetRecoveryMultiplier()), FLinearColor(.45f,.75f,1.f));
    }
}
