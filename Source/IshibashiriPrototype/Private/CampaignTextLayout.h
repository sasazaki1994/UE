#pragma once

#include "CoreMinimal.h"

namespace CampaignTextLayout
{
    // Returns display lines without changing the caller's layout width. Measure
    // reports the unscaled width of a candidate string.
    inline TArray<FString> Wrap(const FString& Text, float DisplayWidth, float Scale, const TFunctionRef<float(const FString&)>& Measure)
    {
        TArray<FString> Lines;
        if (Text.IsEmpty()) return Lines;

        const float UnscaledLimit = FMath::Max(1.f, DisplayWidth) / FMath::Max(Scale, KINDA_SMALL_NUMBER);
        FString Line;
        for (const TCHAR Character : Text)
        {
            if (Character == TEXT('\n'))
            {
                Lines.Add(Line);
                Line.Reset();
                continue;
            }
            const FString Candidate = Line + Character;
            if (!Line.IsEmpty() && Measure(Candidate) > UnscaledLimit)
            {
                Lines.Add(Line);
                Line.Reset();
            }
            Line.AppendChar(Character);
        }
        if (!Line.IsEmpty()) Lines.Add(Line);
        return Lines;
    }
}
