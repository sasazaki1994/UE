#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MinedakiArena.generated.h"
UCLASS()
class ISHIBASHIRIPROTOTYPE_API AMinedakiArena : public AActor
{
    GENERATED_BODY()
public:
    AMinedakiArena();
    virtual void BeginPlay() override;
};
