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
class ACSSquadMate;
class ACSBreachWall;
class ACSRescuee;

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
    bool  bLensUp = false;
    bool  bInteracting = false;
    /** What the crosshair is on right now, refreshed every tick: a ground point for commands, a wall, a downed mate, a rescuee. */
    FVector AimPoint = FVector::ZeroVector; bool bAimValid = false;
    TWeakObjectPtr<ACSBreachWall> AimWall;
    TWeakObjectPtr<ACSSquadMate>  AimDownedMate;
    TWeakObjectPtr<AActor>        AimTalkTarget;
    float InteractHeld = 0.f;
    /** The one prompt the HUD shows for the thing under the crosshair ("Hold  [down]", "Breach  [right]", "Revive Bell  hold [Y]"). */
    FText ContextPrompt;
    /** Health regenerates after RegenDelay without damage (the slice has no medkits; Bell is the medic). */
    UPROPERTY(EditAnywhere, Category="Combat") float RegenDelay = 4.f;
    UPROPERTY(EditAnywhere, Category="Combat") float RegenPerSecond = 18.f;
    double LastDamageTime = -100.0;

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
    UPROPERTY() UInputAction* IA_Lens = nullptr;       // hold: AEGIS overlay
    UPROPERTY() UInputAction* IA_CmdHold = nullptr;    // squad: hold where I'm aiming
    UPROPERTY() UInputAction* IA_CmdMove = nullptr;    // squad: move to where I'm aiming
    UPROPERTY() UInputAction* IA_CmdBreach = nullptr;  // Mercer: breach the wall I'm aiming at
    UPROPERTY() UInputAction* IA_CmdFollow = nullptr;  // squad: on me
    UPROPERTY() UInputAction* IA_Interact = nullptr;   // revive / talk / confirm (hold)

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
    void OnLensStart(const FInputActionValue& V);
    void OnLensStop(const FInputActionValue& V);
    void OnCmdHold(const FInputActionValue& V);
    void OnCmdMove(const FInputActionValue& V);
    void OnCmdBreach(const FInputActionValue& V);
    void OnCmdFollow(const FInputActionValue& V);
    void OnInteractStart(const FInputActionValue& V);
    void OnInteractStop(const FInputActionValue& V);

    void FinishReload();
    void Die();
    void UpdateAim();
    void OrderSquad(TFunctionRef<void(ACSSquadMate*)> Fn, FName Only = NAME_None);

    double LastFireTime = -1.0;
    double LastDodgeTime = -10.0;
    FTimerHandle ReloadTimer;
};
