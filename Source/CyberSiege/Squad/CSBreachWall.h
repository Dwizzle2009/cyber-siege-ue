#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CSBreachWall.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class ACSSquadMate;

/**
 * A wall (or door) Mercer can blow. The player aims at it and issues Breach; Mercer walks to the stand point,
 * counts down, the panel vanishes and anyone within BlastRadius on the far side takes damage. Fires
 * <EventName>_armed and <EventName>_breached on the mission so the script can react.
 */
UCLASS()
class CYBERSIEGE_API ACSBreachWall : public AActor
{
    GENERATED_BODY()
public:
    ACSBreachWall();

    UPROPERTY(EditAnywhere, Category="Breach") FName EventName = TEXT("breach");
    UPROPERTY(EditAnywhere, Category="Breach") float FuseSeconds = 6.f;
    UPROPERTY(EditAnywhere, Category="Breach") float BlastRadius = 450.f;
    UPROPERTY(EditAnywhere, Category="Breach") float BlastDamage = 120.f;
    UPROPERTY(EditAnywhere, Category="Breach") FVector PanelExtent = FVector(20.f, 160.f, 140.f);
    /** Which side the squad stands on, in local space (the blast goes the other way). */
    UPROPERTY(EditAnywhere, Category="Breach") float StandDistance = 160.f;

    bool IsBreached() const { return bBreached; }
    bool IsArmed() const { return ArmedAt > 0.f && !bBreached; }
    float FuseRemaining() const;
    FVector StandPoint() const { return GetActorLocation() - GetActorForwardVector() * StandDistance; }

    void BeginCountdown(ACSSquadMate* By);
    void Detonate(ACSSquadMate* By);

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Panel;
    bool bBreached = false;
    float ArmedAt = 0.f;
};
