#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PrototypeGameMode.generated.h"

class APrototypePlayer;
class AIshibashiriBoss;
class UStaticMesh;
class UMaterialInterface;
class ABasinPrototypeArena;

UENUM()
enum class EEncounterResult : uint8 { Playing, Victory, Defeat };

UCLASS()
class ISHIBASHIRIPROTOTYPE_API APrototypeGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    APrototypeGameMode();
    virtual void StartPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    void RetryEncounter();
    void FinishEncounter(bool bVictory);
    bool IsEncounterActive() const { return Result == EEncounterResult::Playing; }
    EEncounterResult GetResult() const { return Result; }
    APrototypePlayer* GetPlayer() const { return Player; }
    AIshibashiriBoss* GetBoss() const { return Boss; }
    bool IsBasinPrototype() const { return bBasinPrototype; }
    ABasinPrototypeArena* GetBasinArena() const { return BasinArena; }

    UPROPERTY(EditAnywhere, Category="Arena", meta=(ClampMin="1200")) float ArenaHalfExtent = 4000.f;

private:
    void CreateArena();
    void CreateBlock(const FString& Name, const FVector& Position, const FVector& Scale, const FLinearColor& Color);
    FTransform PlayerSpawn() const;
    FTransform BossSpawn() const;

    UPROPERTY() TObjectPtr<APrototypePlayer> Player;
    UPROPERTY() TObjectPtr<AIshibashiriBoss> Boss;
    UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
    UPROPERTY() TObjectPtr<UMaterialInterface> BaseMaterial;
    UPROPERTY() TObjectPtr<ABasinPrototypeArena> BasinArena;
    bool bBasinPrototype = false;
    EEncounterResult Result = EEncounterResult::Playing;
};
