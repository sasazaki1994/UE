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
#include "DebugGuidance.h"

void AMagatsuneHUD::DrawHUD()
{
    Super::DrawHUD();
    auto* M = GetWorld()->GetAuthGameMode<AMagatsuneGameMode>();
    if (!Canvas || !GEngine || !M || !M->GetBoss() || !M->GetPlayer() || !M->GetManager()) return;
    auto* B = M->GetBoss();
    auto* P = M->GetPlayer();
    const bool bDebugGuidance = IsDebugGuidanceEnabled();
    const bool Victory = M->GetManager()->GetEncounterState() == ENushiEncounterState::Completed && B->GetPhase() == EMagatsunePhase::Calm;
    const float Scale = FMath::Clamp(Canvas->ClipY / 900.f, .7f, 1.4f);
    const int32 SenseLines = (P->GetSense()->IsBoundarySenseActive() ? 1 : 0) + (P->GetSense()->IsCorruptionSenseActive() ? 1 : 0);
    const int32 LineCount = 4 + SenseLines + (bDebugGuidance ? 1 : 0);
    DrawRect({0, 0, 0, .82f}, 12, 12, FMath::Min(Canvas->ClipX - 24.f, (bDebugGuidance ? 900.f : 760.f) * Scale), (16 + 26 * LineCount) * Scale);
    float Y = 20 * Scale;
    auto Line = [&](FString S, FLinearColor C = FLinearColor::White)
    {
        DrawText(S, C, 24 * Scale, Y, GEngine->GetMediumFont(), Scale);
        Y += 26 * Scale;
    };
    Line(TEXT("MAGATSUNE / LIVING CORRUPTED TERRAIN"), {.75f, .3f, 1});
    Line(FString::Printf(TEXT("KAKON %d/3 | STAMINA %.0f"),
        B->GetNushiProgressComponent()->GetPurifiedCount(), P->GetStamina()->GetCurrentStamina()));
    if (bDebugGuidance)
        Line(FString::Printf(TEXT("DEBUG Phase=%s | Route=%d/12"), *B->GetPhaseLabel(), P->GetRouteNode() + 1), {.45f,.75f,1.f});
    Line(Victory                                        ? TEXT("CALMED | ENCOUNTER COMPLETED | VICTORY | R / Y: Retry")
            : B->GetPhase() == EMagatsunePhase::Calming ? TEXT("CALMING - the roots are settling, not disappearing")
            : P->IsRecovering()                         ? TEXT("RECOVERY: follow the amber root, then press E / RB")
            : B->IsLargePulse()                         ? TEXT("LARGE PULSE - HOLD E / RB TO CLING")
            : B->GetAnticipation() > .15f               ? TEXT("THE ROOTS ARE GATHERING - PREPARE TO CLING")
            : P->IsMounted()                            ? TEXT("CLIMB W / LS | REST ON SAFE ROCKS | PURIFY LMB / X")
                                                        : TEXT("GRAB AVAILABLE: approach Root A and press E / RB"),
        {1, .8f, .2f});
    if (P->GetSense()->IsBoundarySenseActive())
        Line(FString::Printf(TEXT("BOUNDARY SENSE: %s"), *P->GetSense()->GetBoundaryStrengthLabel()), {.3f, .9f, 1});
    if (P->GetSense()->IsCorruptionSenseActive())
        Line(FString::Printf(TEXT("CORRUPTION SENSE: %s"), *P->GetSense()->GetCorruptionWarningLabel()),
            {1, .35f, .55f});
    Line(TEXT("WASD/LS move | E/RB grab-cling | Space/A detach | R/Y retry"));
}
