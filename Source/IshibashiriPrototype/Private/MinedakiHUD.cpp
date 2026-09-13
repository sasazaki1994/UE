#include "MinedakiHUD.h"
#include "MinedakiGameMode.h"
#include "MinedakiBoss.h"
#include "MinedakiPlayer.h"
#include "StaminaComponent.h"
#include "NushiProgressComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
void AMinedakiHUD::DrawHUD()
{
    Super::DrawHUD(); auto* Mode=GetWorld()->GetAuthGameMode<AMinedakiGameMode>();
    if(!Canvas || !GEngine || !Mode || !Mode->GetBoss() || !Mode->GetPlayer()) return;
    auto* B=Mode->GetBoss(); auto* P=Mode->GetPlayer();
    DrawRect(FLinearColor(0,0,0,.8),12,12,900,150);
    float Y=20; auto Line=[&](const FString& Text,FLinearColor Color=FLinearColor::White) { DrawText(Text,Color,24,Y,GEngine->GetMediumFont()); Y+=25; };
    Line(TEXT("MINEDAKI / THE CLIMBING MOUNTAIN"),FLinearColor(.4,1,.8));
    Line(FString::Printf(TEXT("State: %s | KAKON %d/3 | STAMINA %.0f/100 | NODE %d/8"),*B->GetActionLabel(),B->GetNushiProgressComponent()->GetPurifiedCount(),P->GetStamina()->GetCurrentStamina(),P->GetRouteNode()+1));
    Line(P->HasFallen()?TEXT("FALL - Return to ground. R / Y: Retry"):
        B->IsSliceComplete()?TEXT("MINEDAKI SLICE COMPLETE - KAKON 1/3 | R / Y: Retry"):
        B->IsWallMoving()?(P->IsClinging()?TEXT("CLING - holding on. Stamina drains; keep E / RB held through the shake"):TEXT("HOLD E / RB TO CLING! Arm regrip will throw you off")):
        B->GetActionState()==EMinedakiActionState::UpperPlatform?TEXT("Rest at NODE 5 to refill. Climb to NODE 7, then LMB / X: purify"):
        P->IsMounted()?TEXT("W / LS up: climb to the back. Hold E / RB when the mountain moves"):
        TEXT("Approach the green LEFT LEG marker, then E / RB: Grab"),FLinearColor(1,.8,.25));
    Line(TEXT("WASD / LS: move & climb | Mouse / RS: camera | Space / A: detach | R / Y: retry"));
    Line(FString::Printf(TEXT("HEIGHT %.1fm | Cling %.1fs | Shake %d | Falls %d"),P->GetActorLocation().Z/100,B->Telemetry.ClingSeconds,B->Telemetry.Shakes,B->Telemetry.Falls));
}
