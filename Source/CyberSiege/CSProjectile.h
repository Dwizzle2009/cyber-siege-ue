#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CSProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

UCLASS()
class CYBERSIEGE_API ACSProjectile : public AActor
{
    GENERATED_BODY()
public:
    ACSProjectile();

    UPROPERTY(EditAnywhere, Category="Combat") float Damage = 20.f;
    UPROPERTY(EditAnywhere, Category="Combat") float Speed = 7000.f;

protected:
    UPROPERTY(VisibleAnywhere) USphereComponent* Collision;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Visual;
    UPROPERTY(VisibleAnywhere) UProjectileMovementComponent* Movement;

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
               FVector NormalImpulse, const FHitResult& Hit);
};
