#include "IshibashiriApproachHUD.h"
#include "IshibashiriApproachGameMode.h"
#include "IshibashiriApproachArena.h"
#include "Engine/Canvas.h"
void AIshibashiriApproachHUD::DrawHUD(){Super::DrawHUD();const auto* M=GetWorld()->GetAuthGameMode<AIshibashiriApproachGameMode>();const auto* A=M?M->GetApproachArena():nullptr;if(!A)return;const FString S=A->GetSubtitle();if(!S.IsEmpty())DrawText(S,FLinearColor(.78f,.73f,.62f),Canvas->SizeX*.5f-110,Canvas->SizeY*.78f,nullptr,1.25f);}
