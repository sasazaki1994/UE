#include "CampaignHUD.h"
#include "CampaignGameInstance.h"
#include "CampaignGameMode.h"
#include "CampaignTextLayout.h"
#include "Engine/Canvas.h"

namespace
{
    void DrawWrappedCardText(UCanvas* Canvas, const FString& Text, UFont* Font, float X, float Y, float MaxWidth, float Scale)
    {
        if (!Canvas || !Font || Text.IsEmpty()) return;
        const float DisplayWidth = FMath::Max(1.f, MaxWidth);
        float SampleWidth = 0.f;
        float LineHeight = 0.f;
        Canvas->StrLen(Font, TEXT("白面"), SampleWidth, LineHeight);
        const TArray<FString> Lines = CampaignTextLayout::Wrap(Text, DisplayWidth, Scale,
            [Canvas, Font](const FString& Candidate)
            {
                float MeasuredWidth = 0.f, MeasuredHeight = 0.f;
                Canvas->StrLen(Font, Candidate, MeasuredWidth, MeasuredHeight);
                return MeasuredWidth;
            });
        for (const FString& Line : Lines)
        {
            Canvas->DrawText(Font, Line, X, Y, Scale, Scale);
            Y += LineHeight * Scale * 1.35f;
        }
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
        Body = M->IsNewGameConfirmationPending()
            ? TEXT("新しく始めると前回の続きが上書きされます。\nもう一度 NEW GAME を入力してください。")
            : TEXT("NEW GAME");
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
        const TCHAR* T[] = {TEXT("石走りは生きている。息が戻る。"), TEXT("水の底で、同じ脈動が続いている。"), TEXT("第二の主　淵纏い")};
        Heading = TEXT("INTERLUDE");
        Body = T[FMath::Clamp(I, 0, 2)];
        break;
    }
    case ECampaignState::Interlude2:
    {
        const TCHAR* T[] = {TEXT("腕と顔に、白面自身の穢れが深く残る。"), TEXT("第三の主　峰抱き")};
        Heading = TEXT("INTERLUDE");
        Body = T[FMath::Clamp(I, 0, 1)];
        break;
    }
    case ECampaignState::Interlude3:
    {
        const TCHAR* T[] = {TEXT("三柱は生きて鎮まる。白面の穢れは残る。"), TEXT("島の奥で主ならぬ禍津根が脈打つ。"), TEXT("禍津根")};
        Heading = TEXT("INTERLUDE");
        Body = T[FMath::Clamp(I, 0, 2)];
        break;
    }
    case ECampaignState::Ending:
    {
        const TCHAR* T[] = {TEXT("地は静まり、三柱の主は生きている。"), TEXT("面を外す。白面自身の穢れは、まだ残る。"),
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
    const float BodyTop = Canvas->ClipY * .50f;
    const float ContinueTop = Canvas->ClipY * .78f;
    // The width remains viewport-relative; the vertical budget reserves the
    // continue prompt even on narrow screens.
    DrawWrappedCardText(Canvas, Body, GEngine->GetMediumFont(), Canvas->ClipX * .20f, BodyTop, Canvas->ClipX * .60f,
        FMath::Min(1.05f, FMath::Max(.65f, (ContinueTop - BodyTop) / 180.f)));
    const bool bTitle = C->GetCampaignState() == ECampaignState::Title;
    DrawText(bTitle ? TEXT("SPACE / X : NEW GAME") : TEXT("SPACE / X : CONTINUE"), FLinearColor(.55f, .65f, .7f), Canvas->ClipX * .39f,
        Canvas->ClipY * .78f, GEngine->GetSmallFont());
    if (bTitle && C->HasContinue())
    {
        const TCHAR* Chapter = TEXT("PROLOGUE");
        switch (C->GetContinueChapter())
        {
        case ECampaignState::IshibashiriApproach: Chapter = TEXT("APPROACH"); break;
        case ECampaignState::Ishibashiri: Chapter = TEXT("ISHIBASHIRI"); break;
        case ECampaignState::Interlude1: Chapter = TEXT("INTERLUDE 1"); break;
        case ECampaignState::Fuchimatoi: Chapter = TEXT("FUCHIMATOI"); break;
        case ECampaignState::Interlude2: Chapter = TEXT("INTERLUDE 2"); break;
        case ECampaignState::Minedaki: Chapter = TEXT("MINEDAKI"); break;
        case ECampaignState::Interlude3: Chapter = TEXT("INTERLUDE 3"); break;
        case ECampaignState::Magatsune: Chapter = TEXT("MAGATSUNE"); break;
        case ECampaignState::Ending: Chapter = TEXT("ENDING"); break;
        default: break;
        }
        DrawText(FString::Printf(TEXT("R / Y : CONTINUE (%s)"), Chapter), FLinearColor(.75f, .68f, .46f), Canvas->ClipX * .39f,
            Canvas->ClipY * .84f, GEngine->GetSmallFont());
        if (M->IsNewGameConfirmationPending())
        {
            DrawText(TEXT("ESC / B : CANCEL"), FLinearColor(.65f, .7f, .72f), Canvas->ClipX * .39f, Canvas->ClipY * .89f,
                GEngine->GetSmallFont());
        }
    }
}
