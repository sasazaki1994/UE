#include "FuchimatoiHUD.h"
#include "FuchimatoiGameMode.h"
#include "FuchimatoiBoss.h"
#include "FuchimatoiPlayer.h"
#include "PlayerSenseComponent.h"
#include "FuchimatoiRouteAnchor.h"
#include "KakonActor.h"
#include "NushiProgressComponent.h"
#include "StaminaComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "DebugGuidance.h"

void AFuchimatoiHUD::DrawHUD()
{
    Super::DrawHUD();
    AFuchimatoiGameMode* Mode = GetWorld()->GetAuthGameMode<AFuchimatoiGameMode>();
    if (!Canvas || !GEngine || !Mode || !Mode->GetBoss() || !Mode->GetPlayer()) return;
    AFuchimatoiBoss* Boss = Mode->GetBoss();
    AFuchimatoiPlayer* Player = Mode->GetPlayer();
    const bool bDebugGuidance = IsDebugGuidanceEnabled();
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
    const bool bShowStamina = Player->IsMounted() && (Stamina < MaxStamina * .9f || StaminaTailRemaining > 0.f);
    if (bShowStamina)
    {
        const float W = 230.f * Scale;
        DrawRect(FLinearColor(0.f, 0.f, 0.f, .72f), Margin, Margin, W, 20.f * Scale);
        DrawRect(Stamina < MaxStamina * .25f ? FLinearColor(.95f, .22f, .12f) : FLinearColor(.82f, .68f, .22f), Margin + 3.f * Scale,
            Margin + 3.f * Scale, (W - 6.f * Scale) * Stamina / MaxStamina, 14.f * Scale);
    }

    FString Prompt;
    FLinearColor PromptColor(1.f, .8f, .25f);
    bool bPurifiableKakonNear = false;
    for (int32 I = 0; I < Boss->GetKakonCount(); ++I)
    {
        const AKakonActor* Kakon = Boss->GetKakon(I);
        bPurifiableKakonNear |= Player->IsMounted() && Kakon && Kakon->GetState() != EKakonState::Purified &&
            FVector::Dist(Player->GetActorLocation(), Kakon->GetActorLocation()) < 180.f;
    }
    if (Mode->IsEncounterActive())
    {
        if (Boss->GetActionState() == EFuchimatoiActionState::BiteLunge) Prompt = TEXT("Shift / B  DODGE");
        else if (bPurifiableKakonNear) Prompt = TEXT("LMB / X  PURIFY");
        else if (!Player->IsMounted() && Boss->CanMount()) Prompt = TEXT("E / RB  GRAB");
        else if (Boss->IsRecoveryUnlocked())
            Prompt = Player->IsOnRecoveryGround() && Stamina >= Player->MinimumGrabStamina ? TEXT("E / RB  RETURN")
                                                                                           : TEXT("RETURN TO THE GOLD MARK");
        else if (Boss->IsCoiling() && Player->IsMounted()) Prompt = TEXT("REACH SAFE GROUND");
    }
    if (!Prompt.IsEmpty())
    {
        const float W = FMath::Min(430.f * Scale, Canvas->ClipX - Margin * 2.f);
        const float Left = (Canvas->ClipX - W) * .5f;
        DrawRect(FLinearColor(0.f, 0.f, 0.f, .76f), Left, Canvas->ClipY - 82.f * Scale, W, 48.f * Scale);
        DrawText(Prompt, PromptColor, Left + 18.f * Scale, Canvas->ClipY - 70.f * Scale, GEngine->GetMediumFont(), Scale);
    }

    if (Boss->IsRecoveryUnlocked())
    {
        const FVector Point = Boss->GetRecoveryAnchor()->GetActorLocation();
        const FVector Screen = Project(Point);
        if (Screen.Z > 0)
            DrawText(TEXT("RETURN"), FLinearColor(1.f, .85f, .2f), FMath::Clamp(Screen.X, 25.f, Canvas->ClipX - 120.f),
                FMath::Clamp(Screen.Y, 190.f, Canvas->ClipY - 50.f), GEngine->GetMediumFont(), Scale);
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
        DebugLine(TEXT("DEBUG / FUCHIMATOI"));
        DebugLine(FString::Printf(
            TEXT("State=%s | HP=%d/3 | Coil=%.0f%%"), *Boss->GetActionLabel(), Player->GetHealth(), Boss->GetCoilingProgress() * 100.f));
        DebugLine(FString::Printf(TEXT("Kakon=%d/%d | Stamina=%.1f/%.1f | RouteNode=%d/10 | RecoveryMultiplier=%.1f"), PurifiedCount,
            Boss->GetKakonCount(), Stamina, MaxStamina, Player->GetRouteNode() + 1, Player->GetSense()->GetRecoveryMultiplier()));
        DebugLine(TEXT("Move WASD/LS | Camera Mouse/RS | Grab E/RB | Purify LMB/X | Detach Space/A | Retry R/Y"));
    }

    if (!Mode->IsEncounterActive())
    {
        const bool bCalmed = Mode->IsVictory();
        DrawText(bCalmed ? TEXT("淵纏いは鎮まった") : TEXT("DEFEAT"), bCalmed ? FLinearColor(.82f, .78f, .57f) : FLinearColor::Red,
            Canvas->ClipX * .38f, Canvas->ClipY * .43f, GEngine->GetLargeFont(), Scale);
        if (!bCalmed)
            DrawText(TEXT("R / Y  RETRY"), FLinearColor::White, Canvas->ClipX * .42f, Canvas->ClipY * .5f, GEngine->GetMediumFont(), Scale);
    }
}
