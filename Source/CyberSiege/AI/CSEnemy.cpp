#include "CSEnemy.h"
#include "CSCoverPoint.h"
#include "CSGameMode.h"
#include "Squad/CSSquadMate.h"
#include "Squad/CSRescuee.h"
#include "Dialogue/CSDialogueSubsystem.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ACSEnemy::ACSEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    GetCapsuleComponent()->InitCapsuleSize(38.f, 90.f);
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
    GetCharacterMovement()->bUseRVOAvoidance = true;
    bUseControllerRotationYaw = false;

    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(RootComponent);
    Body->SetRelativeScale3D(FVector(0.65f, 0.65f, 1.75f));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) Body->SetStaticMesh(Cube.Object);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (Mat.Succeeded()) Body->SetMaterial(0, Mat.Object);

    Shield = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shield"));
    Shield->SetupAttachment(RootComponent);
    Shield->SetRelativeLocation(FVector(55.f, 0.f, 0.f));
    Shield->SetRelativeScale3D(FVector(0.12f, 1.1f, 1.7f));
    Shield->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Shield->SetVisibility(false);
    if (Cube.Succeeded()) Shield->SetStaticMesh(Cube.Object);
}

void ACSEnemy::BeginPlay()
{
    Super::BeginPlay();
    SpawnLocation = GetActorLocation();
    switch (Type)
    {
    case ECSEnemyType::Heavy:  MaxHealth = 320.f; MoveSpeed = 260.f; ScoreValue = 400; Shield->SetVisibility(true); break;
    case ECSEnemyType::Gunner: MaxHealth = 120.f; MoveSpeed = 0.f; BurstCount = 12; BurstInterval = 0.07f; BurstPause = 1.8f; SpreadDegrees = 5.f; ShotDamage = 7.f; ScoreValue = 250; break;
    default: break;
    }
    Health = MaxHealth;
    GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
    BodyMID = Body->CreateAndSetMaterialInstanceDynamic(0);
    if (BodyMID) BodyMID->SetVectorParameterValue(TEXT("Color"), Type == ECSEnemyType::Heavy ? FLinearColor(0.5f, 0.1f, 0.1f) : FLinearColor(0.72f, 0.16f, 0.14f));
}

// ---- perception -------------------------------------------------------------------------------
bool ACSEnemy::CanSee(const AActor* A) const
{
    if (!A) return false;
    FHitResult H; FCollisionQueryParams Q(SCENE_QUERY_STAT(EnemyLOS), false, this);
    const FVector To = A->GetActorLocation() + FVector(0, 0, 50.f);
    return !GetWorld()->LineTraceSingleByChannel(H, EyeLocation(), To, ECC_Visibility, Q) || H.GetActor() == A;
}

AActor* ACSEnemy::PickTarget()
{
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now < NextTargetScan && Target.IsValid()) return Target.Get();
    NextTargetScan = Now + 0.5;

    // The player draws fire first; a squad mate or a rescuee in the open is next; anything unseen is ignored.
    AActor* Best = nullptr; float BestScore = -1.f;
    auto Consider = [&](AActor* A, float Weight)
    {
        if (!A) return;
        const float D = FVector::Dist(GetActorLocation(), A->GetActorLocation());
        if (D > EngageRange || !CanSee(A)) return;
        const float S = Weight / FMath::Max(300.f, D);
        if (S > BestScore) { BestScore = S; Best = A; }
    };
    Consider(UGameplayStatics::GetPlayerPawn(this, 0), 1.5f);
    for (TActorIterator<ACSSquadMate> It(GetWorld()); It; ++It) if (!It->IsDown()) Consider(*It, 1.0f);
    for (TActorIterator<ACSRescuee> It(GetWorld()); It; ++It) if (It->IsExposed()) Consider(*It, 1.2f);
    Target = Best;
    return Best;
}

// ---- movement -----------------------------------------------------------------------------------
void ACSEnemy::MoveTowards(const FVector& Where, float Accept)
{
    AAIController* C = Cast<AAIController>(GetController()); if (!C) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now < NextRepath) return;
    NextRepath = Now + 0.5;
    if (FVector::DistXY(GetActorLocation(), Where) <= Accept) { C->StopMovement(); return; }
    C->MoveToLocation(Where, Accept * 0.6f, true, true, false, true);
}

void ACSEnemy::FaceTowards(const FVector& Where, float Dt)
{
    FVector To = Where - GetActorLocation(); To.Z = 0.f; if (To.IsNearlyZero()) return;
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, To.Rotation().Yaw, 0.f), Dt, 9.f));
}

// ---- fire ---------------------------------------------------------------------------------------
void ACSEnemy::Shoot(AActor* At)
{
    const FVector From = EyeLocation();
    FVector Dir = (At->GetActorLocation() + FVector(0, 0, FMath::FRandRange(10.f, 60.f)) - From).GetSafeNormal();
    Dir = FMath::VRandCone(Dir, FMath::DegreesToRadians(SpreadDegrees));
    FHitResult H; FCollisionQueryParams Q(SCENE_QUERY_STAT(EnemyShot), false, this);
    if (GetWorld()->LineTraceSingleByChannel(H, From, From + Dir * EngageRange * 1.5f, ECC_Visibility, Q) && H.GetActor())
        UGameplayStatics::ApplyPointDamage(H.GetActor(), ShotDamage, Dir, H, GetController(), this, nullptr);
}

