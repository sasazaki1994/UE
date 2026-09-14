#include "CampaignHUD.h"
#include "CampaignGameInstance.h"
#include "CampaignGameMode.h"
#include "Engine/Canvas.h"

void ACampaignHUD::DrawHUD()
{
    Super::DrawHUD(); if(!Canvas)return;
    auto* C=GetGameInstance<UCampaignGameInstance>(); auto* M=GetWorld()->GetAuthGameMode<ACampaignGameMode>();
    if(!C||!M)return;
    const int32 I=M->GetCardIndex(); FString Heading,Body;
    switch(C->GetCampaignState())
    {
    case ECampaignState::Title: Heading=TEXT("禍祓い");Body=TEXT("START");break;
    case ECampaignState::Prologue:{const TCHAR* T[]={TEXT("白面の祓い手。"),TEXT("境界石は砕かれ、禍が流れ出した。"),TEXT("禍は祓い手の左腕にも宿っている。"),TEXT("主に宿った禍を、境断ちで鎮めよ。")};Heading=TEXT("PROLOGUE");Body=T[FMath::Clamp(I,0,3)];break;}
    case ECampaignState::Interlude1:{const TCHAR* T[]={TEXT("石走りは静まった。"),TEXT("水の底でも、同じ脈動が続いている。"),TEXT("第二の主　淵纏い")};Heading=TEXT("INTERLUDE");Body=T[FMath::Clamp(I,0,2)];break;}
    case ECampaignState::Interlude2:{const TCHAR* T[]={TEXT("山の奥へ進むほど、左腕の痛みは強くなる。"),TEXT("第三の主　峰抱き")};Heading=TEXT("INTERLUDE");Body=T[FMath::Clamp(I,0,1)];break;}
    case ECampaignState::Interlude3:{const TCHAR* T[]={TEXT("三柱を鎮めても、禍は消えなかった。"),TEXT("流れはすべて、一つの場所へ向かっている。"),TEXT("禍津根")};Heading=TEXT("INTERLUDE");Body=T[FMath::Clamp(I,0,2)];break;}
    case ECampaignState::Ending:{const TCHAR* T[]={TEXT("禍津根は破壊されず、ただ鎮まった。"),TEXT("主たちは生きている。"),TEXT("左腕の禍も、まだ完全には消えていない。"),TEXT("禍は自然とともに、再び生じるかもしれない。")};Heading=TEXT("ENDING");Body=T[FMath::Clamp(I,0,3)];break;}
    case ECampaignState::Completed: Heading=TEXT("禍祓い");Body=TEXT("START - RESTART");break;
    default:return;
    }
    DrawRect(FLinearColor::Black,0,0,Canvas->ClipX,Canvas->ClipY);
    DrawText(Heading,FLinearColor::White,Canvas->ClipX*.42f,Canvas->ClipY*.34f,GEngine->GetLargeFont(),1.4f);
    DrawText(Body,FLinearColor(.85f,.85f,.85f),Canvas->ClipX*.20f,Canvas->ClipY*.50f,GEngine->GetMediumFont(),1.05f);
    DrawText(TEXT("SPACE / X : CONTINUE"),FLinearColor(.55f,.65f,.7f),Canvas->ClipX*.39f,Canvas->ClipY*.78f,GEngine->GetSmallFont());
}
