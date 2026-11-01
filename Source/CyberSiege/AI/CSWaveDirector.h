#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CSWaveDirector.generated.h"

class ACSEnemy;
class ACSSpawnPoint;

USTRUCT(BlueprintType)
struct FCSWave
{
    GENERATED_BODY()
    /** Mission event that starts the wave ("wave1", "hold_start", ...). */
    UPROPERTY(EditAnywhere) FName StartEvent;
    /** Approaches to use (spawn-point tags). Empty = all. */
    UPROPERTY(EditAnywhere) TArray<FName> Approaches;
    /** Add the approach the squad is covering least (Wave 2: "they're splitting us"). */
    UPROPERTY(EditAnywhere) bool bAddWeakestApproach = false;
    UPROPERTY(EditAnywhere) int32 Riflemen = 4;
    UPROPERTY(EditAnywhere) int32 Heavies = 0;
    UPROPERTY(EditAnywhere) int32 Gunners = 0;
    UPROPERTY(EditAnywhere) float SecondsBetweenSpawns = 1.2f;
    /** Where the wave pushes (the cabin door, the container). Actor tag; empty = hunt targets. */
    UPROPERTY(EditAnywhere) FName PushTowardTag;
    /** Fired when every enemy of this wave is dead. */
    UPROPERTY(EditAnywhere) FName ClearedEvent;
    /** Fired when the wave's enemies reach PushToward within this radius (the fail-state door). */
    UPROPERTY(EditAnywhere) FName ReachedEvent;
    UPROPERTY(EditAnywhere) float ReachedRadius = 250.f;
};

/**
 * Spawns waves on mission events, from tagged spawn points grouped into approaches, and reports back:
 * wave cleared, approach left open, an enemy reached the thing it was pushing on. Also hosts the ambient
 * "trickle" used outside the hold (the block's floors), one enemy group per event.
 */
UCLASS()
class CYBERSIEGE_API ACSWaveDirector : public AActor
{
    GENERATED_BODY()
public:
    ACSWaveDirector();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, Category="Waves") TArray<FCSWave> Waves;
    UPROPERTY(EditAnywhere, Category="Waves") TSubclassOf<ACSEnemy> EnemyClass;
    /** An approach nobody in the squad is within this distance of counts as open. */
    UPROPERTY(EditAnywhere, Category="Waves") float CoverDistance = 900.f;
    /** Stop everything (the mission ended or failed). */
    UFUNCTION(BlueprintCallable, Category="Waves") void StopAll();

    int32 AliveCount() const;
    int32 ActiveWave() const { return CurrentWave; }

private:
    UFUNCTION() void OnMissionEvent(FName EventName);
    int32 CurrentWave = -1;
    int32 LeftToSpawn = 0;
    TArray<TWeakObjectPtr<ACSEnemy>> WaveEnemies;
    TArray<ACSSpawnPoint*> WavePoints;
    FVector PushPoint = FVector::ZeroVector; bool bHasPush = false;
    float NextSpawnAt = 0.f;
    bool bWaveActive = false;
    TMap<FName, float> OpenSince;
    void SpawnOne();
    FName WeakestApproach(const TArray<FName>& Exclude) const;
    void CollectPoints(const TArray<FName>& Approaches);
};

/** Where enemies come from. Approach groups several points ("north", "east", "south", "roof2" ...). */
UCLASS()
class CYBERSIEGE_API ACSSpawnPoint : public AActor
{
    GENERATED_BODY()
public:
    ACSSpawnPoint() { PrimaryActorTick.bCanEverTick = false; RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root")); }
    UPROPERTY(EditAnywhere, Category="Spawn") FName Approach = TEXT("north");
};
