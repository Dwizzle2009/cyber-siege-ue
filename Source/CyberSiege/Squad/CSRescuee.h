#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CSRescuee.generated.h"

class UStaticMeshComponent;
class ACSEnemy;

UENUM(BlueprintType)
enum class ECSRescueeState : uint8
{
    Trapped,    // can't move at all (Reyes under the panel)
    Waiting,    // where they are, until told to follow
    Following,  // escorted: follows Leader at WalkSpeed (slow); exposed to fire
    Carried,    // on someone's back: follows the carrier closely; exposed
    Sheltered,  // inside a safe spot (the container): not targeted
    Aboard,     // on the extraction; done
};

/**
 * Someone the squad came for. Three in Mission 1: Reyes (trapped, then walks slowly), Okafor (carried),
 * Halvorsen (walks and shoots one-handed). If one dies the mission fails: "the rescue is a person."
 */
UCLASS()
class CYBERSIEGE_API ACSRescuee : public ACharacter
{
    GENERATED_BODY()
public:
    ACSRescuee();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

    UPROPERTY(EditAnywhere, Category="Rescuee") FName Id = TEXT("reyes");
    UPROPERTY(EditAnywhere, Category="Rescuee") FText DisplayName;
    UPROPERTY(EditAnywhere, Category="Rescuee") ECSRescueeState State = ECSRescueeState::Waiting;
    UPROPERTY(EditAnywhere, Category="Rescuee") float MaxHealth = 120.f;
    UPROPERTY(EditAnywhere, Category="Rescuee") float WalkSpeed = 260.f;
    UPROPERTY(EditAnywhere, Category="Rescuee") bool  bCanShoot = false;     // Halvorsen
    UPROPERTY(EditAnywhere, Category="Rescuee") float ShotDamage = 12.f;
    UPROPERTY(EditAnywhere, Category="Rescuee") float ShotInterval = 0.9f;
    UPROPERTY(EditAnywhere, Category="Rescuee") float EngageRange = 1800.f;
    /** Fired on the mission when this person dies (the fail reason is built from DisplayName). */
    UPROPERTY(EditAnywhere, Category="Rescuee") bool bFailMissionOnDeath = true;

    UFUNCTION(BlueprintCallable, Category="Rescuee") void SetState(ECSRescueeState NewState);
    UFUNCTION(BlueprintCallable, Category="Rescuee") void Follow(AActor* NewLeader);
    UFUNCTION(BlueprintCallable, Category="Rescuee") void CarriedBy(AActor* Carrier);
    UFUNCTION(BlueprintCallable, Category="Rescuee") void ShelterAt(const FVector& Where);
    UFUNCTION(BlueprintPure,     Category="Rescuee") bool IsExposed() const { return (State == ECSRescueeState::Following || State == ECSRescueeState::Carried) && Health > 0.f; }
    float GetHealthNormalized() const { return MaxHealth > 0 ? Health / MaxHealth : 0.f; }

protected:
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Body;
    TWeakObjectPtr<AActor> Leader;
    FVector ShelterPoint = FVector::ZeroVector;
    float Health = 120.f;
    double NextRepath = 0.0, NextShot = 0.0;
    void MoveTowards(const FVector& Where, float Accept);
};
