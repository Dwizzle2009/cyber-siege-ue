#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CSCheckpointSave.generated.h"

/** The one thing a restart needs: which beat, and where the player stood when it began. */
UCLASS()
class CYBERSIEGE_API UCSCheckpointSave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() FName   MissionId;
    UPROPERTY() int32   BeatIndex = 0;
    UPROPERTY() FVector PlayerLocation = FVector::ZeroVector;
    UPROPERTY() float   PlayerYaw = 0.f;
};
