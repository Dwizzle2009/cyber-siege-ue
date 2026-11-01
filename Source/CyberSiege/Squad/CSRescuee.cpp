#include "CSRescuee.h"
#include "AI/CSEnemy.h"
#include "Mission/CSMissionDirector.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

ACSRescuee::ACSRescuee()
{
    PrimaryActorTick.bCanEverTick = true;
    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    GetCapsuleComponent()->InitCapsuleSize(36.f, 90.f);
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 360.f, 0.f);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    bUseControllerRotationYaw = false;

    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(RootComponent);
    Body->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.7f));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) Body->SetStaticMesh(Cube.Object);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (Mat.Succeeded()) Body->SetMaterial(0, Mat.Object);
}

void ACSRescuee::BeginPlay()
{
    Super::BeginPlay();
    Health = MaxHealth;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    if (UMaterialInstanceDynamic* M = Body->CreateAndSetMaterialInstanceDynamic(0)) M->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.95f, 0.78f, 0.30f));
    if (State == ECSRescueeState::Trapped) Body->SetRelativeRotation(FRotator(0.f, 0.f, 70.f));
}

void ACSRescuee::SetState(ECSRescueeState S)
{
    State = S;
    Body->SetRelativeRotation(S == ECSRescueeState::Trapped ? FRotator(0.f, 0.f, 70.f) : FRotator::ZeroRotator);
    if (ACSMissionDirector* D = ACSMissionDirector::Get(this))
    {
        static const TCHAR* Names[] = { TEXT("trapped"), TEXT("waiting"), TEXT("following"), TEXT("carried"), TEXT("sheltered"), TEXT("aboard") };
        D->FireEvent(FName(*FString::Printf(TEXT("%s_%s"), *Id.ToString(), Names[(int32)S])));
    }
}

void ACSRescuee::Follow(AActor* L)       { Leader = L; SetState(ECSRescueeState::Following); }
void ACSRescuee::CarriedBy(AActor* C)    { Leader = C; SetState(ECSRescueeState::Carried); }
void ACSRescuee::ShelterAt(const FVector& W) { ShelterPoint = W; SetState(ECSRescueeState::Sheltered); }

void ACSRescuee::MoveTowards(const FVector& Where, float Accept)
{
    AAIController* C = Cast<AAIController>(GetController()); if (!C) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now < NextRepath) return; NextRepath = Now + 0.4;
    if (FVector::DistXY(GetActorLocation(), Where) <= Accept) { C->StopMovement(); return; }
    C->MoveToLocation(Where, Accept * 0.6f, true, true, false, true);
}

void ACSRescuee::Tick(float Dt)
{
    Super::Tick(Dt);
    if (Health <= 0.f) return;
    switch (State)
    {
    case ECSRescueeState::Following:
        if (AActor* L = Leader.Get()) MoveTowards(L->GetActorLocation() - L->GetActorForwardVector() * 180.f, 120.f);
        break;
    case ECSRescueeState::Carried:
        if (AActor* L = Leader.Get()) { GetCharacterMovement()->MaxWalkSpeed = 600.f; MoveTowards(L->GetActorLocation() - L->GetActorForwardVector() * 70.f, 50.f); }
        break;
    case ECSRescueeState::Sheltered:
        MoveTowards(ShelterPoint, 60.f);
        break;
    default: break;
    }

    if (bCanShoot && State != ECSRescueeState::Trapped && State != ECSRescueeState::Aboard)
    {
        const double Now = GetWorld()->GetTimeSeconds();
        if (Now >= NextShot)
        {
            ACSEnemy* Best = nullptr; float BestD = EngageRange;
            for (TActorIterator<ACSEnemy> It(GetWorld()); It; ++It)
            {
                if (It->IsDead()) continue;
                const float D = FVector::Dist(GetActorLocation(), It->GetActorLocation());
                if (D < BestD) { BestD = D; Best = *It; }
            }
            if (Best)
            {
                NextShot = Now + ShotInterval;
                FHitResult H; FCollisionQueryParams Q(SCENE_QUERY_STAT(RescueeShot), false, this);
                const FVector From = GetActorLocation() + FVector(0, 0, 60.f);
                if (!GetWorld()->LineTraceSingleByChannel(H, From, Best->EyeLocation(), ECC_Visibility, Q) || H.GetActor() == Best)
                {
                    if (FMath::FRand() < 0.45f) UGameplayStatics::ApplyDamage(Best, ShotDamage, GetController(), this, nullptr);
                    if (ACSMissionDirector* D = ACSMissionDirector::Get(this)) D->FireEvent(FName(*FString::Printf(TEXT("%s_shooting"), *Id.ToString())));
                }
            }
        }
    }
}

float ACSRescuee::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (Health <= 0.f || State == ECSRescueeState::Sheltered || State == ECSRescueeState::Aboard) return 0.f;
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    Health = FMath::Clamp(Health - Applied, 0.f, MaxHealth);
    if (Health <= 0.f)
    {
        Body->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
        if (ACSMissionDirector* D = ACSMissionDirector::Get(this))
        {
            D->FireEvent(FName(*FString::Printf(TEXT("%s_killed"), *Id.ToString())));
            if (bFailMissionOnDeath) D->Fail(FText::Format(NSLOCTEXT("CS", "RescueeLost", "{0} didn't make it."), DisplayName));
        }
    }
    return Applied;
}
