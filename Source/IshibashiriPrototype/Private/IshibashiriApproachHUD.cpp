#include "IshibashiriApproachHUD.h"
#include "IshibashiriApproachGameMode.h"
#include "IshibashiriApproachArena.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void AIshibashiriApproachHUD::DrawHUD()
{
    Super::DrawHUD();
    const auto* M = GetWorld()->GetAuthGameMode<AIshibashiriApproachGameMode>();
    const auto* A = M ? M->GetApproachArena() : nullptr;
    if (!A || !Canvas || !GEngine) return;
    const FString S = A->GetSubtitle();
    if (S.IsEmpty()) return;

    const float Scale = FMath::Clamp(Canvas->ClipY / 900.f, .72f, 1.35f);
    const float SafeMargin = 24.f * Scale;
    const float AvailableWidth = FMath::Max(1.f, Canvas->ClipX - SafeMargin * 2.f);
    float TextWidth = 0.f;
    float TextHeight = 0.f;
    GetTextSize(S, TextWidth, TextHeight, GEngine->GetMediumFont(), 1.f);
    const float TextScale = FMath::Min(1.25f * Scale, AvailableWidth / FMath::Max(1.f, TextWidth));
    TextWidth *= TextScale;
    TextHeight *= TextScale;

    // Size and center from the actual glyph bounds instead of a resolution-specific offset.
    // The restrained panel preserves the scene while keeping the warning legible over foliage.
    const float PanelWidth = FMath::Min(TextWidth + 52.f * Scale, AvailableWidth);
    const float PanelHeight = TextHeight + 28.f * Scale;
    const float PanelX = (Canvas->ClipX - PanelWidth) * .5f;
    const float PanelY = Canvas->ClipY * .76f;
    const float TextX = FMath::Clamp((Canvas->ClipX - TextWidth) * .5f, SafeMargin, FMath::Max(SafeMargin, Canvas->ClipX - SafeMargin - TextWidth));
    const float TextY = PanelY + (PanelHeight - TextHeight) * .5f;

    DrawRect(FLinearColor(.01f, .015f, .018f, .82f), PanelX, PanelY, PanelWidth, PanelHeight);
    DrawLine(PanelX, PanelY, PanelX + PanelWidth, PanelY, FLinearColor(.42f, .72f, .68f, .8f), 1.5f * Scale);
    DrawText(S, FLinearColor(.86f, .82f, .7f), TextX, TextY, GEngine->GetMediumFont(), TextScale, false);
}
