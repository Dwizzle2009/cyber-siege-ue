#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CSMissionTrigger.generated.h"

class UBoxComponent;
class ACSMissionDirector;

UENUM(BlueprintType)
enum class ECSTriggerKind : uint8
{
    /** The player walks in: advance the mission to Beat. */
    AdvanceBeat,
    /** The player walks in: fire Event (dialogue, squad orders, wave director all listen). */
    FireEvent,
    /** Fires Event when the player has been inside for DwellSeconds without leaving (the idle nags). */
    Dwell,
    /** Fires Event when a mission event named WaitFor has fired AND the player is inside. */
    EventAndInside,
};

/**
 * A box in the level that talks to the mission director. Placed by the bootstrap script or by hand.
 * Fires once unless bRepeatable. Can be gated on a beat (only live while the mission is at or past
 * RequireBeat) so the same courtyard can mean different things on the way in and the way out.
 */
UCLASS()
class CYBERSIEGE_API ACSMissionTrigger : public AActor
{
    GENERATED_BODY()
public:
    ACSMissionTrigger();

    UPROPERTY(EditAnywhere, Category="Trigger") ECSTriggerKind Kind = ECSTriggerKind::FireEvent;
    UPROPERTY(EditAnywhere, Category="Trigger") FName Beat;          // AdvanceBeat
    UPROPERTY(EditAnywhere, Category="Trigger") FName Event;         // FireEvent / Dwell / EventAndInside
    UPROPERTY(EditAnywhere, Category="Trigger") FName WaitFor;       // EventAndInside
    UPROPERTY(EditAnywhere, Category="Trigger") float DwellSeconds = 10.f;
    UPROPERTY(EditAnywhere, Category="Trigger") FName RequireBeat;   // live only at/after this beat (None = always)
    UPROPERTY(EditAnywhere, Category="Trigger") bool  bRepeatable = false;
    UPROPERTY(EditAnywhere, Category="Trigger") FVector Extent = FVector(300.f, 300.f, 200.f);

    /** Resume support: a one-shot trigger whose beat is behind the checkpoint counts as used. */
    void NotifyFastForward(ACSMissionDirector* Director, int32 CheckpointBeatIndex);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(VisibleAnywhere) UBoxComponent* Box;

    UFUNCTION() void OnEnter(UPrimitiveComponent* Comp, AActor* Other, UPrimitiveComponent* OtherComp, int32 Idx, bool bSweep, const FHitResult& Hit);
    UFUNCTION() void OnLeave(UPrimitiveComponent* Comp, AActor* Other, UPrimitiveComponent* OtherComp, int32 Idx);
    UFUNCTION() void OnMissionEvent(FName EventName);

    bool bUsed = false;
    bool bPlayerInside = false;
    float InsideSince = 0.f;
    bool IsLive() const;
    void Fire();
};
