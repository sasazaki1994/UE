#include "PrototypeHUD.h"
#include "PrototypeGameMode.h"
#include "PrototypePlayer.h"
#include "IshibashiriBoss.h"
#include "ColossusClimbingComponent.h"
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
    const float Width = FMath::Min(630.f * Scale, Canvas->ClipX - X * 2.f);
    DrawRect(FLinearColor(0.015f, 0.02f, 0.025f, 0.78f), X - 8.f, Y - 8.f, Width, 195.f * Scale);
    auto Line = [this, X, &Y, Scale](const FString& Text, FLinearColor Color = FLinearColor::White, float Size = 1.f)
    {
        DrawText(Text, Color, X, Y, GEngine->GetMediumFont(), Scale * Size, false);
        Y += 26.f * Scale * Size;
    };
    Line(TEXT("MAGAHARAI / ISHIBASHIRI"), FLinearColor(0.85f, 0.88f, 0.78f), 1.2f);
    if (!Player || !Boss)
    {
        Line(TEXT("Spawn failed. Use Play, not Simulate. See Output Log."), FLinearColor::Red);
        return;
    }
    Line(FString::Printf(TEXT("PLAYER HP  %d / %d       BOSS HP  %d / %d"), Player->GetHealth(), Player->MaxHealth, Boss->GetHealth(), Boss->MaxHealth));
    Line(FString::Printf(TEXT("%s  (%.2fs)"), *Boss->GetStateLabel(), Boss->GetStateTimeRemaining()), Boss->CanBeCountered() ? FLinearColor::Green : FLinearColor::White);
    Line(FString::Printf(TEXT("E / RB grab / brace | STAMINA %.0f / 100 | CORES %d / 3"),
        Player->GetClimbing()->GetStamina(),Boss->GetPurifiedCount()),FLinearColor(1,.75,.25));
    Line(TEXT("Move: WASD / LS | Camera: Mouse / RS | Attack: LMB / X | Dodge: Shift / B"), FLinearColor(.7f,.8f,.85f), .75f);
    if (!Player->IsGrabbing()) Line(TEXT("Cyan arrow: slash direction | White: committed swing"), FLinearColor(.2f,.9f,1.f), .75f);
    Line(Player->GetFeedback(), FLinearColor::Yellow);

    const FString Hint = Player->bUseRouteClimbing ? Player->GetClimbing()->GetHint()
        : TEXT("Hold E / RB: grab | WASD / LS: local climb | Release E / RB: detach | R / Y: retry");
    DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.8f), 0.f, Canvas->ClipY - 46.f * Scale, Canvas->ClipX, 46.f * Scale);
    DrawText(Hint, Boss->CanBeCountered() ? FLinearColor::Green : FLinearColor::White,
        X, Canvas->ClipY - 33.f * Scale, GEngine->GetMediumFont(), Scale, false);

    if (!Mode->IsEncounterActive())
    {
        const bool bVictory = Mode->GetResult() == EEncounterResult::Victory;
        const float BoxWidth = FMath::Min(640.f * Scale, Canvas->ClipX - 40.f);
        const float Left = (Canvas->ClipX - BoxWidth) * 0.5f;
        const float Top = Canvas->ClipY * 0.43f;
        DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.9f), Left, Top, BoxWidth, 145.f * Scale);
        DrawText(bVictory ? TEXT("VICTORY - Ishibashiri is calmed") : TEXT("DEFEAT"), bVictory ? FLinearColor::Green : FLinearColor::Red,
            Left + 24.f * Scale, Top + 24.f * Scale, GEngine->GetMediumFont(), 1.4f * Scale, false);
        DrawText(TEXT("R / Y - Retry encounter"), FLinearColor::White,
            Left + 24.f * Scale, Top + 83.f * Scale, GEngine->GetMediumFont(), 1.2f * Scale, false);
    }
}
