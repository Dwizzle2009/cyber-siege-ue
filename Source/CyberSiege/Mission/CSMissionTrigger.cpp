#include "CSMissionTrigger.h"
#include "CSMissionDirector.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

ACSMissionTrigger::ACSMissionTrigger()
{
    PrimaryActorTick.bCanEverTick = true;
    Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
    Box->SetBoxExtent(Extent);
    Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Box->SetCollisionObjectType(ECC_WorldDynamic);
    Box->SetCollisionResponseToAllChannels(ECR_Ignore);
    Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Box->SetGenerateOverlapEvents(true);
    RootComponent = Box;
}

void ACSMissionTrigger::OnConstruction(const FTransform&) { if (Box) Box->SetBoxExtent(Extent); }

void ACSMissionTrigger::BeginPlay()
{
    Super::BeginPlay();
    Box->OnComponentBeginOverlap.AddDynamic(this, &ACSMissionTrigger::OnEnter);
    Box->OnComponentEndOverlap.AddDynamic(this, &ACSMissionTrigger::OnLeave);
    if (ACSMissionDirector* D = ACSMissionDirector::Get(this)) D->OnEvent.AddDynamic(this, &ACSMissionTrigger::OnMissionEvent);
}

bool ACSMissionTrigger::IsLive() const
{
    if (bUsed && !bRepeatable) return false;
    if (RequireBeat.IsNone()) return true;
    const ACSMissionDirector* D = ACSMissionDirector::Get(this);
    return D && D->IsBeatAtOrPast(RequireBeat);
}

void ACSMissionTrigger::OnEnter(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    if (Other != UGameplayStatics::GetPlayerPawn(this, 0)) return;
    bPlayerInside = true; InsideSince = GetWorld()->GetTimeSeconds();
    if (!IsLive()) return;
    switch (Kind)
    {
    case ECSTriggerKind::AdvanceBeat:
    case ECSTriggerKind::FireEvent: Fire(); break;
    case ECSTriggerKind::EventAndInside: { const ACSMissionDirector* D = ACSMissionDirector::Get(this); if (D && D->HasFired(WaitFor)) Fire(); break; }
    default: break;   // Dwell fires from Tick
    }
}

void ACSMissionTrigger::OnLeave(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32)
{
    if (Other == UGameplayStatics::GetPlayerPawn(this, 0)) bPlayerInside = false;
}

void ACSMissionTrigger::OnMissionEvent(FName EventName)
{
    if (Kind == ECSTriggerKind::EventAndInside && EventName == WaitFor && bPlayerInside && IsLive()) Fire();
}

void ACSMissionTrigger::Tick(float)
{
    if (Kind != ECSTriggerKind::Dwell || !bPlayerInside || !IsLive()) return;
    if (GetWorld()->GetTimeSeconds() - InsideSince >= DwellSeconds) { Fire(); InsideSince = GetWorld()->GetTimeSeconds(); }
}

void ACSMissionTrigger::Fire()
{
    ACSMissionDirector* D = ACSMissionDirector::Get(this);
    if (!D) return;
    bUsed = true;
    if (Kind == ECSTriggerKind::AdvanceBeat) D->AdvanceTo(Beat); else D->FireEvent(Event);
}

void ACSMissionTrigger::NotifyFastForward(ACSMissionDirector* Director, int32 CheckpointBeatIndex)
{
    // A trigger that advances to a beat the checkpoint is already past has done its job.
    if (Kind == ECSTriggerKind::AdvanceBeat && Director)
    {
        for (int32 i = 0; i < Director->Beats.Num(); i++)
            if (Director->Beats[i].Name == Beat) { if (i <= CheckpointBeatIndex) bUsed = true; return; }
    }
}
