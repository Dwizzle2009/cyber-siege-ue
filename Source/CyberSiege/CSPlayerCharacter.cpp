#include "CSPlayerCharacter.h"
#include "CSProjectile.h"
#include "CSGameMode.h"
#include "CyberSiege.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ACSPlayerCharacter::ACSPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(40.f, 92.f);

    // First person: the controller drives yaw and pitch, the camera follows the controller.
    bUseControllerRotationYaw = true;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;

    UCharacterMovementComponent* Move = GetCharacterMovement();
    Move->bOrientRotationToMovement = false;
    Move->MaxWalkSpeed = WalkSpeed;
    Move->JumpZVelocity = 560.f;
    Move->AirControl = 0.35f;
    Move->BrakingDecelerationWalking = 2200.f;
    Move->GroundFriction = 8.f;

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));   // eye height
    FirstPersonCamera->bUsePawnControlRotation = true;
    FirstPersonCamera->FieldOfView = 92.f;

    // Placeholder rifle hanging off the camera. Swap for the real skeletal weapon later;
    // gameplay reads MuzzlePoint, not this mesh.
    PrototypeGun = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeGun"));
    PrototypeGun->SetupAttachment(FirstPersonCamera);
    PrototypeGun->SetRelativeLocation(FVector(42.f, 18.f, -22.f));
    PrototypeGun->SetRelativeScale3D(FVector(0.55f, 0.08f, 0.10f));
    PrototypeGun->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PrototypeGun->SetCastShadow(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded()) PrototypeGun->SetStaticMesh(CubeMesh.Object);

    MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
    MuzzlePoint->SetupAttachment(FirstPersonCamera);
    MuzzlePoint->SetRelativeLocation(FVector(70.f, 18.f, -20.f));

    ProjectileClass = ACSProjectile::StaticClass();
}

void ACSPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    Health = MaxHealth;
    AmmoInMag = MagazineSize;
    AmmoReserve = ReserveAmmo;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void ACSPlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}

// ------------------------------------------------------------------------------------
// Enhanced Input, entirely in code. One place holds every binding.
// ------------------------------------------------------------------------------------
static UInputAction* MakeAction(UObject* Outer, const TCHAR* Name, EInputActionValueType Type)
{
    UInputAction* A = NewObject<UInputAction>(Outer, Name);
    A->ValueType = Type;
    return A;
}

