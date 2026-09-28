#include "PrototypeHUD.h"
#include "PrototypeGameMode.h"
#include "PrototypePlayer.h"
#include "PlayerSenseComponent.h"
#include "IshibashiriBoss.h"
#include "KakonActor.h"
#include "ColossusClimbingComponent.h"
#include "DebugGuidance.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void APrototypeHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !GEngine) return;
    const APrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (!Mode) return;
    const APrototypePlayer* Player = Mode->GetPlayer();
    const AIshibashiriBoss* Boss = Mode->GetBoss();
    if (!Player || !Boss) return;

    const bool bDebugGuidance = IsDebugGuidanceEnabled();
    const float Dt = GetWorld()->GetDeltaSeconds();
    const float Scale = FMath::Clamp(Canvas->ClipY / 900.f, 0.65f, 1.4f) * FMath::Clamp(UserInterfaceScale, .75f, 1.5f);
    const float Margin = 22.f * Scale;
    const UColossusClimbingComponent* Climbing = Player->GetClimbing();

    // A new Boss marks a new encounter/session. HUD memory is presentation-only.
    if (ObservedBoss.Get() != Boss || Boss->GetPurifiedCount() < PreviousPurifiedCount)
    {
        ObservedBoss = const_cast<AIshibashiriBoss*>(Boss);
        PreviousPurifiedCount = Boss->GetPurifiedCount();
        PreviousPosture = Boss->GetPosture();
        PurificationNoticeRemaining = FullStaminaTailRemaining = 0.f;
        bPreviousBuckWarning = bSawDodge = bSawCounter = bSawGrab = bSawCling = bSawPurify = false;
    }

    if (Player->IsDodging()) bSawDodge = true;
    if (Boss->GetPosture() < PreviousPosture) bSawCounter = true;
    if (Player->IsGrabbing()) bSawGrab = true;
    if (Boss->GetPurifiedCount() > PreviousPurifiedCount)
    {
        PreviousPurifiedCount = Boss->GetPurifiedCount();
        PurificationNoticeRemaining = 1.5f;
        bSawPurify = true;
    }
    const bool bBuckWarning = Boss->IsBuckWarning();
    if (bBuckWarning && Climbing->IsGripping()) bSawCling = true;
    if (bPreviousBuckWarning && !bBuckWarning) bSawCling = true;
    bPreviousBuckWarning = bBuckWarning;
    PreviousPosture = Boss->GetPosture();
    PurificationNoticeRemaining = FMath::Max(0.f, PurificationNoticeRemaining - Dt);

    const float Stamina = Climbing->GetStamina();
    if (Stamina < 99.5f) FullStaminaTailRemaining = 1.5f;
    else FullStaminaTailRemaining = FMath::Max(0.f, FullStaminaTailRemaining - Dt);

    // The direction indicator is contextual, not a permanent range-like ruler.
    if (Mode->IsEncounterActive() && !Player->IsGrabbing() && Player->IsAttacking() && PlayerOwner)
    {
        const FVector Origin = Player->GetActorLocation() + FVector(0.f, 0.f, 15.f);
        FVector2D ScreenOrigin, ScreenTip;
        if (PlayerOwner->ProjectWorldLocationToScreen(Origin, ScreenOrigin)
            && PlayerOwner->ProjectWorldLocationToScreen(Origin + Player->GetAttackIndicatorDirection() * Player->AttackReach, ScreenTip)
            && !ScreenOrigin.Equals(ScreenTip, 1.f))
        {
            const FVector2D Along = (ScreenTip - ScreenOrigin).GetSafeNormal();
            const FVector2D Across(-Along.Y, Along.X);
            const FVector2D Start = ScreenOrigin + Along * 28.f * Scale;
            const FVector2D End = Start + Along * 72.f * Scale;
            const FVector2D HeadA = End - Along * 16.f * Scale + Across * 10.f * Scale;
            const FVector2D HeadB = End - Along * 16.f * Scale - Across * 10.f * Scale;
            for (const FVector2D Point : {Start, HeadA, HeadB})
            {
                DrawLine(End.X, End.Y, Point.X, Point.Y, FLinearColor::Black, 8.f * Scale);
                DrawLine(End.X, End.Y, Point.X, Point.Y, FLinearColor::White, 4.f * Scale);
            }
        }
    }

    // Stamina is a non-numeric survival cue in Production and appears only while relevant.
    const bool bShowStamina = Player->IsGrabbing() && (Stamina < 90.f || bBuckWarning || Boss->IsBucking() || FullStaminaTailRemaining > 0.f);
    if (bShowStamina)
    {
        const float W = 230.f * Scale;
        DrawRect(FLinearColor(0.f, 0.f, 0.f, .72f), Margin, Margin, W, 20.f * Scale);
        DrawRect(Stamina < 25.f ? FLinearColor(.95f, .22f, .12f) : FLinearColor(.82f, .68f, .22f),
            Margin + 3.f * Scale, Margin + 3.f * Scale, (W - 6.f * Scale) * Stamina / 100.f, 14.f * Scale);
    }

    FString Prompt;
    bool bPurifiableCoreNear = false;
    if (Player->IsGrabbing() && Climbing->IsResting() && !Boss->IsBucking())
    {
        for (int32 I = 0; I < Boss->GetCoreKakonCount(); ++I)
        {
            const AKakonActor* Kakon = Boss->GetCoreKakon(I);
            bPurifiableCoreNear |= Kakon && Kakon->GetState() != EKakonState::Purified
                && FVector::Dist(Player->GetActorLocation(), Kakon->GetActorLocation()) < 170.f;
        }
    }
    if (ButtonPrompts != EHUDButtonPromptMode::Off && Mode->IsEncounterActive())
    {
        const bool bAlways = ButtonPrompts == EHUDButtonPromptMode::Always;
        if ((bAlways || !bSawDodge) && Boss->GetState() == EIshibashiriState::Telegraph)
            Prompt = TEXT("Shift / B  回避");
        else if ((bAlways || !bSawCounter) && Boss->CanBeCountered())
            Prompt = TEXT("今だ: LMB / X  反撃");
        else if ((bAlways || !bSawGrab) && Boss->CanMount()
            && FVector::Dist(Player->GetActorLocation(), Boss->GetActorLocation()) <= 400.f)
            Prompt = TEXT("E / RB  取り付く");
        else if ((bAlways || !bSawCling) && Player->IsGrabbing() && bBuckWarning)
            Prompt = TEXT("E / RB  長押し");
        else if ((bAlways || !bSawPurify) && Player->IsGrabbing()
            && bPurifiableCoreNear)
            Prompt = TEXT("LMB / X  浄化");
    }
    if (!Prompt.IsEmpty())
    {
        const float W = FMath::Min(430.f * Scale, Canvas->ClipX - Margin * 2.f);
        const float Left = (Canvas->ClipX - W) * .5f;
        DrawRect(FLinearColor(0.f, 0.f, 0.f, .76f), Left, Canvas->ClipY - 82.f * Scale, W, 48.f * Scale);
        DrawText(Prompt, FLinearColor(.95f, .88f, .62f), Left + 18.f * Scale, Canvas->ClipY - 70.f * Scale,
            GEngine->GetMediumFont(), Scale, false);
    }

    if (PurificationNoticeRemaining > 0.f)
    {
        const FString Notice = FString::Printf(TEXT("禍根 %d/%d"), Boss->GetPurifiedCount(), Boss->GetCoreKakonCount());
        DrawText(Notice, FLinearColor(.95f, .82f, .42f), Canvas->ClipX * .5f - 62.f * Scale,
            Canvas->ClipY * .23f, GEngine->GetMediumFont(), 1.15f * Scale, false);
    }

    if (bDebugGuidance)
    {
        float Y = Margin;
        const float W = FMath::Min(760.f * Scale, Canvas->ClipX - Margin * 2.f);
        DrawRect(FLinearColor(.02f, .025f, .03f, .88f), Margin, Y, W, 150.f * Scale);
        auto DebugLine = [this, Margin, &Y, Scale](const FString& Text, FLinearColor Color = FLinearColor::White)
        {
            DrawText(Text, Color, Margin + 10.f * Scale, Y + 8.f * Scale, GEngine->GetMediumFont(), .78f * Scale, false);
            Y += 27.f * Scale;
        };
        DebugLine(TEXT("DEBUG GUIDANCE"), FLinearColor(.35f, .9f, 1.f));
        DebugLine(FString::Printf(TEXT("HP %d/%d | POSTURE %d/%d | STATE %s (%.2fs)"), Player->GetHealth(), Player->MaxHealth,
            Boss->GetPosture(), Boss->MaxPosture, *Boss->GetStateLabel(), Boss->GetStateTimeRemaining()));
        DebugLine(FString::Printf(TEXT("STAMINA %.1f/100 | KAKON %d/%d | ROUTE %d -> %d | GRIP %s | SHAKE %s"), Stamina,
            Boss->GetPurifiedCount(), Boss->GetCoreKakonCount(), Climbing->GetNode(), Climbing->GetDestination(),
            Climbing->IsGripping() ? TEXT("ON") : TEXT("OFF"), Boss->IsBucking() ? TEXT("ACTIVE") : bBuckWarning ? TEXT("WARNING") : TEXT("OFF")));
        DebugLine(FString::Printf(TEXT("SENSE Boundary=%s %s | Arm=%s %s"), Player->GetSense()->IsBoundarySenseActive() ? TEXT("ON") : TEXT("OFF"),
            *Player->GetSense()->GetBoundaryStrengthLabel(), Player->GetSense()->IsCorruptionSenseActive() ? TEXT("ON") : TEXT("OFF"),
            *Player->GetSense()->GetCorruptionWarningLabel()));
        DebugLine(TEXT("Move WASD/LS | Camera Mouse/RS | Attack LMB/X | Dodge Shift/B | Grab E/RB | Retry R/Y"));
    }

    if (!Mode->IsEncounterActive())
    {
        const bool bCalmed = Mode->GetResult() == EEncounterResult::Victory;
        const float W = FMath::Min(640.f * Scale, Canvas->ClipX - Margin * 2.f);
        const float Left = (Canvas->ClipX - W) * .5f;
        const float Top = Canvas->ClipY * .43f;
        DrawRect(FLinearColor(0.f, 0.f, 0.f, .88f), Left, Top, W, (bCalmed ? 100.f : 150.f) * Scale);
        DrawText(bCalmed ? TEXT("石走りは鎮まった / Encounter Completed") : TEXT("DEFEAT"),
            bCalmed ? FLinearColor(.82f, .78f, .57f) : FLinearColor::Red, Left + 24.f * Scale, Top + 24.f * Scale,
            GEngine->GetMediumFont(), 1.2f * Scale, false);
        if (!bCalmed)
            DrawText(TEXT("R / Y - Retry encounter"), FLinearColor::White, Left + 24.f * Scale, Top + 82.f * Scale,
                GEngine->GetMediumFont(), Scale, false);
    }
}
