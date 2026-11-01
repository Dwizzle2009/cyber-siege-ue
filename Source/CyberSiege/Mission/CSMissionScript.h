#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CSMissionScript.generated.h"

UENUM(BlueprintType)
enum class ECSRuleAction : uint8
{
    FireEvent,          // Name = event; Seconds = delay (0 = now)
    AdvanceBeat,        // Name = beat
    StartClock,         // Seconds = length; Name = event when it ends
    SquadScripted,      // Who = callsign (None = all); Name = tag of the actor to stand at
    SquadFollow,        // Who = callsign (None = all)
    SquadCarry,         // Who = callsign; Name = rescuee id
    RescueeState,       // Name = rescuee id; Who = state (trapped/waiting/following/carried/sheltered/aboard)
    RescueeFollow,      // Name = rescuee id; Who = callsign to follow ("player" = the player)
    RescueeShelter,     // Name = rescuee id; Who = tag of the actor marking the shelter spot
    KillEnemiesFrom,    // Name = approach tag: every live enemy from it dies (the strike)
    Fail,               // Text = reason
    Presence,           // Text = what friends see
    Achievement,        // Name = achievement id
    Objective,          // Text = replaces the current objective line until the next beat
    EnemyPush,          // Name = actor tag every live enemy should push toward
    HideTagged,         // Name = actor tag: hidden + no collision (a door opens, a wall is gone)
    ShowTagged,         // Name = actor tag: shown + collision (rubble appears)
    SquadHold,          // Who = callsign (None = all); Name = tag of the spot
    SquadMoveTo,        // Who = callsign (None = all); Name = tag of the spot
    SquadBreach,        // Who = callsign (mercer); Name = tag of the ACSBreachWall
    TeleportPlayerTo,   // Name = tag of the spot (the base scene); the squad comes along
};

USTRUCT(BlueprintType)
struct FCSRule
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FName OnEvent;
    UPROPERTY(EditAnywhere) ECSRuleAction Action = ECSRuleAction::FireEvent;
    UPROPERTY(EditAnywhere) FName Name;
    UPROPERTY(EditAnywhere) FName Who;
    UPROPERTY(EditAnywhere) float Seconds = 0.f;
    UPROPERTY(EditAnywhere) FText Text;
    /** Only once, even if the event fires again. */
    UPROPERTY(EditAnywhere) bool bOnce = true;
    /** Run only if this event has fired / has not fired. */
    UPROPERTY(EditAnywhere) FName OnlyIf;
    UPROPERTY(EditAnywhere) FName CutIf;
};

/**
 * "When X happens, do Y." The mission's logic as a table, filled by the bootstrap (and editable in the
 * Details panel). Keeps the director generic and Mission 1 a data file.
 */
UCLASS()
class CYBERSIEGE_API ACSMissionScript : public AActor
{
    GENERATED_BODY()
public:
    ACSMissionScript();
    virtual void BeginPlay() override;
    UPROPERTY(EditAnywhere, Category="Script") TArray<FCSRule> Rules;
private:
    UFUNCTION() void OnMissionEvent(FName EventName);
    void Run(const FCSRule& R);
    TSet<int32> Used;
};
