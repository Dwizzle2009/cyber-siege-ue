#include "CSSquadMate.h"
#include "CSBreachWall.h"
#include "AI/CSEnemy.h"
#include "Mission/CSMissionDirector.h"
#include "Dialogue/CSDialogueSubsystem.h"
#include "CyberSiege.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

ACSSquadMate::ACSSquadMate()
{
    PrimaryActorTick.bCanEverTick = true;
    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    GetCapsuleComponent()->InitCapsuleSize(38.f, 92.f);
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 480.f, 0.f);
    GetCharacterMovement()->MaxWalkSpeed = 600.f;
    GetCharacterMovement()->bUseRVOAvoidance = true;
    GetCharacterMovement()->AvoidanceConsiderationRadius = 300.f;
    bUseControllerRotationYaw = false;

    // Placeholder body + rifle. The MetaHuman replaces Body; Rifle becomes the skeletal weapon.
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(RootComponent);
    Body->SetRelativeScale3D(FVector(0.65f, 0.65f, 1.75f));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) Body->SetStaticMesh(Cube.Object);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (Mat.Succeeded()) Body->SetMaterial(0, Mat.Object);

    Rifle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rifle"));
    Rifle->SetupAttachment(RootComponent);
    Rifle->SetRelativeLocation(FVector(40.f, 18.f, 20.f));
    Rifle->SetRelativeScale3D(FVector(0.6f, 0.07f, 0.09f));
    Rifle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Rifle->SetCastShadow(false);
    if (Cube.Succeeded()) Rifle->SetStaticMesh(Cube.Object);
}

void ACSSquadMate::BeginPlay()
{
    Super::BeginPlay();
    Health = MaxHealth;
    if (UMaterialInstanceDynamic* M = Body->CreateAndSetMaterialInstanceDynamic(0)) M->SetVectorParameterValue(TEXT("Color"), TeamColor);
    if (ACSMissionDirector* D = ACSMissionDirector::Get(this)) D->RegisterSquadMate(this);
}

AAIController* ACSSquadMate::AI() const { return Cast<AAIController>(GetController()); }

// ---- orders -----------------------------------------------------------------------------------
void ACSSquadMate::OrderFollow()                        { Order = ECSSquadOrder::Follow; BreachTarget = nullptr; CarryTarget = nullptr; }
void ACSSquadMate::OrderHold(const FVector& W)          { Order = ECSSquadOrder::Hold;   OrderPoint = W; Bark(TEXT("ack_hold"), 2.f); }
void ACSSquadMate::OrderMoveTo(const FVector& W)        { Order = ECSSquadOrder::MoveTo; OrderPoint = W; Bark(TEXT("ack_move"), 2.f); }
void ACSSquadMate::OrderCarry(AActor* R)                { Order = ECSSquadOrder::Carry;  CarryTarget = R; }
void ACSSquadMate::OrderScripted(const FVector& At, float Yaw) { Order = ECSSquadOrder::Scripted; OrderPoint = At; OrderYaw = Yaw; }
void ACSSquadMate::OrderBreach(ACSBreachWall* Wall)
{
    if (!Wall || Wall->IsBreached()) return;
    Order = ECSSquadOrder::Breach; BreachTarget = Wall; BreachArmedAt = 0.f; Bark(TEXT("ack_breach"), 2.f);
}

void ACSSquadMate::TeleportBeside(const FVector& PlayerLoc, float PlayerYaw)
{
    const FVector Off = FRotator(0.f, PlayerYaw, 0.f).RotateVector(FollowOffset);
    SetActorLocation(PlayerLoc + Off + FVector(0, 0, 4.f), false, nullptr, ETeleportType::TeleportPhysics);
    if (Order != ECSSquadOrder::Scripted) OrderFollow();
}

