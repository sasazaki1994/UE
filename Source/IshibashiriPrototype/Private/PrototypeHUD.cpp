#include "PrototypeHUD.h"
#include "PrototypeGameMode.h"
#include "PrototypePlayer.h"
#include "IshibashiriBoss.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

void APrototypeHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !GEngine) return;
    const APrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (!Mode) return;
    const APrototypePlayer* Player = Mode->GetPlayer();
    const AIshibashiriBoss* Boss = Mode->GetBoss();
    const float Scale = FMath::Clamp(Canvas->ClipY / 900.f, 0.65f, 1.4f);
    const float X = 22.f * Scale;
    float Y = 18.f * Scale;
    const float Width = FMath::Min(760.f * Scale, Canvas->ClipX - X * 2.f);
    DrawRect(FLinearColor(0.015f, 0.02f, 0.025f, 0.86f), X - 8.f, Y - 8.f, Width, 250.f * Scale);
    auto Line = [this, X, &Y, Scale](const FString& Text, FLinearColor Color = FLinearColor::White, float Size = 1.f)
    {
        DrawText(Text, Color, X, Y, GEngine->GetMediumFont(), Scale * Size, false);
        Y += 29.f * Scale * Size;
    };
    Line(TEXT("MAGAHARAI / ISHIBASHIRI"), FLinearColor(0.85f, 0.88f, 0.78f), 1.2f);
    if (!Player || !Boss)
    {
        Line(TEXT("Spawn failed. Use Play, not Simulate. See Output Log."), FLinearColor::Red);
        return;
    }
    Line(FString::Printf(TEXT("PLAYER HP  %d / %d       BOSS HP  %d / %d"), Player->GetHealth(), Player->MaxHealth, Boss->GetHealth(), Boss->MaxHealth));
    Line(FString::Printf(TEXT("%s  (%.2fs)"), *Boss->GetStateLabel(), Boss->GetStateTimeRemaining()), Boss->CanBeCountered() ? FLinearColor::Green : FLinearColor::White);
    Line(FString::Printf(TEXT("DODGE: %s   cooldown %.2fs"), Player->IsInvulnerable() ? TEXT("INVULNERABLE") : TEXT("ready when cooldown is zero"), Player->GetDodgeCooldown()), FLinearColor(0.4f, 0.85f, 1.f));
    Line(TEXT("WASD move | Mouse camera | LMB slash (camera direction)"));
    Line(TEXT("Shift / RMB dodge + direction | Space jump | R retry"));
    Line(Player->GetFeedback(), FLinearColor::Yellow);

    const FString Hint = Boss->CanBeCountered() ? TEXT("COUNTER NOW - get close and slash!")
        : TEXT("Read the red wind-up. Dodge sideways. Counter while green. Three counters to calm the boar.");
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
        DrawText(TEXT("R - Retry encounter"), FLinearColor::White,
            Left + 24.f * Scale, Top + 83.f * Scale, GEngine->GetMediumFont(), 1.2f * Scale, false);
    }
}
