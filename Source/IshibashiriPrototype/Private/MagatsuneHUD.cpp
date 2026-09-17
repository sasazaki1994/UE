#include "MagatsuneHUD.h"
#include "MagatsuneGameMode.h"
#include "MagatsuneBoss.h"
#include "MagatsunePlayer.h"
#include "PlayerSenseComponent.h"
#include "StaminaComponent.h"
#include "NushiProgressComponent.h"
#include "NushiEncounterManager.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

void AMagatsuneHUD::DrawHUD()
{
    Super::DrawHUD();
    auto* M = GetWorld()->GetAuthGameMode<AMagatsuneGameMode>();
    if (!Canvas || !GEngine || !M || !M->GetBoss()) return;
    auto* B = M->GetBoss();
    auto* P = M->GetPlayer();
    const bool Victory = M->GetManager()->GetEncounterState() == ENushiEncounterState::Completed && B->GetPhase() == EMagatsunePhase::Calm;
    DrawRect({0, 0, 0, .82f}, 12, 12, 1120, 165);
    float Y = 20;
    auto Line = [&](FString S, FLinearColor C = FLinearColor::White)
    {
        DrawText(S, C, 24, Y, GEngine->GetMediumFont());
        Y += 26;
    };
    Line(TEXT("MAGATSUNE / LIVING CORRUPTED TERRAIN"), {.75f, .3f, 1});
    Line(FString::Printf(TEXT("%s | KAKON %d/3 | STAMINA %.0f | ROUTE %d/12"), *B->GetPhaseLabel(),
        B->GetNushiProgressComponent()->GetPurifiedCount(), P->GetStamina()->GetCurrentStamina(), P->GetRouteNode() + 1));
    Line(Victory                                        ? TEXT("CALMED | ENCOUNTER COMPLETED | VICTORY | R / Y: Retry")
            : B->GetPhase() == EMagatsunePhase::Calming ? TEXT("CALMING - the roots are settling, not disappearing")
            : P->IsRecovering()                         ? TEXT("RECOVERY: follow amber root base, then E / RB to re-Grab")
            : B->IsLargePulse()                         ? TEXT("LARGE PULSE - HOLD E / RB TO CLING")
            : P->IsMounted()                            ? TEXT("NEXT ROUTE: W / LS | SAFE ROCKS RESTORE STAMINA | LMB / X: Purify")
                                                        : TEXT("GRAB AVAILABLE: approach Root A and press E / RB"),
        {1, .8f, .2f});
    if (P->GetSense()->IsBoundarySenseActive())
        Line(FString::Printf(TEXT("BOUNDARY SENSE: %s"), *P->GetSense()->GetBoundaryStrengthLabel()), {.3f, .9f, 1});
    if (P->GetSense()->IsCorruptionSenseActive())
        Line(FString::Printf(TEXT("CORRUPTION SENSE: %s | RECOVERY x%.1f"), *P->GetSense()->GetCorruptionWarningLabel(),
                 P->GetSense()->GetRecoveryMultiplier()),
            {1, .35f, .55f});
    Line(TEXT("WASD / LS Move | Mouse / RS Camera | E / RB Grab-Cling | Space / A Detach | R / Y Retry"));
}
