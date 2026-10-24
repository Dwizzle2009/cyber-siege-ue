#include "CSEnemy.h"
#include "CSGameMode.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ACSEnemy::ACSEnemy()
{
    PrimaryActorTick.bCanEverTick = true;

    // A plain AIController possesses us so CharacterMovement consumes AddMovementInput.
    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    GetCapsuleComponent()->InitCapsuleSize(38.f, 88.f);
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
    GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
    bUseControllerRotationYaw = false;

    // Placeholder body: a stretched cube. Replace with the enemy robot skeletal mesh later.
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(RootComponent);
    Body->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.7f));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) Body->SetStaticMesh(Cube.Object);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (Mat.Succeeded()) Body->SetMaterial(0, Mat.Object);
}

void ACSEnemy::BeginPlay()
{
    Super::BeginPlay();
    Health = MaxHealth;
    GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
    BodyMID = Body->CreateAndSetMaterialInstanceDynamic(0);
    if (BodyMID) BodyMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.92f, 0.08f, 0.16f));
}

void ACSEnemy::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!Player) return;

    FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
    ToPlayer.Z = 0.f;
    const float Dist = ToPlayer.Size();

    if (Dist > ContactRange)
    {
        AddMovementInput(ToPlayer.GetSafeNormal());
    }
    else
    {
        const double Now = GetWorld()->GetTimeSeconds();
        if (Now >= NextContactTime)
        {
            NextContactTime = Now + ContactCooldown;
            UGameplayStatics::ApplyDamage(Player, ContactDamage, GetController(), this, nullptr);
        }
    }
}

float ACSEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                           AController* EventInstigator, AActor* DamageCauser)
{
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    Health -= Applied;

    if (BodyMID)
    {
        BodyMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.9f, 0.9f));
        GetWorldTimerManager().SetTimer(FlashTimer, this, &ACSEnemy::EndFlash, 0.06f, false);
    }

    if (Health <= 0.f)
    {
        if (ACSGameMode* GM = GetWorld()->GetAuthGameMode<ACSGameMode>()) GM->RegisterEnemyKill(this, ScoreValue);
        Destroy();
    }
    return Applied;
}

void ACSEnemy::EndFlash()
{
    if (BodyMID) BodyMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.92f, 0.08f, 0.16f));
}
