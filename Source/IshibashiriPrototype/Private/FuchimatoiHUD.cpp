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
    else if (Boss->IsRecoveryUnlocked())
    {
        Line(Player->IsOnRecoveryGround()
            ?(Player->GetStamina()->GetCurrentStamina()<Player->MinimumGrabStamina
                ?TEXT("RECOVERY: rest on ground until STAMINA 25 | E/RB: grab")
                :TEXT("RECOVERY READY: E/RB near gold beacon | progress is kept"))
            :TEXT("Land on the floor, then return to the gold recovery beacon"));
        const FVector Point=Boss->GetRecoveryAnchor()->GetActorLocation();
        FVector Screen=Project(Point);
        if (Screen.Z>0)
            DrawText(FString::Printf(TEXT("RETURN HERE  %.1fm"),FVector::Dist2D(Point,Player->GetActorLocation())/100.f),
                FLinearColor(1,.85,.2),FMath::Clamp(Screen.X,25.f,Canvas->ClipX-230.f),
                FMath::Clamp(Screen.Y,190.f,Canvas->ClipY-50.f),GEngine->GetMediumFont(),Scale);
        else Line(TEXT("Gold beacon is behind the camera - turn to find it"),FLinearColor(1,.85,.2));
    }
    else if (Boss->GetActionState()==EFuchimatoiActionState::BiteLunge)
        Line(TEXT("Shift/B: dodge sideways | Wait for gold head marker, then E/RB"));
    else if (Boss->IsCoiling())
        Line(TEXT("Wait for Coiling to finish, then land on the floor to recover"));
    else Line(FString::Printf(TEXT("%s | E/RB: grab | Shift/B: dodge | R/Y: retry"),Boss->CanMount()?TEXT("GRAB NOW - reach the gold head marker"):TEXT("Reach the gold square in front of the rock")));
    Line(TEXT("WASD / LS: move | Mouse / RS: camera | Purify red cores; gold ledges restore stamina"));
    if (!Mode->IsEncounterActive())
    {
        DrawRect(FLinearColor(0,0,0,.82),Canvas->ClipX*.22f,Canvas->ClipY*.4f,Canvas->ClipX*.6f,100);
        DrawText(Mode->IsVictory()?TEXT("VICTORY - Fuchimatoi is calm"):TEXT("DEFEAT"),Mode->IsVictory()?FLinearColor::Green:FLinearColor::Red,
            Canvas->ClipX*.25f,Canvas->ClipY*.43f,GEngine->GetLargeFont(),Scale);
        DrawText(TEXT("R / Y - Retry encounter"),FLinearColor::White,Canvas->ClipX*.25f,Canvas->ClipY*.5f,GEngine->GetMediumFont(),Scale);
    }
}
