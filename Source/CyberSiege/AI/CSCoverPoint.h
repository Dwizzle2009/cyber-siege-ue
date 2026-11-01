#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CSCoverPoint.generated.h"

/**
 * A spot an enemy can fight from. Forward = the direction the cover protects from (the open side faces
 * away). Placed by the bootstrap next to every cover block, and by hand in real levels.
 */
UCLASS()
class CYBERSIEGE_API ACSCoverPoint : public AActor
{
    GENERATED_BODY()
public:
    ACSCoverPoint() { PrimaryActorTick.bCanEverTick = false; RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root")); }
    TWeakObjectPtr<AActor> Occupant;
    bool IsTaken() const { return Occupant.IsValid(); }
    /** True when the thing at Threat is roughly in front of the cover (within 70 degrees of forward). */
    bool Protects(const FVector& Threat) const
    {
        FVector To = Threat - GetActorLocation(); To.Z = 0.f;
        return FVector::DotProduct(To.GetSafeNormal(), GetActorForwardVector()) > 0.34f;
    }
};
