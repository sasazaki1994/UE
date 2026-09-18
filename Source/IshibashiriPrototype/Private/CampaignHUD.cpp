#include "CampaignHUD.h"
#include "CampaignGameInstance.h"
#include "CampaignGameMode.h"
#include "Engine/Canvas.h"

namespace
{
    void DrawWrappedCardText(UCanvas* Canvas, const FString& Text, UFont* Font, float X, float Y, float MaxWidth, float Scale)
    {
        FString Line;
        float LineHeight = 0.f;
        Canvas->StrLen(Font, TEXT("白面"), MaxWidth, LineHeight);
        for (const TCHAR Character : Text)
        {
            const FString Candidate = Line + Character;
            float Width = 0.f;
            float Height = 0.f;
            Canvas->StrLen(Font, *Candidate, Width, Height);
            if (!Line.IsEmpty() && Width * Scale > MaxWidth)
            {
                Canvas->DrawText(Font, Line, X, Y, Scale, Scale);
                Y += LineHeight * Scale * 1.35f;
                Line.Reset();
            }
            Line.AppendChar(Character);
        }
        if (!Line.IsEmpty()) Canvas->DrawText(Font, Line, X, Y, Scale, Scale);
    }
}

void ACampaignHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas) return;
    auto* C = GetGameInstance<UCampaignGameInstance>();
    auto* M = GetWorld()->GetAuthGameMode<ACampaignGameMode>();
    if (!C || !M) return;
    const int32 I = M->GetCardIndex();
    FString Heading, Body;
    switch (C->GetCampaignState())
    {
    case ECampaignState::Title:
        Heading = TEXT("禍祓い");
        Body = TEXT("START");
        break;
    case ECampaignState::Prologue:
    {
        const TCHAR* T[] = {TEXT("白い面は、禍祓いのしるし。"), TEXT("境界石が砕け、白面の手と顔にも禍が残った。"),
            TEXT("自らの穢れを祓う手掛かりを求め、主のもとへ。"), TEXT("主を討つな。宿った禍だけを祓え。")};
        Heading = TEXT("PROLOGUE");
        Body = T[FMath::Clamp(I, 0, 3)];
        break;
    }
    case ECampaignState::Interlude1:
    {
        const TCHAR* T[] = {TEXT("石走りの息が、ゆっくりと戻る。"), TEXT("水の底で、同じ脈動が続いている。"), TEXT("第二の主　淵纏い")};
        Heading = TEXT("INTERLUDE");
        Body = T[FMath::Clamp(I, 0, 2)];
        break;
    }
    case ECampaignState::Interlude2:
    {
        const TCHAR* T[] = {TEXT("腕の痕が広がり、顔の痕も深くなっていた。"), TEXT("第三の主　峰抱き")};
        Heading = TEXT("INTERLUDE");
        Body = T[FMath::Clamp(I, 0, 1)];
        break;
    }
    case ECampaignState::Interlude3:
    {
        const TCHAR* T[] = {TEXT("三柱は鎮まった。顔と腕の痕は消えない。"), TEXT("禍の流れは、島の奥へ続いている。"), TEXT("禍津根")};
        Heading = TEXT("INTERLUDE");
        Body = T[FMath::Clamp(I, 0, 2)];
        break;
    }
    case ECampaignState::Ending:
    {
        const TCHAR* T[] = {TEXT("地の脈動が静まり、主たちの息が戻る。"), TEXT("面を外す。顔と腕の痕は、まだそこにある。"),
            TEXT("白面は、再び面を着けた。"), TEXT("禍がまた生じても、ここで向き合う。")};
        Heading = TEXT("ENDING");
        Body = T[FMath::Clamp(I, 0, 3)];
        break;
    }
    case ECampaignState::Completed:
        Heading = TEXT("禍祓い");
        Body = TEXT("START - RESTART");
        break;
    default: return;
    }
    DrawRect(FLinearColor::Black, 0, 0, Canvas->ClipX, Canvas->ClipY);
    DrawText(Heading, FLinearColor::White, Canvas->ClipX * .42f, Canvas->ClipY * .34f, GEngine->GetLargeFont(), 1.4f);
    DrawWrappedCardText(Canvas, Body, GEngine->GetMediumFont(), Canvas->ClipX * .20f, Canvas->ClipY * .50f, Canvas->ClipX * .60f, 1.05f);
    DrawText(
        TEXT("SPACE / X : CONTINUE"), FLinearColor(.55f, .65f, .7f), Canvas->ClipX * .39f, Canvas->ClipY * .78f, GEngine->GetSmallFont());
}
