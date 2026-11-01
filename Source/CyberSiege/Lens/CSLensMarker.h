#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CSLensMarker.generated.h"

UENUM(BlueprintType)
enum class ECSMarkerKind : uint8
{
    Objective,   // the wreck's beacon: always drawn, lens or not
    Route,       // a path AEGIS offers: a chain of points drawn as a line while the lens is up
    Threat,      // something AEGIS knows about that the player can't see (the mortar team)
    Strike,      // a threat with a strike package: shows the CONFIRM prompt while the lens is up
    Hold,        // a suggested hold position (Beat 4's three markers)
    Label,       // plain text in the world (names over the crew in the cabin)
};

/**
 * Something the AEGIS lens draws. Placed in the level; shown between two mission events. The HUD projects
 * it to the screen. A Route marker lists its points; the others are a single spot.
 */
UCLASS()
class CYBERSIEGE_API ACSLensMarker : public AActor
{
    GENERATED_BODY()
public:
    ACSLensMarker();
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, Category="Lens") ECSMarkerKind Kind = ECSMarkerKind::Objective;
    UPROPERTY(EditAnywhere, Category="Lens") FText Label;
    UPROPERTY(EditAnywhere, Category="Lens") FName ShowOnEvent;    // None = from the start
    UPROPERTY(EditAnywhere, Category="Lens") FName HideOnEvent;
    /** Route only: world points in order; the actor's own location is the first. */
    UPROPERTY(EditAnywhere, Category="Lens") TArray<FVector> RoutePoints;
    /** Strike only: the mission event fired when the player confirms, and the seconds before AEGIS confirms alone (0 = never). */
    UPROPERTY(EditAnywhere, Category="Lens") FName StrikeEvent = TEXT("strike_confirmed");
    UPROPERTY(EditAnywhere, Category="Lens") float StrikeAutoConfirm = 15.f;
    /** Hold only: the squad mates this marker is for (any of them standing here counts it as taken). */
    UPROPERTY(EditAnywhere, Category="Lens") FName HoldName;

    bool IsShown() const { return bShown; }

private:
    UFUNCTION() void OnMissionEvent(FName EventName);
    bool bShown = false;
    bool bStrikeRequested = false;
};
