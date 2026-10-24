#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "CSPlayerCharacter.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class USceneComponent;
class UInputAction;
class UInputMappingContext;
class ACSProjectile;

/**
 * First-person player. Every input is an Enhanced Input action built in code
 * (BuildInputMapping) so the project ships with no input assets and every
 * binding is remappable later from one place.
 */
UCLASS()
class CYBERSIEGE_API ACSPlayerCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    ACSPlayerCharacter();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                             AController* EventInstigator, AActor* DamageCauser) override;

    // ---- tuning ----
    UPROPERTY(EditAnywhere, Category="Combat") float MaxHealth = 100.f;
    UPROPERTY(EditAnywhere, Category="Combat") float FireInterval = 0.09f;
    UPROPERTY(EditAnywhere, Category="Combat") int32 MagazineSize = 30;
    UPROPERTY(EditAnywhere, Category="Combat") int32 ReserveAmmo = 150;
    UPROPERTY(EditAnywhere, Category="Combat") float ReloadTime = 1.6f;
    UPROPERTY(EditAnywhere, Category="Movement") float WalkSpeed = 620.f;
    UPROPERTY(EditAnywhere, Category="Movement") float SprintSpeed = 900.f;
    UPROPERTY(EditAnywhere, Category="Movement") float DodgeImpulse = 1400.f;
    UPROPERTY(EditAnywhere, Category="Movement") float DodgeCooldown = 1.1f;
    UPROPERTY(EditAnywhere, Category="Input")    float StickLookRate = 160.f;   // deg/sec at full deflection
    UPROPERTY(EditAnywhere, Category="Combat")   TSubclassOf<ACSProjectile> ProjectileClass;

    // ---- state read by the HUD ----
    float Health = 100.f;
    int32 AmmoInMag = 30;
    int32 AmmoReserve = 150;
    bool  bReloading = false;
    bool  bDead = false;
    float GetHealthNormalized() const { return MaxHealth > 0 ? Health / MaxHealth : 0.f; }

protected:
    UPROPERTY(VisibleAnywhere) UCameraComponent* FirstPersonCamera;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* PrototypeGun;   // replaced by the real rifle later
    UPROPERTY(VisibleAnywhere) USceneComponent* MuzzlePoint;

    // Enhanced Input objects, created in BuildInputMapping
    UPROPERTY() UInputMappingContext* Mapping = nullptr;
    UPROPERTY() UInputAction* IA_Move = nullptr;
    UPROPERTY() UInputAction* IA_LookMouse = nullptr;
    UPROPERTY() UInputAction* IA_LookStick = nullptr;
    UPROPERTY() UInputAction* IA_Fire = nullptr;
    UPROPERTY() UInputAction* IA_Reload = nullptr;
    UPROPERTY() UInputAction* IA_Jump = nullptr;
    UPROPERTY() UInputAction* IA_Sprint = nullptr;
    UPROPERTY() UInputAction* IA_Dodge = nullptr;

    void BuildInputMapping();

    void OnMove(const FInputActionValue& V);
    void OnLookMouse(const FInputActionValue& V);
    void OnLookStick(const FInputActionValue& V);
    void OnFire(const FInputActionValue& V);
    void OnReload(const FInputActionValue& V);
    void OnJumpStart(const FInputActionValue& V);
    void OnJumpStop(const FInputActionValue& V);
    void OnSprintStart(const FInputActionValue& V);
    void OnSprintStop(const FInputActionValue& V);
    void OnDodge(const FInputActionValue& V);

    void FinishReload();
    void Die();

    double LastFireTime = -1.0;
    double LastDodgeTime = -10.0;
    FTimerHandle ReloadTimer;
};
