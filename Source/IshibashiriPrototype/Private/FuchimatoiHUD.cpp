#include "FuchimatoiHUD.h"
#include "FuchimatoiGameMode.h"
#include "FuchimatoiBoss.h"
#include "FuchimatoiPlayer.h"
#include "PlayerSenseComponent.h"
#include "FuchimatoiRouteAnchor.h"
#include "NushiProgressComponent.h"
#include "StaminaComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "DebugGuidance.h"
void AFuchimatoiHUD::DrawHUD()
{
    Super::DrawHUD();
    AFuchimatoiGameMode* Mode=GetWorld()->GetAuthGameMode<AFuchimatoiGameMode>();
    if (!Canvas || !GEngine || !Mode || !Mode->GetBoss() || !Mode->GetPlayer()) return;
    AFuchimatoiBoss* Boss=Mode->GetBoss(); AFuchimatoiPlayer* Player=Mode->GetPlayer();
    const bool bDebugGuidance=IsDebugGuidanceEnabled();
    const float Scale=FMath::Clamp(Canvas->ClipY/900.f,.7f,1.4f);
    float Y=18; auto Line=[&](const FString& Text,FLinearColor Color=FLinearColor::White)
    {
        DrawText(Text,Color,22,Y,GEngine->GetMediumFont(),Scale); Y+=25*Scale;
    };
    const int32 SenseLines=(Player->GetSense()->IsBoundarySenseActive()?1:0)+(Player->GetSense()->IsCorruptionSenseActive()?1:0);
    const bool bRecoveryBehindCamera=Boss->IsRecoveryUnlocked()
        && Project(Boss->GetRecoveryAnchor()->GetActorLocation()).Z<=0;
    const int32 LineCount=4+SenseLines+(bDebugGuidance?2:0)+(bRecoveryBehindCamera?1:0);
    DrawRect(FLinearColor(0,0,0,.78),10,8,FMath::Min(Canvas->ClipX-20,(bDebugGuidance?900.f:720.f)*Scale),(16+25*LineCount)*Scale);
    Line(TEXT("MAGAHARAI / FUCHIMATOI"),FLinearColor(.4,1,.8));
    Line(FString::Printf(TEXT("KAKON %d/3 | STAMINA %.0f/100"),
        Boss->GetNushiProgressComponent()->GetPurifiedCount(),Player->GetStamina()->GetCurrentStamina()));
    if(Player->GetSense()->IsBoundarySenseActive()) Line(FString::Printf(TEXT("BOUNDARY SENSE: %s"),*Player->GetSense()->GetBoundaryStrengthLabel()),FLinearColor(.3f,.9f,1));
    if(Player->GetSense()->IsCorruptionSenseActive()) Line(FString::Printf(TEXT("CORRUPTION SENSE: %s"),*Player->GetSense()->GetCorruptionWarningLabel()),FLinearColor(1,.35f,.55f));
    if (bDebugGuidance)
    {
        Line(FString::Printf(TEXT("DEBUG State=%s | HP=%d/3 | Coil=%.0f%%"),*Boss->GetActionLabel(),Player->GetHealth(),Boss->GetCoilingProgress()*100),FLinearColor(.45f,.75f,1.f));
        Line(FString::Printf(TEXT("DEBUG RouteNode=%d/10 | RecoveryMultiplier=%.1f"),Player->GetRouteNode()+1,Player->GetSense()->GetRecoveryMultiplier()),FLinearColor(.45f,.75f,1.f));
    }
    if (Player->IsMounted())
    {
        AFuchimatoiRouteAnchor* Node=Boss->GetRouteAnchor(Player->GetRouteNode());
        Line(FString::Printf(TEXT("%s | W/LS climb | LMB/X purify | Space/A detach"),
            Node && Node->IsRock()?TEXT("REST: REFILL STAMINA"):TEXT("CLIMB: STAMINA DRAINS")));
    }
    else if (Boss->IsRecoveryUnlocked())
    {
        Line(Player->IsOnRecoveryGround()
            ?(Player->GetStamina()->GetCurrentStamina()<Player->MinimumGrabStamina
                ?TEXT("RECOVERY: rest for STAMINA 25, then E/RB")
                :TEXT("RECOVERY READY: E/RB at the gold beacon"))
            :TEXT("Land, then return to the gold recovery beacon"));
        const FVector Point=Boss->GetRecoveryAnchor()->GetActorLocation();
        FVector Screen=Project(Point);
        if (Screen.Z>0)
            DrawText(FString::Printf(TEXT("RETURN HERE  %.1fm"),FVector::Dist2D(Point,Player->GetActorLocation())/100.f),
                FLinearColor(1,.85,.2),FMath::Clamp(Screen.X,25.f,Canvas->ClipX-230.f),
                FMath::Clamp(Screen.Y,190.f,Canvas->ClipY-50.f),GEngine->GetMediumFont(),Scale);
        else Line(TEXT("Gold beacon is behind the camera - turn to find it"),FLinearColor(1,.85,.2));
    }
    else if (Boss->GetActionState()==EFuchimatoiActionState::BiteLunge)
        Line(TEXT("DODGE Shift/B | Grab the gold head marker with E/RB"));
    else if (Boss->IsCoiling())
        Line(TEXT("Wait for Coiling to finish, then land on the floor to recover"));
    else Line(FString::Printf(TEXT("%s | E/RB grab | Shift/B dodge | R/Y retry"),Boss->CanMount()?TEXT("GRAB NOW: gold head marker"):TEXT("Reach the gold square by the rock")));
    Line(TEXT("WASD/LS move | Mouse/RS camera | Red: purify | Gold: rest"));
    if (!Mode->IsEncounterActive())
    {
        DrawRect(FLinearColor(0,0,0,.82),Canvas->ClipX*.22f,Canvas->ClipY*.4f,Canvas->ClipX*.6f,100);
        DrawText(Mode->IsVictory()?TEXT("VICTORY - Fuchimatoi is calm"):TEXT("DEFEAT"),Mode->IsVictory()?FLinearColor::Green:FLinearColor::Red,
            Canvas->ClipX*.25f,Canvas->ClipY*.43f,GEngine->GetLargeFont(),Scale);
        DrawText(TEXT("R / Y - Retry encounter"),FLinearColor::White,Canvas->ClipX*.25f,Canvas->ClipY*.5f,GEngine->GetMediumFont(),Scale);
    }
}
