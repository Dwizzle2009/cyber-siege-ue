#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CSEnemy.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class ACSCoverPoint;

UENUM(BlueprintType)
enum class ECSEnemyType : uint8
{
    Rifleman,   // moves cover to cover toward the squad, shoots in bursts
    Heavy,      // shield in front: frontal hits do 15%; slow; walks straight at the nearest target; melee
    Gunner,     // stays put at its spawn (the mounted / light machine gun); long bursts, big spread
};

DECLARE_MULTICAST_DELEGATE_OneParam(FCSEnemyDied, class ACSEnemy*);

/**
 * A regular. Hitscan rifle, cover-seeking, targets the player, the squad and rescuees. Body is a placeholder
 * until the soldier mesh lands; all gameplay reads the capsule and the eye point.
 */
UCLASS()
class CYBERSIEGE_API ACSEnemy : public ACharacter
{
    GENERATED_BODY()
public:
    ACSEnemy();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

    UPROPERTY(EditAnywhere, Category="Enemy") ECSEnemyType Type = ECSEnemyType::Rifleman;
    UPROPERTY(EditAnywhere, Category="Enemy") float MaxHealth = 90.f;
    UPROPERTY(EditAnywhere, Category="Enemy") float MoveSpeed = 440.f;
    UPROPERTY(EditAnywhere, Category="Enemy") float EngageRange = 3000.f;
    UPROPERTY(EditAnywhere, Category="Enemy") float PreferredRange = 1400.f;   // riflemen stop advancing here
    UPROPERTY(EditAnywhere, Category="Enemy") float ShotDamage = 9.f;
    UPROPERTY(EditAnywhere, Category="Enemy") float SpreadDegrees = 3.5f;
    UPROPERTY(EditAnywhere, Category="Enemy") int32 BurstCount = 3;
    UPROPERTY(EditAnywhere, Category="Enemy") float BurstInterval = 0.11f;
    UPROPERTY(EditAnywhere, Category="Enemy") float BurstPause = 1.3f;
    UPROPERTY(EditAnywhere, Category="Enemy") float MeleeDamage = 30.f;
    UPROPERTY(EditAnywhere, Category="Enemy") int32 ScoreValue = 100;
    /** Which approach this enemy came from (the wave director's tag); the HUD lens groups by it. */
    UPROPERTY(BlueprintReadOnly, Category="Enemy") FName Approach;

    bool IsDead() const { return bDead; }
    FVector EyeLocation() const { return GetActorLocation() + FVector(0, 0, 62.f); }
    FCSEnemyDied OnDied;

    /** Point this enemy toward something to push on (the wreck's cabin, the container): overrides target-seeking for movement. */
    void SetObjective(const FVector& Where) { Objective = Where; bHasObjective = true; }

protected:
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Body;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Shield;   // heavy only
    UPROPERTY() UMaterialInstanceDynamic* BodyMID = nullptr;

    float Health = 90.f;
    bool bDead = false;
    TWeakObjectPtr<AActor> Target;
    double NextTargetScan = 0.0;
    double NextShot = 0.0;
    int32 BurstLeft = 0;
    double NextRepath = 0.0;
    TWeakObjectPtr<ACSCoverPoint> Cover;
    double CoverUntil = 0.0;
    FVector Objective = FVector::ZeroVector; bool bHasObjective = false;
    FVector SpawnLocation;
    FTimerHandle FlashTimer;

    AActor* PickTarget();
    bool CanSee(const AActor* A) const;
    void MoveTowards(const FVector& Where, float Accept);
    void FaceTowards(const FVector& Where, float DeltaSeconds);
    void Shoot(AActor* At);
    void TickRifleman(float Dt, AActor* T);
    void TickHeavy(float Dt, AActor* T);
    void TickGunner(float Dt, AActor* T);
    void Die(AController* Killer);
    void EndFlash();
};