void ACSPlayerCharacter::BuildInputMapping()
{
    Mapping      = NewObject<UInputMappingContext>(this, TEXT("IMC_CyberSiege"));
    IA_Move      = MakeAction(this, TEXT("IA_Move"),      EInputActionValueType::Axis2D);
    IA_LookMouse = MakeAction(this, TEXT("IA_LookMouse"), EInputActionValueType::Axis2D);
    IA_LookStick = MakeAction(this, TEXT("IA_LookStick"), EInputActionValueType::Axis2D);
    IA_Fire      = MakeAction(this, TEXT("IA_Fire"),      EInputActionValueType::Boolean);
    IA_Reload    = MakeAction(this, TEXT("IA_Reload"),    EInputActionValueType::Boolean);
    IA_Jump      = MakeAction(this, TEXT("IA_Jump"),      EInputActionValueType::Boolean);
    IA_Sprint    = MakeAction(this, TEXT("IA_Sprint"),    EInputActionValueType::Boolean);
    IA_Dodge     = MakeAction(this, TEXT("IA_Dodge"),     EInputActionValueType::Boolean);

    auto Negate = [this](bool bX, bool bY) {
        UInputModifierNegate* M = NewObject<UInputModifierNegate>(this);
        M->bX = bX; M->bY = bY; M->bZ = false; return M; };
    auto SwizzleYX = [this]() {
        UInputModifierSwizzleAxis* M = NewObject<UInputModifierSwizzleAxis>(this);
        M->Order = EInputAxisSwizzle::YXZ; return M; };

    // Move: X = right, Y = forward
    Mapping->MapKey(IA_Move, EKeys::Gamepad_Left2D);
    Mapping->MapKey(IA_Move, EKeys::D);
    { FEnhancedActionKeyMapping& K = Mapping->MapKey(IA_Move, EKeys::A); K.Modifiers.Add(Negate(true, false)); }
    { FEnhancedActionKeyMapping& K = Mapping->MapKey(IA_Move, EKeys::W); K.Modifiers.Add(SwizzleYX()); }
    { FEnhancedActionKeyMapping& K = Mapping->MapKey(IA_Move, EKeys::S); K.Modifiers.Add(Negate(true, false)); K.Modifiers.Add(SwizzleYX()); }

    // Look: mouse is a delta, stick is a rate (scaled by DeltaTime in the handler)
    { FEnhancedActionKeyMapping& K = Mapping->MapKey(IA_LookMouse, EKeys::Mouse2D);         K.Modifiers.Add(Negate(false, true)); }
    { FEnhancedActionKeyMapping& K = Mapping->MapKey(IA_LookStick, EKeys::Gamepad_Right2D); K.Modifiers.Add(Negate(false, true)); }

    // Actions
    Mapping->MapKey(IA_Fire,   EKeys::LeftMouseButton);
    Mapping->MapKey(IA_Fire,   EKeys::Gamepad_RightTrigger);        // RT only -- no RB
    Mapping->MapKey(IA_Reload, EKeys::R);
    Mapping->MapKey(IA_Reload, EKeys::Gamepad_FaceButton_Left);     // X
    Mapping->MapKey(IA_Jump,   EKeys::SpaceBar);
    Mapping->MapKey(IA_Jump,   EKeys::Gamepad_FaceButton_Bottom);   // A
    Mapping->MapKey(IA_Sprint, EKeys::LeftShift);
    Mapping->MapKey(IA_Sprint, EKeys::Gamepad_LeftThumbstick);      // click LS
    Mapping->MapKey(IA_Dodge,  EKeys::LeftAlt);
    Mapping->MapKey(IA_Dodge,  EKeys::Gamepad_FaceButton_Right);    // B
}

void ACSPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    BuildInputMapping();

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
        if (ULocalPlayer* LP = PC->GetLocalPlayer())
            if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                Sub->ClearAllMappings();
                Sub->AddMappingContext(Mapping, 0);
            }

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!EIC) { UE_LOG(LogCyberSiege, Error, TEXT("Enhanced Input component missing -- check DefaultInput.ini")); return; }

    EIC->BindAction(IA_Move,      ETriggerEvent::Triggered, this, &ACSPlayerCharacter::OnMove);
    EIC->BindAction(IA_LookMouse, ETriggerEvent::Triggered, this, &ACSPlayerCharacter::OnLookMouse);
    EIC->BindAction(IA_LookStick, ETriggerEvent::Triggered, this, &ACSPlayerCharacter::OnLookStick);
    EIC->BindAction(IA_Fire,      ETriggerEvent::Triggered, this, &ACSPlayerCharacter::OnFire);      // held = auto
    EIC->BindAction(IA_Reload,    ETriggerEvent::Started,   this, &ACSPlayerCharacter::OnReload);
    EIC->BindAction(IA_Jump,      ETriggerEvent::Started,   this, &ACSPlayerCharacter::OnJumpStart);
    EIC->BindAction(IA_Jump,      ETriggerEvent::Completed, this, &ACSPlayerCharacter::OnJumpStop);
    EIC->BindAction(IA_Sprint,    ETriggerEvent::Started,   this, &ACSPlayerCharacter::OnSprintStart);
    EIC->BindAction(IA_Sprint,    ETriggerEvent::Completed, this, &ACSPlayerCharacter::OnSprintStop);
    EIC->BindAction(IA_Dodge,     ETriggerEvent::Started,   this, &ACSPlayerCharacter::OnDodge);
}

// ------------------------------------------------------------------------------------
void ACSPlayerCharacter::OnMove(const FInputActionValue& V)
{
    if (bDead) return;
    const FVector2D In = V.Get<FVector2D>();
    const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);
    AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), In.Y);
    AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), In.X);
}

void ACSPlayerCharacter::OnLookMouse(const FInputActionValue& V)
{
    if (bDead) return;
    const FVector2D In = V.Get<FVector2D>();
    AddControllerYawInput(In.X);
    AddControllerPitchInput(In.Y);
}

