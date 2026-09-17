#include "PrototypeHUD.h"
#include "CampaignGameInstance.h"
#include "PrototypeGameMode.h"
#include "PrototypePlayer.h"
#include "PlayerSenseComponent.h"
#include "IshibashiriBoss.h"
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
    const bool bDebugGuidance = IsDebugGuidanceEnabled();
    const float Scale = FMath::Clamp(Canvas->ClipY / 900.f, 0.65f, 1.4f);
    if (Player && Mode->IsEncounterActive() && PlayerOwner && !Player->IsGrabbing())
    {
        // Project the same world direction as the sword trace. Canvas keeps this
        // readable even when the raised view looks toward a boss behind the pawn.
        const FVector Direction = Player->GetAttackIndicatorDirection();
        const FVector Origin = Player->GetActorLocation() + FVector(0.f, 0.f, 15.f);
        const FVector Tip = Origin + Direction * Player->AttackReach;
        FVector2D ScreenOrigin, ScreenTip;
        if (PlayerOwner->ProjectWorldLocationToScreen(Origin, ScreenOrigin)
            && PlayerOwner->ProjectWorldLocationToScreen(Tip, ScreenTip)
            && !ScreenOrigin.Equals(ScreenTip, 1.f))
        {
            // This indicates direction, not reach. A fixed screen length avoids
            // an unreadably short arrow in perspective or a clipped one overhead.
            const FVector2D Along = (ScreenTip - ScreenOrigin).GetSafeNormal();
            const FVector2D Across(-Along.Y, Along.X);
            const FVector2D Start = ScreenOrigin + Along * 28.f * Scale;
            const FVector2D EndPoint = Start + Along * 72.f * Scale;
            const FVector2D Screen[] = { Start, EndPoint,
                EndPoint - Along * 16.f * Scale + Across * 10.f * Scale,
                EndPoint - Along * 16.f * Scale - Across * 10.f * Scale };
            const FLinearColor Color = Player->IsAttacking() ? FLinearColor::White : FLinearColor(0.2f, 0.9f, 1.f);
            const int32 Ends[] = { 0, 2, 3 };
            for (int32 End : Ends)
            {
                DrawLine(Screen[1].X, Screen[1].Y, Screen[End].X, Screen[End].Y, FLinearColor::Black, 8.f * Scale);
                DrawLine(Screen[1].X, Screen[1].Y, Screen[End].X, Screen[End].Y, Color, 4.f * Scale);
            }
        }
    }
    const float X = 22.f * Scale;
    float Y = 18.f * Scale;
    const float Width = FMath::Min((bDebugGuidance ? 630.f : 430.f) * Scale, Canvas->ClipX - X * 2.f);
    DrawRect(FLinearColor(0.015f, 0.02f, 0.025f, 0.72f), X - 8.f, Y - 8.f, Width, (bDebugGuidance ? 195.f : 105.f) * Scale);
    auto Line = [this, X, &Y, Scale](const FString& Text, FLinearColor Color = FLinearColor::White, float Size = 1.f)
    {
        DrawText(Text, Color, X, Y, GEngine->GetMediumFont(), Scale * Size, false);
        Y += 26.f * Scale * Size;
    };
    Line(TEXT("MAGAHARAI / ISHIBASHIRI"), FLinearColor(0.85f, 0.88f, 0.78f), 1.2f);
    if (const auto* Campaign=GetGameInstance<UCampaignGameInstance>(); Campaign && Campaign->IsCampaignActive())
        // The combat HUD stays English so it never depends on a CJK-capable UFont.
        Line(TEXT("Q / LT: Boundary Sense    F / LB: Corrupted Arm"),FLinearColor(.3f,.9f,1.f),.8f);
    if (!Player || !Boss)
    {
        Line(TEXT("Spawn failed. Use Play, not Simulate. See Output Log."), FLinearColor::Red);
        return;
    }
    Line(FString::Printf(TEXT("STAMINA  %.0f / 100        KAKON  %d / 3"),
        Player->GetClimbing()->GetStamina(),Boss->GetPurifiedCount()),FLinearColor(.82f,.78f,.57f));
    if(Player->GetSense()->IsBoundarySenseActive()) Line(FString::Printf(TEXT("BOUNDARY SENSE: %s"),*Player->GetSense()->GetBoundaryStrengthLabel()),FLinearColor(.3f,.9f,1));
    if(Player->GetSense()->IsCorruptionSenseActive()) Line(FString::Printf(TEXT("CORRUPTION SENSE: %s"),*Player->GetSense()->GetCorruptionWarningLabel()),FLinearColor(1,.35f,.55f));
    if (bDebugGuidance)
    {
        Line(FString::Printf(TEXT("PLAYER HP %d/%d | BOSS HP %d/%d | %s (%.2fs)"),Player->GetHealth(),Player->MaxHealth,
            Boss->GetHealth(),Boss->MaxHealth,*Boss->GetStateLabel(),Boss->GetStateTimeRemaining()));
        Line(TEXT("Move WASD/LS | Camera Mouse/RS | Attack LMB/X | Dodge Shift/B"),FLinearColor(.7f,.8f,.85f),.75f);
    }
    if (!Player->GetFeedback().IsEmpty()) Line(Player->GetFeedback(), FLinearColor(.88f,.76f,.42f),.85f);

    if (bDebugGuidance || Player->IsGrabbing())
    {
        const FString Hint = Player->bUseRouteClimbing ? Player->GetClimbing()->GetHint()
            : TEXT("Hold E / RB: grab | WASD / LS: local climb | Release E / RB: detach | R / Y: retry");
        DrawRect(FLinearColor(0.f,0.f,0.f,.72f),0.f,Canvas->ClipY-46.f*Scale,Canvas->ClipX,46.f*Scale);
        DrawText(Hint,Boss->CanBeCountered()?FLinearColor::Green:FLinearColor::White,
            X,Canvas->ClipY-33.f*Scale,GEngine->GetMediumFont(),Scale,false);
    }

    if (!Mode->IsEncounterActive())
    {
        const bool bVictory = Mode->GetResult() == EEncounterResult::Victory;
        const float BoxWidth = FMath::Min(640.f * Scale, Canvas->ClipX - 40.f);
        const float Left = (Canvas->ClipX - BoxWidth) * 0.5f;
        const float Top = Canvas->ClipY * 0.43f;
        DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.9f), Left, Top, BoxWidth, 170.f * Scale);
        DrawText(bVictory ? TEXT("VICTORY - Ishibashiri is calmed") : TEXT("DEFEAT"), bVictory ? FLinearColor::Green : FLinearColor::Red,
            Left + 24.f * Scale, Top + 24.f * Scale, GEngine->GetMediumFont(), 1.4f * Scale, false);
        // Calm can come from counters, purified cores, or a mix (both reduce boss
        // HP); show both tallies so a partial KAKON count is not read as a bug.
        DrawText(FString::Printf(TEXT("BOSS HP %d / %d   KAKON %d / %d"),
                Boss->GetHealth(), Boss->MaxHealth, Boss->GetPurifiedCount(), Boss->GetCoreKakonCount()),
            FLinearColor(.82f, .78f, .57f), Left + 24.f * Scale, Top + 70.f * Scale, GEngine->GetMediumFont(), 0.9f * Scale, false);
        DrawText(TEXT("R / Y - Retry encounter"), FLinearColor::White,
            Left + 24.f * Scale, Top + 108.f * Scale, GEngine->GetMediumFont(), 1.2f * Scale, false);
    }
}
