#include "CSTalkPoint.h"
#include "Mission/CSMissionDirector.h"
#include "Dialogue/CSDialogueSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

ACSTalkPoint::ACSTalkPoint()
{
    PrimaryActorTick.bCanEverTick = false;
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    RootComponent = Body;
    Body->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.7f));
    Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) Body->SetStaticMesh(Cube.Object);
}

void ACSTalkPoint::BeginPlay()
{
    Super::BeginPlay();
    ACSMissionDirector* D = ACSMissionDirector::Get(this);
    if (D) D->OnEvent.AddDynamic(this, &ACSTalkPoint::OnMissionEvent);
    bShown = ShowOnEvent.IsNone() || (D && D->HasFired(ShowOnEvent));
    SetActorHiddenInGame(!bShown); SetActorEnableCollision(bShown);
}

void ACSTalkPoint::OnMissionEvent(FName EventName)
{
    if (EventName == ShowOnEvent && !bShown) { bShown = true; SetActorHiddenInGame(false); SetActorEnableCollision(true); }
}

bool ACSTalkPoint::CanTalk() const
{
    if (!bShown || (bOnce && bTalked)) return false;
    const UCSDialogueSubsystem* Dlg = GetWorld()->GetSubsystem<UCSDialogueSubsystem>();
    return !Dlg || !Dlg->IsQueueBusy();
}

void ACSTalkPoint::Talk(AActor*)
{
    if (!CanTalk()) return;
    bTalked = true;
    if (ACSMissionDirector* D = ACSMissionDirector::Get(this)) D->FireEvent(Event);
    CheckAllRequired(GetWorld());
}

void ACSTalkPoint::CheckAllRequired(UWorld* W)
{
    for (TActorIterator<ACSTalkPoint> It(W); It; ++It) if (It->bRequired && !It->bTalked) return;
    if (ACSMissionDirector* D = ACSMissionDirector::Get(W)) D->FireEvent(TEXT("talks_done"));
}
