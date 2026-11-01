#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/DataTable.h"
#include "Sound/SoundBase.h"
#include "CSDialogueSubsystem.generated.h"

class ACSMissionDirector;

/**
 * One row of the mission's dialogue table (Content/Data/<mission>_Lines.csv, imported as a DataTable).
 * Scripted lines are keyed by Event: when the mission fires that event every line with that Event plays in
 * row order, each after its Delay. Barks are keyed by Bark (the trigger name) and picked at random among
 * the variants; they yield to scripted lines.
 */
USTRUCT(BlueprintType)
struct FCSDialogueLine : public FTableRowBase
{
    GENERATED_BODY()
    /** Who speaks: alex, bell, chen, mercer, reyes, halvorsen, okafor, rourke. Drives the subtitle color and which mouth moves. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Speaker;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Text;
    /** Scripted: the mission event that starts this line. Empty for barks. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Event;
    /** Seconds after the event (or after the previous line of the same event when bChain) before it plays. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Delay = 0.f;
    /** Chain: Delay counts from the end of the previous line in the same event, so the exchange plays as a conversation. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bChain = true;
    /** Skip this line if the named event has already fired (the player outran it). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName CutIf;
    /** Play this line only if the named event HAS fired (branches like "if Mercer holds east"). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName OnlyIf;
    /** Bark trigger name (contact_n, heavy, low_ammo ...). Set for barks; Event is then empty. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Bark;
    /** 0 = computed from the text length (reading speed), otherwise seconds. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Duration = 0.f;
    /** Fire this mission event when the line finishes (lets a conversation gate the next thing). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName FireOnEnd;
    /** Voice clip; temporary TTS to begin with. Resolved by name under /Game/Audio/VO/<mission>/<RowName>. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<USoundBase> Voice;
};

/** What the HUD draws right now. */
struct FCSActiveLine
{
    FName Speaker; FText Text; float EndsAt = 0.f; bool bBark = false;
};

/**
 * Plays scripted lines and barks. Rules (from the script tab): scripted lines are never interrupted; a bark
 * waits three seconds after any scripted line and is dropped if another bark is already up; the same
 * bark trigger can't fire twice within its cooldown; a line whose CutIf event has fired is skipped.
 */
UCLASS()
class CYBERSIEGE_API UCSDialogueSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UCSDialogueSubsystem, STATGROUP_Tickables); }

    /** The mission's table. The mission director's level sets it (bootstrap does), or it's found at /Game/Data/<MissionId>_Lines. */
    UFUNCTION(BlueprintCallable, Category="Dialogue") void LoadTable(UDataTable* Table);

    /** A bark request from gameplay. Speaker may be None to let the row decide. Returns true if it will play. */
    UFUNCTION(BlueprintCallable, Category="Dialogue") bool Bark(FName Trigger, float Cooldown = 6.f);

    /** Queue a scripted exchange by hand (used by the base scene's talk points). */
    UFUNCTION(BlueprintCallable, Category="Dialogue") void PlayEvent(FName Event);

    const FCSActiveLine* Current() const { return Active.EndsAt > 0.f ? &Active : nullptr; }
    bool IsScriptedPlaying() const { return Active.EndsAt > 0.f && !Active.bBark; }
    bool IsQueueBusy() const { return Queue.Num() > 0 || IsScriptedPlaying(); }

private:
    struct FQueued { FName Row; const FCSDialogueLine* Line; float ReadyAt; bool bChainFromPrev; };
    TArray<FQueued> Queue;
    FCSActiveLine Active;
    float LastScriptedEnd = -100.f;
    TMap<FName, float> BarkCooldownUntil;
    UPROPERTY() UDataTable* Table = nullptr;
    TWeakObjectPtr<ACSMissionDirector> Director;

    UFUNCTION() void OnMissionEvent(FName EventName);
    void Start(const FQueued& Q);
    float DurationFor(const FCSDialogueLine& L) const;
};
