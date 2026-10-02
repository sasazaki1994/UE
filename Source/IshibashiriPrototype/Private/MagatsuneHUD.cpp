#include "MagatsuneHUD.h"
#include "MagatsuneGameMode.h"
#include "MagatsuneBoss.h"
#include "MagatsunePlayer.h"
#include "PlayerSenseComponent.h"
#include "KakonActor.h"
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
    AMagatsuneGameMode* Mode = GetWorld()->GetAuthGameMode<AMagatsuneGameMode>();
    if (!Canvas || !GEngine || !Mode || !Mode->GetBoss() || !Mode->GetPlayer() || !Mode->GetManager()) return;
    AMagatsuneBoss* Boss = Mode->GetBoss();
    AMagatsunePlayer* Player = Mode->GetPlayer();
    const bool bDebugGuidance = IsDebugGuidanceEnabled();
    const bool bVictory =
        Mode->GetManager()->GetEncounterState() == ENushiEncounterState::Completed && Boss->GetPhase() == EMagatsunePhase::Calm;
    const float Dt = GetWorld()->GetDeltaSeconds();
    const float Scale = FMath::Clamp(Canvas->ClipY / 900.f, .7f, 1.4f);
    const float Margin = 22.f * Scale;
    const int32 PurifiedCount = Boss->GetNushiProgressComponent()->GetPurifiedCount();

    if (ObservedBoss.Get() != Boss || PurifiedCount < PreviousPurifiedCount)
    {
        ObservedBoss = Boss;
        PreviousPurifiedCount = PurifiedCount;
        PurificationNoticeRemaining = StaminaTailRemaining = 0.f;
    }
    if (PurifiedCount > PreviousPurifiedCount)
    {
        PreviousPurifiedCount = PurifiedCount;
        PurificationNoticeRemaining = 1.5f;
    }
    PurificationNoticeRemaining = FMath::Max(0.f, PurificationNoticeRemaining - Dt);

    const float Stamina = Player->GetStamina()->GetCurrentStamina();
    const float MaxStamina = Player->GetStamina()->GetMaxStamina();
    if (Stamina < MaxStamina - .5f) StaminaTailRemaining = 1.5f;
    else StaminaTailRemaining = FMath::Max(0.f, StaminaTailRemaining - Dt);
    const bool bShowStamina = Player->IsMounted() && (Stamina < MaxStamina * .9f || Player->IsClinging() || StaminaTailRemaining > 0.f);
    if (bShowStamina)
    {
        const float W = 230.f * Scale;
        DrawRect(FLinearColor(0.f, 0.f, 0.f, .72f), Margin, Margin, W, 20.f * Scale);
        DrawRect(Stamina < MaxStamina * .25f ? FLinearColor(.95f, .22f, .12f) : FLinearColor(.82f, .68f, .22f), Margin + 3.f * Scale,
            Margin + 3.f * Scale, (W - 6.f * Scale) * Stamina / MaxStamina, 14.f * Scale);
    }

    bool bPurifiableKakonNear = false;
    for (int32 I = 0; I < Boss->GetKakonCount(); ++I)
    {
        const AKakonActor* Kakon = Boss->GetKakon(I);
        bPurifiableKakonNear |= Player->IsMounted() && Kakon && Kakon->GetState() != EKakonState::Purified &&
            FVector::Dist(Player->GetActorLocation(), Kakon->GetActorLocation()) < 180.f;
    }
    FString Prompt;
    if (!bVictory)
    {
        if (Boss->IsLargePulse() && Player->IsMounted() && !Player->IsClinging()) Prompt = TEXT("E / RB  HOLD");
        else if (bPurifiableKakonNear) Prompt = TEXT("LMB / X  PURIFY");
        else if (!Player->IsMounted() && !Player->IsRecovering()) Prompt = TEXT("E / RB  GRAB");
        else if (Player->IsRecovering()) Prompt = TEXT("RETURN TO THE AMBER ROOT");
    }
    if (!Prompt.IsEmpty())
    {
        const float W = FMath::Min(430.f * Scale, Canvas->ClipX - Margin * 2.f);
        const float Left = (Canvas->ClipX - W) * .5f;
        DrawRect(FLinearColor(0.f, 0.f, 0.f, .76f), Left, Canvas->ClipY - 82.f * Scale, W, 48.f * Scale);
        DrawText(Prompt, FLinearColor(1.f, .8f, .25f), Left + 18.f * Scale, Canvas->ClipY - 70.f * Scale, GEngine->GetMediumFont(), Scale);
    }

    float StatusY = bShowStamina ? Margin + 32.f * Scale : Margin;
    if (Player->GetSense()->IsBoundarySenseActive())
    {
        DrawText(FString::Printf(TEXT("BOUNDARY SENSE: %s"), *Player->GetSense()->GetBoundaryStrengthLabel()), FLinearColor(.3f, .9f, 1.f),
            Margin, StatusY, GEngine->GetMediumFont(), Scale);
        StatusY += 25.f * Scale;
    }
    if (Player->GetSense()->IsCorruptionSenseActive())
    {
        DrawText(Player->GetSense()->GetCorruptionWarningLabel(), FLinearColor(1.f, .35f, .55f), Margin, StatusY, GEngine->GetMediumFont(),
            Scale);
    }

    if (PurificationNoticeRemaining > 0.f)
        DrawText(FString::Printf(TEXT("禍根 %d/%d"), PurifiedCount, Boss->GetKakonCount()), FLinearColor(.95f, .82f, .42f),
            Canvas->ClipX * .5f - 62.f * Scale, Canvas->ClipY * .23f, GEngine->GetMediumFont(), 1.15f * Scale);

    if (bDebugGuidance)
    {
        float Y = Margin + 80.f * Scale;
        auto DebugLine = [&](const FString& Text)
        {
            DrawText(Text, FLinearColor(.45f, .75f, 1.f), Margin, Y, GEngine->GetMediumFont(), .8f * Scale);
            Y += 25.f * Scale;
        };
        DebugLine(TEXT("DEBUG / MAGATSUNE"));
        DebugLine(FString::Printf(TEXT("Phase=%s | Kakon=%d/%d | Stamina=%.1f/%.1f | RouteNode=%d/12 | RecoveryMultiplier=%.1f"),
            *Boss->GetPhaseLabel(), PurifiedCount, Boss->GetKakonCount(), Stamina, MaxStamina, Player->GetRouteNode() + 1,
            Player->GetSense()->GetRecoveryMultiplier()));
        DebugLine(FString::Printf(TEXT("Telemetry Pulses=%d | Falls=%d | Recoveries=%d"), Boss->Telemetry.LargePulses,
            Boss->Telemetry.Falls, Boss->Telemetry.Recoveries));
        DebugLine(TEXT("Move WASD/LS | Camera Mouse/RS | Grab/Cling E/RB | Purify LMB/X | Detach Space/A | Retry R/Y"));
    }

    if (bVictory)
        DrawText(TEXT("禍津根は鎮まった"), FLinearColor(.82f, .78f, .57f), Canvas->ClipX * .4f, Canvas->ClipY * .43f,
            GEngine->GetLargeFont(), Scale);
}
