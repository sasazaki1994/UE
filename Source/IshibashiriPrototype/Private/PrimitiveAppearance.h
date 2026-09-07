#pragma once

#include "Materials/MaterialInstanceDynamic.h"

// BasicShapeMaterial has shipped with both spellings across engine content versions.
inline void SetPrimitiveColor(UMaterialInstanceDynamic* Material, const FLinearColor& Color)
{
    if (!Material) return;
    Material->SetVectorParameterValue(TEXT("Color"), Color);
    Material->SetVectorParameterValue(TEXT("Colour"), Color);
}
