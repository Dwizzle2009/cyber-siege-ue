#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CSGameMode.generated.h"

class ACSEnemy;

UCLASS()
class CYBERSIEGE_API ACSGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ACSGameMode();
    virtual void BeginPlay() override;

    void RegisterEnemyKill(AActor* Enemy, int32 Points);
    void OnPlayerDied();
    void RequestRestart();

    // read by the HUD
    int32 Wave = 0;
    int32 Score = 0;
    int32 Kills = 0;
    bool  bGameOver = false;
    FString StatusText;

    UPROPERTY(EditAnywhere, Category="Waves") TSubclassOf<ACSEnemy> EnemyClass;
    UPROPERTY(EditAnywhere, Category="Waves") float SpawnRadius = 2200.f;
    UPROPERTY(EditAnywhere, Category="Waves") float TimeBetweenSpawns = 0.85f;

private:
    int32 EnemiesRemainingToSpawn = 0;
    int32 EnemiesAlive = 0;
    FTimerHandle SpawnTimer, NextWaveTimer, StatusTimer;

    void StartWave();
    void SpawnOneEnemy();
    void ClearStatus();
};
