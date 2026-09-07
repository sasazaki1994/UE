#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PrototypeHUD.generated.h"

UCLASS()
class ISHIBASHIRIPROTOTYPE_API APrototypeHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
};
