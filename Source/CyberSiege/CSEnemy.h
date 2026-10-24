#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CSEnemy.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/** Pursues the player on foot and hits on contact. No NavMesh needed: the arena is open. */
UCLASS()
class CYBERSIEGE_API ACSEnemy : public ACharacter
{
    GENERATED_BODY()
public:
    ACSEnemy();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                             AController* EventInstigator, AActor* DamageCauser) override;

    UPROPERTY(EditAnywhere, Category="Combat") float MaxHealth = 60.f;
    UPROPERTY(EditAnywhere, Category="Combat") float ContactDamage = 12.f;
    UPROPERTY(EditAnywhere, Category="Combat") float ContactRange = 170.f;
    UPROPERTY(EditAnywhere, Category="Combat") float ContactCooldown = 0.7f;
    UPROPERTY(EditAnywhere, Category="Combat") int32 ScoreValue = 100;
    UPROPERTY(EditAnywhere, Category="Movement") float MoveSpeed = 420.f;

protected:
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Body;
    UPROPERTY() UMaterialInstanceDynamic* BodyMID = nullptr;

    float Health = 60.f;
    double NextContactTime = 0.0;
    FTimerHandle FlashTimer;
    void EndFlash();
};
