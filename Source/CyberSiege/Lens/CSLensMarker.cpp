#include "CSLensMarker.h"
#include "Mission/CSMissionDirector.h"

ACSLensMarker::ACSLensMarker()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ACSLensMarker::BeginPlay()
{
    Super::BeginPlay();
    ACSMissionDirector* D = ACSMissionDirector::Get(this);
    if (D) D->OnEvent.AddDynamic(this, &ACSLensMarker::OnMissionEvent);
    bShown = ShowOnEvent.IsNone() || (D && D->HasFired(ShowOnEvent));
    if (bShown && D && !HideOnEvent.IsNone() && D->HasFired(HideOnEvent)) bShown = false;
    if (bShown && Kind == ECSMarkerKind::Strike && D && !bStrikeRequested) { bStrikeRequested = true; D->RequestConfirm(Label, StrikeEvent, StrikeAutoConfirm); }
}

void ACSLensMarker::OnMissionEvent(FName EventName)
{
    ACSMissionDirector* D = ACSMissionDirector::Get(this);
    if (EventName == ShowOnEvent && !bShown)
    {
        bShown = true;
        if (Kind == ECSMarkerKind::Strike && D && !bStrikeRequested) { bStrikeRequested = true; D->RequestConfirm(Label, StrikeEvent, StrikeAutoConfirm); }
    }
    if (EventName == HideOnEvent) bShown = false;
    if (Kind == ECSMarkerKind::Strike && EventName == StrikeEvent) bShown = false;
}
