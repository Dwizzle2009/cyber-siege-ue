#include "CSBreachWall.h"
#include "CSSquadMate.h"
#include "Mission/CSMissionDirector.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ACSBreachWall::ACSBreachWall()
{
    PrimaryActorTick.bCanEverTick = false;
    Panel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Panel"));
    RootComponent = Panel;
    Panel->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Panel->SetCollisionObjectType(ECC_WorldStatic);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) Panel->SetStaticMesh(Cube.Object);
    Tags.Add(TEXT("Breachable"));
}

void ACSBreachWall::OnConstruction(const FTransform&)
{
    if (Panel) Panel->SetRelativeScale3D(PanelExtent / 50.f);   // the engine cube is 100 cm
}

float ACSBreachWall::FuseRemaining() const
{
    return IsArmed() ? FMath::Max(0.f, FuseSeconds - (GetWorld()->GetTimeSeconds() - ArmedAt)) : 0.f;
}

void ACSBreachWall::BeginCountdown(ACSSquadMate*)
{
    if (bBreached || ArmedAt > 0.f) return;
    ArmedAt = GetWorld()->GetTimeSeconds();
    if (ACSMissionDirector* D = ACSMissionDirector::Get(this)) D->FireEvent(FName(*(EventName.ToString() + TEXT("_armed"))));
}

void ACSBreachWall::Detonate(ACSSquadMate* By)
{
    if (bBreached) return;
    bBreached = true;
    // Blast on the far side only: the squad stands on the near side by construction.
    const FVector Center = GetActorLocation() + GetActorForwardVector() * (BlastRadius * 0.5f);
    TArray<AActor*> Ignore; Ignore.Add(this); if (By) Ignore.Add(By);
    UGameplayStatics::ApplyRadialDamage(this, BlastDamage, Center, BlastRadius, nullptr, Ignore, this, By ? By->GetController() : nullptr, true);
    Panel->SetVisibility(false);
    Panel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (ACSMissionDirector* D = ACSMissionDirector::Get(this)) D->FireEvent(FName(*(EventName.ToString() + TEXT("_breached"))));
}