void ACSPlayerCharacter::OnLookStick(const FInputActionValue& V)
{
    if (bDead) return;
    const FVector2D In = V.Get<FVector2D>();
    const float Dt = GetWorld()->GetDeltaSeconds();
    AddControllerYawInput(In.X * StickLookRate * Dt);
    AddControllerPitchInput(In.Y * StickLookRate * Dt);
}

void ACSPlayerCharacter::OnFire(const FInputActionValue&)
{
    if (bDead)
    {
        if (ACSGameMode* GM = GetWorld()->GetAuthGameMode<ACSGameMode>()) GM->RequestRestart();
        return;
    }
    if (bReloading || !ProjectileClass) return;
    if (AmmoInMag <= 0) { OnReload(FInputActionValue()); return; }

    const double Now = GetWorld()->GetTimeSeconds();
    if (Now - LastFireTime < FireInterval) return;
    LastFireTime = Now;
    AmmoInMag--;

    // Spawn along the camera's aim so the projectile goes where the crosshair is.
    const FRotator Aim = GetControlRotation();
    const FVector  From = MuzzlePoint->GetComponentLocation();
    FActorSpawnParameters P;
    P.Owner = this; P.Instigator = this;
    P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    GetWorld()->SpawnActor<ACSProjectile>(ProjectileClass, From, Aim, P);

    // Recoil: a small kick up and to the side, felt through the controller rotation.
    AddControllerPitchInput(-0.55f);
    AddControllerYawInput(FMath::FRandRange(-0.25f, 0.25f));
}

void ACSPlayerCharacter::OnReload(const FInputActionValue&)
{
    if (bDead || bReloading || AmmoInMag >= MagazineSize || AmmoReserve <= 0) return;
    bReloading = true;
    GetWorldTimerManager().SetTimer(ReloadTimer, this, &ACSPlayerCharacter::FinishReload, ReloadTime, false);
}

void ACSPlayerCharacter::FinishReload()
{
    const int32 Need = MagazineSize - AmmoInMag;
    const int32 Take = FMath::Min(Need, AmmoReserve);
    AmmoInMag += Take;
    AmmoReserve -= Take;
    bReloading = false;
}

void ACSPlayerCharacter::OnJumpStart(const FInputActionValue&)
{
    if (bDead)
    {
        if (ACSGameMode* GM = GetWorld()->GetAuthGameMode<ACSGameMode>()) GM->RequestRestart();
        return;
    }
    Jump();
}
void ACSPlayerCharacter::OnJumpStop(const FInputActionValue&) { StopJumping(); }

void ACSPlayerCharacter::OnSprintStart(const FInputActionValue&) { if (!bDead) GetCharacterMovement()->MaxWalkSpeed = SprintSpeed; }
void ACSPlayerCharacter::OnSprintStop(const FInputActionValue&)  { GetCharacterMovement()->MaxWalkSpeed = WalkSpeed; }

void ACSPlayerCharacter::OnDodge(const FInputActionValue&)
{
    if (bDead) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now - LastDodgeTime < DodgeCooldown) return;
    LastDodgeTime = Now;

    // Dash in the direction being pressed; straight back if standing still.
    FVector Dir = GetLastMovementInputVector();
    if (Dir.IsNearlyZero()) Dir = -GetActorForwardVector();
    Dir.Z = 0.f;
    LaunchCharacter(Dir.GetSafeNormal() * DodgeImpulse + FVector(0, 0, 120.f), true, true);
}

// ------------------------------------------------------------------------------------
float ACSPlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                                     AController* EventInstigator, AActor* DamageCauser)
{
    if (bDead) return 0.f;
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    Health = FMath::Clamp(Health - Applied, 0.f, MaxHealth);
    if (Health <= 0.f) Die();
    return Applied;
}

void ACSPlayerCharacter::Die()
{
    bDead = true;
    GetCharacterMovement()->DisableMovement();
    if (ACSGameMode* GM = GetWorld()->GetAuthGameMode<ACSGameMode>()) GM->OnPlayerDied();
}
