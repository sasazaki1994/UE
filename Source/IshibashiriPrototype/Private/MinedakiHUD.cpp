#include "MinedakiHUD.h"
#include "MinedakiGameMode.h"
#include "MinedakiBoss.h"
#include "MinedakiPlayer.h"
#include "PlayerSenseComponent.h"
#include "StaminaComponent.h"
#include "NushiProgressComponent.h"
#include "NushiEncounterManager.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
void AMinedakiHUD::DrawHUD()
{
    Super::DrawHUD(); auto* Mode=GetWorld()->GetAuthGameMode<AMinedakiGameMode>();
    if(!Canvas || !GEngine || !Mode || !Mode->GetBoss() || !Mode->GetPlayer()) return;
    auto* B=Mode->GetBoss(); auto* P=Mode->GetPlayer();
    DrawRect(FLinearColor(0,0,0,.8),12,12,1050,175);
    float Y=20; auto Line=[&](const FString& Text,FLinearColor Color=FLinearColor::White) { DrawText(Text,Color,24,Y,GEngine->GetMediumFont()); Y+=25; };
    Line(TEXT("MINEDAKI / THE CLIMBING MOUNTAIN"),FLinearColor(.4,1,.8));
    Line(FString::Printf(TEXT("State: %s | KAKON %d/3 | STAMINA %.0f/100 | NODE %d/%d"),*B->GetActionLabel(),B->GetNushiProgressComponent()->GetPurifiedCount(),P->GetStamina()->GetCurrentStamina(),P->GetRouteNode()+1,AMinedakiBoss::RouteNodeCount));
    if(P->GetSense()->IsBoundarySenseActive()) Line(FString::Printf(TEXT("BOUNDARY SENSE: %s"),*P->GetSense()->GetBoundaryStrengthLabel()),FLinearColor(.3f,.9f,1));
    if(P->GetSense()->IsCorruptionSenseActive()) Line(FString::Printf(TEXT("CORRUPTION SENSE: %s | RECOVERY x%.1f"),*P->GetSense()->GetCorruptionWarningLabel(),P->GetSense()->GetRecoveryMultiplier()),FLinearColor(1,.35f,.55f));
    const bool Victory=Mode->GetManager()->GetEncounterState()==ENushiEncounterState::Completed;
    Line(Victory?TEXT("VICTORY - MINEDAKI CALMED / ENCOUNTER COMPLETED | R / Y: Retry"):
        P->HasFallen()?TEXT("FALL - recovery anchor will preserve KAKON progress"):
        P->IsRecovering()?TEXT("RECOVERY READY - move to the amber anchor and E / RB to re-Grab"):
        B->IsBodyTransitioning()?(P->IsClinging()?TEXT("CLING - body route changing; wait for the next green anchors"):TEXT("HOLD E / RB! The arm/head route is moving")):
        B->IsWallMoving()?(P->IsClinging()?TEXT("CLING - holding on. Keep E / RB held through the shake"):TEXT("HOLD E / RB TO CLING! Arm regrip will throw you off")):
        B->GetActionState()==EMinedakiActionState::UpperPlatform?TEXT("SAFE NODE 5. Climb to KAKON 1 at NODE 7; LMB / X"):
        B->GetActionState()==EMinedakiActionState::ArmBridge?TEXT("ARM BRIDGE OPEN. Cross to KAKON 2 at NODE 11; rest at NODE 8"):
        B->GetActionState()==EMinedakiActionState::FinalRoute?TEXT("FINAL HEAD ROUTE OPEN. Rest at NODE 12, then KAKON 3 at NODE 14"):
        P->IsMounted()?TEXT("W / LS up: climb to the back. Hold E / RB when the mountain moves"):
        TEXT("Approach the green LEFT LEG marker, then E / RB: Grab"),FLinearColor(1,.8,.25));
    Line(TEXT("WASD / LS: move & climb | Mouse / RS: camera | Space / A: detach | R / Y: retry"));
    Line(FString::Printf(TEXT("NEXT ROUTE <= NODE %d | HEIGHT %.1fm | Cling %.1fs | Shake %d | Falls %d | Recovery %d"),B->GetMaximumRouteNode()+1,P->GetActorLocation().Z/100,B->Telemetry.ClingSeconds,B->Telemetry.Shakes,B->Telemetry.Falls,B->Telemetry.RecoverySuccesses));
}
