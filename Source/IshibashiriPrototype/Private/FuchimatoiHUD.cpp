#include "FuchimatoiHUD.h"
#include "FuchimatoiGameMode.h"
#include "FuchimatoiBoss.h"
#include "FuchimatoiPlayer.h"
#include "FuchimatoiRouteAnchor.h"
#include "NushiProgressComponent.h"
#include "StaminaComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
void AFuchimatoiHUD::DrawHUD()
{
    Super::DrawHUD();
    AFuchimatoiGameMode* Mode=GetWorld()->GetAuthGameMode<AFuchimatoiGameMode>();
    if (!Canvas || !GEngine || !Mode || !Mode->GetBoss() || !Mode->GetPlayer()) return;
    AFuchimatoiBoss* Boss=Mode->GetBoss(); AFuchimatoiPlayer* Player=Mode->GetPlayer();
    const float Scale=FMath::Clamp(Canvas->ClipY/900.f,.7f,1.4f);
    float Y=18; auto Line=[&](const FString& Text,FLinearColor Color=FLinearColor::White)
    {
        DrawText(Text,Color,22,Y,GEngine->GetMediumFont(),Scale); Y+=25*Scale;
    };
    DrawRect(FLinearColor(0,0,0,.78),10,8,FMath::Min(Canvas->ClipX-20,900*Scale),165*Scale);
    Line(TEXT("MAGAHARAI / FUCHIMATOI - primitive encounter"),FLinearColor(.4,1,.8));
    Line(Boss->GetActionLabel(),FLinearColor(1,.8,.3));
    Line(FString::Printf(TEXT("HP %d/3 | KAKON %d/3 | STAMINA %.0f/100 | COIL %.0f%%"),Player->GetHealth(),
        Boss->GetNushiProgressComponent()->GetPurifiedCount(),Player->GetStamina()->GetCurrentStamina(),Boss->GetCoilingProgress()*100));
    if (Player->IsMounted())
    {
        AFuchimatoiRouteAnchor* Node=Boss->GetRouteAnchor(Player->GetRouteNode());
        Line(FString::Printf(TEXT("NODE %d/10 - %s | W/S or LS: climb | LMB/X: purify | Space/A: detach"),
            Player->GetRouteNode()+1,Node && Node->IsRock()?TEXT("ROCK - REST TO REFILL"):TEXT("SNAKE - stamina drains")));
    }
    else Line(FString::Printf(TEXT("%s | E/RB: grab | Shift/B: dodge | R/Y: retry"),Boss->CanMount()?TEXT("GRAB POSSIBLE - reach the head"):TEXT("Bait the bite in front of the gold rock")));
    Line(TEXT("WASD / LS: move | Mouse / RS: camera | Purify red cores; gold ledges restore stamina"));
    if (!Mode->IsEncounterActive())
    {
        DrawRect(FLinearColor(0,0,0,.82),Canvas->ClipX*.22f,Canvas->ClipY*.4f,Canvas->ClipX*.6f,100);
        DrawText(Mode->IsVictory()?TEXT("VICTORY - Fuchimatoi is calm"):TEXT("DEFEAT"),Mode->IsVictory()?FLinearColor::Green:FLinearColor::Red,
            Canvas->ClipX*.25f,Canvas->ClipY*.43f,GEngine->GetLargeFont(),Scale);
        DrawText(TEXT("R / Y - Retry encounter"),FLinearColor::White,Canvas->ClipX*.25f,Canvas->ClipY*.5f,GEngine->GetMediumFont(),Scale);
    }
}
