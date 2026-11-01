#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CSSquadMate.generated.h"

class UStaticMeshComponent;
class AAIController;
class ACSBreachWall;
class ACSEnemy;

UENUM(BlueprintType)
enum class ECSSquadOrder : uint8
{
    Follow,     // stay near the player, at a slot offset
    Hold,       // stand at a point, face the threat, fire
    MoveTo,     // go to a point, then hold there
    Breach,     // go to a wall, set the charge, blow it, then follow
    Carry,      // escort a rescuee: walk beside them at their pace
    Scripted,   // the mission has them (cabin work, base scene): no AI movement
    Down,       // bleeding out, waiting for a revive
};

/**
 * One of the three. Follows the player, takes the three commands, shoots what it can see, goes down
 * and gets picked up. Mesh is a placeholder capsule+cube until the MetaHuman lands; all gameplay reads
 * the capsule, so swapping the body changes nothing here.
 */
UCLASS()
class CYBERSIEGE_API ACSSquadMate : public ACharacter
{
    GENERATED_BODY()
public:
    ACSSquadMate();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

    /** bell / chen / mercer. Matches the dialogue Speaker column. */
    UPROPERTY(EditAnywhere, Category="Squad") FName Callsign = TEXT("bell");
    UPROPERTY(EditAnywhere, Category="Squad") FText DisplayName;
    /** Position in the follow formation, relative to the player's facing (cm). */
    UPROPERTY(EditAnywhere, Category="Squad") FVector FollowOffset = FVector(-220.f, 160.f, 0.f);
    UPROPERTY(EditAnywhere, Category="Squad") FLinearColor TeamColor = FLinearColor(0.2f, 0.6f, 1.0f);

    UPROPERTY(EditAnywhere, Category="Combat") float MaxHealth = 150.f;
    UPROPERTY(EditAnywhere, Category="Combat") float EngageRange = 3200.f;
    UPROPERTY(EditAnywhere, Category="Combat") float ShotInterval = 0.32f;
    UPROPERTY(EditAnywhere, Category="Combat") float ShotDamage = 14.f;
    UPROPERTY(EditAnywhere, Category="Combat") float HitChance = 0.55f;
    UPROPERTY(EditAnywhere, Category="Combat") float BleedOutSeconds = 45.f;
    UPROPERTY(EditAnywhere, Category="Combat") bool  bInvulnerable = false;   // the slice can't lose the squad in Mission 1

    // ---- orders ----
    UFUNCTION(BlueprintCallable, Category="Squad") void OrderFollow();
    UFUNCTION(BlueprintCallable, Category="Squad") void OrderHold(const FVector& Where);
    UFUNCTION(BlueprintCallable, Category="Squad") void OrderMoveTo(const FVector& Where);
    UFUNCTION(BlueprintCallable, Category="Squad") void OrderBreach(ACSBreachWall* Wall);
    UFUNCTION(BlueprintCallable, Category="Squad") void OrderCarry(AActor* Rescuee);
    UFUNCTION(BlueprintCallable, Category="Squad") void OrderScripted(const FVector& StandAt, float FaceYaw);
    UFUNCTION(BlueprintCallable, Category="Squad") void Revive();
    UFUNCTION(BlueprintPure,     Category="Squad") ECSSquadOrder GetOrder() const { return Order; }
    UFUNCTION(BlueprintPure,     Category="Squad") bool IsDown() const { return Order == ECSSquadOrder::Down; }
    float GetHealthNormalized() const { return MaxHealth > 0 ? Health / MaxHealth : 0.f; }
    float BleedOutRemaining() const;

    /** Checkpoint resume: appear beside the player. */
    void TeleportBeside(const FVector& PlayerLoc, float PlayerYaw);

    /** The player (or a mission event) pressed revive on this mate. Takes ReviveSeconds of standing next to them. */
    UPROPERTY(EditAnywhere, Category="Combat") float ReviveSeconds = 3.f;
    void ReviveTick(float DeltaSeconds);

protected:
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Body;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Rifle;

    ECSSquadOrder Order = ECSSquadOrder::Follow;
    FVector OrderPoint = FVector::ZeroVector;
    float   OrderYaw = 0.f;
    TWeakObjectPtr<ACSBreachWall> BreachTarget;
    TWeakObjectPtr<AActor> CarryTarget;
    float Health = 150.f;
    float DownSince = 0.f;
    float ReviveProgress = 0.f;
    float BreachArmedAt = 0.f;
    double NextShotTime = 0.0;
    double NextRepathTime = 0.0;
    double LastBarkTime = 0.0;

    AAIController* AI() const;
    void MoveTowards(const FVector& Where, float AcceptRadius);
    ACSEnemy* PickTarget() const;
    void TryShoot(ACSEnemy* Target, float DeltaSeconds);
    void FaceTowards(const FVector& Where, float DeltaSeconds);
    void GoDown();
    void Bark(FName Trigger, float Cooldown = 6.f);
};
