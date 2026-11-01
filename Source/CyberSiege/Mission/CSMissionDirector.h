#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CSMissionDirector.generated.h"

class ACSMissionTrigger;
class ACSSquadMate;

/** One scripted stage of a mission. Beats run in order; triggers advance them. */
USTRUCT(BlueprintType)
struct FCSBeat
{
    GENERATED_BODY()
    /** Stable name used by triggers, dialogue rows and the HUD ("insertion", "block", ...). */
    UPROPERTY(EditAnywhere) FName Name;
    /** What the objective line reads while this beat is active. */
    UPROPERTY(EditAnywhere) FText Objective;
    /** Dialogue event fired when the beat begins (lines with Event == this play). Empty = none. */
    UPROPERTY(EditAnywhere) FName OnEnterEvent;
    /** A checkpoint is written when the beat begins. */
    UPROPERTY(EditAnywhere) bool bCheckpoint = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCSMissionEvent, FName, EventName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCSBeatChanged, FName, BeatName);

/**
 * The mission's spine. One per level. Holds the ordered beat list, the set of events that have
 * fired (so dialogue can cut lines the player has already outrun), the active checkpoint, and the
 * fail state. Everything else -- triggers, the wave director, squad AI, the lens -- talks to the
 * mission through FireEvent / AdvanceTo and listens on OnEvent / OnBeat. The director knows no
 * mission-specific logic; Mission 1 is data (the beat list in the level + the dialogue CSV) plus
 * triggers placed in the map.
 */
UCLASS()
class CYBERSIEGE_API ACSMissionDirector : public AActor
{
    GENERATED_BODY()
public:
    ACSMissionDirector();
    virtual void BeginPlay() override;

    static ACSMissionDirector* Get(const UObject* WorldContext);

    UPROPERTY(EditAnywhere, Category="Mission") FName MissionId = TEXT("m01");
    UPROPERTY(EditAnywhere, Category="Mission") TArray<FCSBeat> Beats;

    // ---- flow ----
    UFUNCTION(BlueprintCallable, Category="Mission") void AdvanceTo(FName BeatName);
    UFUNCTION(BlueprintCallable, Category="Mission") void AdvanceNext();
    UFUNCTION(BlueprintCallable, Category="Mission") void FireEvent(FName EventName);
    UFUNCTION(BlueprintPure,     Category="Mission") bool HasFired(FName EventName) const { return FiredEvents.Contains(EventName); }
    UFUNCTION(BlueprintPure,     Category="Mission") FName CurrentBeat() const;
    UFUNCTION(BlueprintPure,     Category="Mission") int32 CurrentBeatIndex() const { return BeatIndex; }
    UFUNCTION(BlueprintPure,     Category="Mission") FText CurrentObjective() const;
    UFUNCTION(BlueprintPure,     Category="Mission") bool IsBeatAtOrPast(FName BeatName) const;

    // ---- a named timer the HUD can show (Bell's four minutes) ----
    UFUNCTION(BlueprintCallable, Category="Mission") void StartClock(float Seconds, FName EventOnDone);
    UFUNCTION(BlueprintPure,     Category="Mission") float ClockRemaining() const { return ClockRemainingSeconds; }
    UFUNCTION(BlueprintPure,     Category="Mission") bool  ClockRunning() const { return ClockRemainingSeconds > 0.f; }

    // ---- fail / checkpoint ----
    UFUNCTION(BlueprintCallable, Category="Mission") void Fail(const FText& Reason);
    UFUNCTION(BlueprintCallable, Category="Mission") void RestartFromCheckpoint();
    UFUNCTION(BlueprintPure,     Category="Mission") bool IsFailed() const { return bFailed; }
    FText FailReason;

    // ---- the lens' pending confirm (Beat 6: the strike) ----
    UFUNCTION(BlueprintCallable, Category="Mission") void RequestConfirm(const FText& Prompt, FName EventOnConfirm, float AutoConfirmAfter);
    UFUNCTION(BlueprintCallable, Category="Mission") void Confirm();
    bool  ConfirmPending() const { return !PendingConfirmEvent.IsNone(); }
    FText ConfirmPrompt;

    UPROPERTY(BlueprintAssignable) FCSMissionEvent OnEvent;
    UPROPERTY(BlueprintAssignable) FCSBeatChanged  OnBeat;

    /** Squad mates register themselves so commands and dialogue can find them by callsign. */
    void RegisterSquadMate(ACSSquadMate* Mate);
    ACSSquadMate* FindSquadMate(FName Callsign) const;
    const TArray<ACSSquadMate*>& Squad() const { return SquadMates; }

protected:
    virtual void Tick(float DeltaSeconds) override;

private:
    int32 BeatIndex = -1;
    TSet<FName> FiredEvents;
    bool bFailed = false;

    float ClockRemainingSeconds = 0.f;
    FName ClockEvent;

    FName PendingConfirmEvent;
    float ConfirmAutoAt = 0.f;

    UPROPERTY() TArray<ACSSquadMate*> SquadMates;

    void EnterBeat(int32 Index, bool bFromCheckpoint);
    void WriteCheckpoint();
    void FastForwardToCheckpoint();
};
