#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KakonProgressTestListener.generated.h"

class AKakonActor;

UCLASS()
class UKakonProgressTestListener : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION()
    void HandlePurified(AKakonActor* Kakon)
    {
        (void)Kakon;
        ++PurifiedEventCount;
    }

    UFUNCTION()
    void HandleAllPurified() { ++AllPurifiedEventCount; }

    int32 PurifiedEventCount = 0;
    int32 AllPurifiedEventCount = 0;
};
