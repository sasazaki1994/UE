#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NushiProgressComponent.generated.h"

class AKakonActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAllKakonPurifiedSignature);

// Tracks the Kakon purification progress for one Nushi without owning game flow.
UCLASS(ClassGroup=(Kakon), meta=(BlueprintSpawnableComponent))
class ISHIBASHIRIPROTOTYPE_API UNushiProgressComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNushiProgressComponent();

    UFUNCTION(BlueprintCallable, Category="Kakon")
    void RegisterKakon(AKakonActor* Kakon);

    // Starts a new cycle and resets every currently registered Kakon.
    UFUNCTION(BlueprintCallable, Category="Kakon")
    void ResetProgress();

    UFUNCTION(BlueprintPure, Category="Kakon")
    int32 GetRegisteredKakonCount() const { return RegisteredKakon.Num(); }

    UFUNCTION(BlueprintPure, Category="Kakon")
    int32 GetPurifiedCount() const { return PurifiedKakon.Num(); }

    UFUNCTION(BlueprintPure, Category="Kakon")
    bool IsAllPurified() const { return bAllPurified; }

    UPROPERTY(BlueprintAssignable, Category="Kakon")
    FAllKakonPurifiedSignature OnAllPurified;

private:
    UFUNCTION()
    void HandleKakonPurified(AKakonActor* Kakon);

    UPROPERTY()
    TSet<TObjectPtr<AKakonActor>> RegisteredKakon;

    UPROPERTY()
    TSet<TObjectPtr<AKakonActor>> PurifiedKakon;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Kakon", meta=(AllowPrivateAccess="true"))
    bool bAllPurified = false;
};