// ---- movement helpers ---------------------------------------------------------------------------
void ACSSquadMate::MoveTowards(const FVector& Where, float AcceptRadius)
{
    AAIController* C = AI(); if (!C) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now < NextRepathTime) return;
    NextRepathTime = Now + 0.4;
    if (FVector::DistXY(GetActorLocation(), Where) <= AcceptRadius) { C->StopMovement(); return; }
    C->MoveToLocation(Where, AcceptRadius * 0.6f, true, true, false, true);
}

void ACSSquadMate::FaceTowards(const FVector& Where, float DeltaSeconds)
{
    FVector To = Where - GetActorLocation(); To.Z = 0.f;
    if (To.IsNearlyZero()) return;
    const FRotator Want = To.Rotation();
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, Want.Yaw, 0.f), DeltaSeconds, 8.f));
}

// ---- combat -------------------------------------------------------------------------------------
ACSEnemy* ACSSquadMate::PickTarget() const
{
    ACSEnemy* Best = nullptr; float BestD = EngageRange;
    const FVector Eye = GetActorLocation() + FVector(0, 0, 60.f);
    for (TActorIterator<ACSEnemy> It(GetWorld()); It; ++It)
    {
        ACSEnemy* E = *It; if (!E || E->IsDead()) continue;
        const float D = FVector::Dist(Eye, E->GetActorLocation());
        if (D >= BestD) continue;
        FHitResult H; FCollisionQueryParams Q(SCENE_QUERY_STAT(SquadLOS), false, this);
        if (GetWorld()->LineTraceSingleByChannel(H, Eye, E->GetActorLocation() + FVector(0, 0, 50.f), ECC_Visibility, Q) && H.GetActor() != E) continue;
        Best = E; BestD = D;
    }
    return Best;
}

void ACSSquadMate::TryShoot(ACSEnemy* T, float DeltaSeconds)
{
    if (!T) return;
    FaceTowards(T->GetActorLocation(), DeltaSeconds);
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now < NextShotTime) return;
    NextShotTime = Now + ShotInterval * FMath::FRandRange(0.8f, 1.3f);
    // A squad mate's bullets are abstract: a hit chance, a tracer the HUD can draw later, damage on a hit.
    if (FMath::FRand() < HitChance) UGameplayStatics::ApplyDamage(T, ShotDamage, GetController(), this, nullptr);
}

float ACSSquadMate::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (Order == ECSSquadOrder::Down || bInvulnerable) return 0.f;
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    Health = FMath::Clamp(Health - Applied, 0.f, MaxHealth);
    if (Health <= 0.f) GoDown();
    else if (Health < MaxHealth * 0.3f) Bark(FName(*FString::Printf(TEXT("hurt_%s"), *Callsign.ToString())), 12.f);
    return Applied;
}

void ACSSquadMate::GoDown()
{
    Order = ECSSquadOrder::Down; DownSince = GetWorld()->GetTimeSeconds(); ReviveProgress = 0.f;
    if (AAIController* C = AI()) C->StopMovement();
    GetCapsuleComponent()->SetCapsuleHalfHeight(40.f);
    Body->SetRelativeRotation(FRotator(0.f, 0.f, 88.f));
    if (ACSMissionDirector* D = ACSMissionDirector::Get(this)) D->FireEvent(FName(*FString::Printf(TEXT("down_%s"), *Callsign.ToString())));
    if (UCSDialogueSubsystem* Dlg = GetWorld()->GetSubsystem<UCSDialogueSubsystem>()) Dlg->Bark(FName(*FString::Printf(TEXT("mate_down_%s"), *Callsign.ToString())), 1.f);
}

float ACSSquadMate::BleedOutRemaining() const
{
    return Order == ECSSquadOrder::Down ? FMath::Max(0.f, BleedOutSeconds - (GetWorld()->GetTimeSeconds() - DownSince)) : 0.f;
}

void ACSSquadMate::ReviveTick(float DeltaSeconds)
{
    if (Order != ECSSquadOrder::Down) return;
    ReviveProgress += DeltaSeconds;
    if (ReviveProgress >= ReviveSeconds) Revive();
}

