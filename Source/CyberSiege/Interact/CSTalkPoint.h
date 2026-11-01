#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CSTalkPoint.generated.h"

class UStaticMeshComponent;

/**
 * Someone to talk to in a walkable scene (Beat 8). Aim at them, press interact, the exchange keyed by Event
 * plays. Required talk points gate the scene's exit: the director fires "talks_done" when every required
 * one has been used.
 */
UCLASS()
class CYBERSIEGE_API ACSTalkPoint : public AActor
{
    GENERATED_BODY()
public:
    ACSTalkPoint();
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, Category="Talk") FText DisplayName;
    UPROPERTY(EditAnywhere, Category="Talk") FName Event;          // dialogue rows with this Event play
    UPROPERTY(EditAnywhere, Category="Talk") bool  bRequired = false;
    UPROPERTY(EditAnywhere, Category="Talk") FName ShowOnEvent;    // None = always there
    UPROPERTY(EditAnywhere, Category="Talk") bool  bOnce = true;

    bool CanTalk() const;
    void Talk(AActor* By);

protected:
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Body;
    bool bTalked = false;
    bool bShown = true;
    UFUNCTION() void OnMissionEvent(FName EventName);
    static void CheckAllRequired(UWorld* W);
};