void ACSEnemy::TickRifleman(float Dt, AActor* T)
{
    const double Now = GetWorld()->GetTimeSeconds();
    if (!T)
    {
        if (bHasObjective) MoveTowards(Objective, 200.f);
        return;
    }
    const float D = FVector::Dist(GetActorLocation(), T->GetActorLocation());

    // Cover: pick a point that faces the target within preferred range, sit for a few seconds, then move up.
    if (Now > CoverUntil)
    {
        ACSCoverPoint* Best = nullptr; float BestScore = -1.f;
        for (TActorIterator<ACSCoverPoint> It(GetWorld()); It; ++It)
        {
            ACSCoverPoint* P = *It; if (P->IsTaken() && P->Occupant != this) continue;
            const float DT = FVector::Dist(P->GetActorLocation(), T->GetActorLocation());
            const float DM = FVector::Dist(P->GetActorLocation(), GetActorLocation());
            if (DT > PreferredRange * 1.4f || DT < 350.f || DM > 2200.f) continue;
            if (!P->Protects(T->GetActorLocation())) continue;
            const float Score = 1.f / (DM + 0.5f * FMath::Abs(DT - PreferredRange) + 1.f);
            if (Score > BestScore) { BestScore = Score; Best = P; }
        }
        if (Cover.IsValid()) Cover->Occupant = nullptr;
        Cover = Best; if (Best) Best->Occupant = this;
        CoverUntil = Now + FMath::FRandRange(4.f, 8.f);
    }

    if (Cover.IsValid()) MoveTowards(Cover->GetActorLocation(), 70.f);
    else if (D > PreferredRange) MoveTowards(T->GetActorLocation(), PreferredRange * 0.8f);
    else if (AAIController* C = Cast<AAIController>(GetController())) C->StopMovement();

    if (D < EngageRange && CanSee(T))
    {
        FaceTowards(T->GetActorLocation(), Dt);
        if (Now >= NextShot)
        {
            if (BurstLeft <= 0) BurstLeft = BurstCount;
            Shoot(T); BurstLeft--;
            NextShot = Now + (BurstLeft > 0 ? BurstInterval : BurstPause * FMath::FRandRange(0.8f, 1.4f));
        }
    }
}

void ACSEnemy::TickHeavy(float Dt, AActor* T)
{
    const FVector Goal = T ? T->GetActorLocation() : (bHasObjective ? Objective : GetActorLocation());
    const float D = FVector::DistXY(GetActorLocation(), Goal);
    if (D > 180.f) { MoveTowards(Goal, 150.f); if (T) FaceTowards(Goal, Dt); return; }
    if (!T) return;
    FaceTowards(Goal, Dt);
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now >= NextShot) { NextShot = Now + 1.1; UGameplayStatics::ApplyDamage(T, MeleeDamage, GetController(), this, nullptr); }
}

void ACSEnemy::TickGunner(float Dt, AActor* T)
{
    if (!T) return;
    const double Now = GetWorld()->GetTimeSeconds();
    FaceTowards(T->GetActorLocation(), Dt);
    if (!CanSee(T) || Now < NextShot) return;
    if (BurstLeft <= 0) BurstLeft = BurstCount;
    Shoot(T); BurstLeft--;
    NextShot = Now + (BurstLeft > 0 ? BurstInterval : BurstPause);
}

void ACSEnemy::Tick(float Dt)
{
    Super::Tick(Dt);
    if (bDead) return;
    AActor* T = PickTarget();
    switch (Type)
    {
    case ECSEnemyType::Heavy:  TickHeavy(Dt, T); break;
    case ECSEnemyType::Gunner: TickGunner(Dt, T); break;
    default:                   TickRifleman(Dt, T); break;
    }
}

// ---- damage -------------------------------------------------------------------------------------
float ACSEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bDead) return 0.f;
    float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    if (Type == ECSEnemyType::Heavy && DamageCauser)
    {
        // The shield: anything coming from the front half does almost nothing. Flank him, he turns slow.
        FVector From = DamageCauser->GetActorLocation() - GetActorLocation(); From.Z = 0.f;
        if (FVector::DotProduct(From.GetSafeNormal(), GetActorForwardVector()) > 0.35f) Applied *= 0.15f;
    }
    Health -= Applied;
    if (BodyMID) { BodyMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.9f, 0.9f)); GetWorldTimerManager().SetTimer(FlashTimer, this, &ACSEnemy::EndFlash, 0.06f, false); }
    if (Health <= 0.f) Die(EventInstigator);
    return Applied;
}

void ACSEnemy::Die(AController* Killer)
{
    bDead = true;
    if (Cover.IsValid()) Cover->Occupant = nullptr;
    if (AAIController* C = Cast<AAIController>(GetController())) C->StopMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetRelativeRotation(FRotator(0.f, 0.f, 90.f)); Body->SetRelativeLocation(FVector(0, 0, -60.f));
    Shield->SetVisibility(false);
    if (ACSGameMode* GM = GetWorld()->GetAuthGameMode<ACSGameMode>()) GM->RegisterEnemyKill(this, ScoreValue, Killer);
    OnDied.Broadcast(this);
    SetLifeSpan(8.f);
}

void ACSEnemy::EndFlash()
{
    if (BodyMID) BodyMID->SetVectorParameterValue(TEXT("Color"), Type == ECSEnemyType::Heavy ? FLinearColor(0.5f, 0.1f, 0.1f) : FLinearColor(0.72f, 0.16f, 0.14f));
}