void ACSSquadMate::Revive()
{
    if (Order != ECSSquadOrder::Down) return;
    Health = MaxHealth * 0.5f;
    GetCapsuleComponent()->SetCapsuleHalfHeight(92.f);
    Body->SetRelativeRotation(FRotator::ZeroRotator);
    OrderFollow();
    Bark(FName(*FString::Printf(TEXT("revived_%s"), *Callsign.ToString())), 1.f);
    if (ACSMissionDirector* D = ACSMissionDirector::Get(this)) D->FireEvent(FName(*FString::Printf(TEXT("up_%s"), *Callsign.ToString())));
}

void ACSSquadMate::Bark(FName Trigger, float Cooldown)
{
    if (UCSDialogueSubsystem* Dlg = GetWorld()->GetSubsystem<UCSDialogueSubsystem>()) Dlg->Bark(Trigger, Cooldown);
}

// ---- the brain ----------------------------------------------------------------------------------
void ACSSquadMate::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!Player) return;
    const float Now = GetWorld()->GetTimeSeconds();

    switch (Order)
    {
    case ECSSquadOrder::Down:
        if (Now - DownSince > BleedOutSeconds && !bInvulnerable) { /* Mission 1 never loses a squad mate; later missions may */ Revive(); }
        return;

    case ECSSquadOrder::Scripted:
        MoveTowards(OrderPoint, 60.f);
        if (FVector::DistXY(GetActorLocation(), OrderPoint) < 80.f) SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, OrderYaw, 0.f), DeltaSeconds, 6.f));
        return;

    case ECSSquadOrder::Follow:
    {
        const FVector Want = Player->GetActorLocation() + FRotator(0.f, Player->GetActorRotation().Yaw, 0.f).RotateVector(FollowOffset);
        const float D = FVector::DistXY(GetActorLocation(), Want);
        GetCharacterMovement()->MaxWalkSpeed = D > 900.f ? 900.f : 600.f;   // catch up when left behind
        if (ACSEnemy* T = PickTarget()) { if (D < 500.f) { if (AAIController* C = AI()) C->StopMovement(); TryShoot(T, DeltaSeconds); } else MoveTowards(Want, 150.f); }
        else MoveTowards(Want, 150.f);
        break;
    }

    case ECSSquadOrder::MoveTo:
        MoveTowards(OrderPoint, 90.f);
        if (FVector::DistXY(GetActorLocation(), OrderPoint) < 120.f) { Order = ECSSquadOrder::Hold; }
        break;

    case ECSSquadOrder::Hold:
        MoveTowards(OrderPoint, 90.f);
        if (ACSEnemy* T = PickTarget()) TryShoot(T, DeltaSeconds);
        else FaceTowards(Player->GetActorLocation(), DeltaSeconds);
        break;

    case ECSSquadOrder::Breach:
    {
        ACSBreachWall* Wall = BreachTarget.Get();
        if (!Wall || Wall->IsBreached()) { OrderFollow(); break; }
        const FVector Stand = Wall->StandPoint();
        if (FVector::DistXY(GetActorLocation(), Stand) > 110.f) { MoveTowards(Stand, 80.f); BreachArmedAt = 0.f; break; }
        FaceTowards(Wall->GetActorLocation(), DeltaSeconds);
        if (BreachArmedAt <= 0.f) { BreachArmedAt = Now; Wall->BeginCountdown(this); }
        if (Now - BreachArmedAt >= Wall->FuseSeconds) { Wall->Detonate(this); OrderFollow(); }
        break;
    }

    case ECSSquadOrder::Carry:
    {
        AActor* R = CarryTarget.Get(); if (!R) { OrderFollow(); break; }
        // walk beside the rescuee; the rescuee follows the player at its own pace, so this reads as a carry
        const FVector Want = R->GetActorLocation() + R->GetActorRightVector() * 90.f;
        GetCharacterMovement()->MaxWalkSpeed = 420.f;
        MoveTowards(Want, 70.f);
        break;
    }
    }
}
