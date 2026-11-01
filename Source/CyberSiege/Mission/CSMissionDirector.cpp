#include "CSMissionDirector.h"
#include "CSMissionTrigger.h"
#include "CSCheckpointSave.h"
#include "CyberSiege.h"
#include "Squad/CSSquadMate.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

static const TCHAR* kCheckpointSlot = TEXT("checkpoint");

ACSMissionDirector::ACSMissionDirector()
{
    PrimaryActorTick.bCanEverTick = true;
}

ACSMissionDirector* ACSMissionDirector::Get(const UObject* WorldContext)
{
    UWorld* W = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!W) return nullptr;
    for (TActorIterator<ACSMissionDirector> It(W); It; ++It) return *It;
    return nullptr;
}

void ACSMissionDirector::BeginPlay()
{
    Super::BeginPlay();
    // Opened with ?checkpoint -> resume where the player last was. Any other open starts clean and clears the slot.
    // Start a beat later, once every listener (triggers, markers, waves, script, dialogue) has had its BeginPlay.
    FTimerHandle H;
    GetWorldTimerManager().SetTimer(H, [this]()
    {
        const FString Options = GetWorld()->GetAuthGameMode() ? GetWorld()->GetAuthGameMode()->OptionsString : FString();
        if (UGameplayStatics::HasOption(Options, TEXT("checkpoint")) && UGameplayStatics::DoesSaveGameExist(kCheckpointSlot, 0))
        {
            FastForwardToCheckpoint();
        }
        else
        {
            UGameplayStatics::DeleteGameInSlot(kCheckpointSlot, 0);
            if (Beats.Num() > 0) EnterBeat(0, false);
        }
    }, 0.2f, false);
}

void ACSMissionDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (ClockRemainingSeconds > 0.f)
    {
        ClockRemainingSeconds -= DeltaSeconds;
        if (ClockRemainingSeconds <= 0.f) { ClockRemainingSeconds = 0.f; const FName Ev = ClockEvent; ClockEvent = NAME_None; if (!Ev.IsNone()) FireEvent(Ev); }
    }
    if (!PendingConfirmEvent.IsNone() && ConfirmAutoAt > 0.f && GetWorld()->GetTimeSeconds() >= ConfirmAutoAt)
    {
        // Nobody pressed it: the system goes ahead on its own (Beat 6: "Confirming -- Ward, it's confirming").
        FireEvent(TEXT("confirm_auto"));
        Confirm();
    }
}

// ---------------------------------------------------------------------------------------------
FName ACSMissionDirector::CurrentBeat() const { return Beats.IsValidIndex(BeatIndex) ? Beats[BeatIndex].Name : NAME_None; }
FText ACSMissionDirector::CurrentObjective() const { return Beats.IsValidIndex(BeatIndex) ? Beats[BeatIndex].Objective : FText::GetEmpty(); }

bool ACSMissionDirector::IsBeatAtOrPast(FName BeatName) const
{
    for (int32 i = 0; i < Beats.Num(); i++) if (Beats[i].Name == BeatName) return BeatIndex >= i;
    return false;
}

void ACSMissionDirector::AdvanceTo(FName BeatName)
{
    for (int32 i = 0; i < Beats.Num(); i++)
        if (Beats[i].Name == BeatName) { if (i > BeatIndex) EnterBeat(i, false); return; }
    UE_LOG(LogCyberSiege, Warning, TEXT("[MISSION] AdvanceTo: no beat named %s"), *BeatName.ToString());
}

void ACSMissionDirector::AdvanceNext()
{
    if (Beats.IsValidIndex(BeatIndex + 1)) EnterBeat(BeatIndex + 1, false);
}

void ACSMissionDirector::EnterBeat(int32 Index, bool bFromCheckpoint)
{
    if (bFailed) return;
    BeatIndex = Index;
    const FCSBeat& B = Beats[Index];
    UE_LOG(LogCyberSiege, Log, TEXT("[MISSION] beat %d: %s%s"), Index, *B.Name.ToString(), bFromCheckpoint ? TEXT(" (checkpoint)") : TEXT(""));
    if (B.bCheckpoint) WriteCheckpoint();
    OnBeat.Broadcast(B.Name);
    FireEvent(FName(*FString::Printf(TEXT("beat_%s"), *B.Name.ToString())));
    if (!B.OnEnterEvent.IsNone()) FireEvent(B.OnEnterEvent);
}

void ACSMissionDirector::FireEvent(FName EventName)
{
    if (EventName.IsNone()) return;
    const bool bNew = !FiredEvents.Contains(EventName);
    FiredEvents.Add(EventName);
    UE_LOG(LogCyberSiege, Verbose, TEXT("[MISSION] event %s%s"), *EventName.ToString(), bNew ? TEXT("") : TEXT(" (again)"));
    OnEvent.Broadcast(EventName);
}

// ---------------------------------------------------------------------------------------------
void ACSMissionDirector::StartClock(float Seconds, FName EventOnDone)
{
    ClockRemainingSeconds = FMath::Max(0.01f, Seconds);
    ClockEvent = EventOnDone;
}

void ACSMissionDirector::RequestConfirm(const FText& Prompt, FName EventOnConfirm, float AutoConfirmAfter)
{
    ConfirmPrompt = Prompt; PendingConfirmEvent = EventOnConfirm;
    ConfirmAutoAt = AutoConfirmAfter > 0.f ? GetWorld()->GetTimeSeconds() + AutoConfirmAfter : 0.f;
}

void ACSMissionDirector::Confirm()
{
    if (PendingConfirmEvent.IsNone()) return;
    const FName Ev = PendingConfirmEvent; PendingConfirmEvent = NAME_None; ConfirmAutoAt = 0.f; ConfirmPrompt = FText::GetEmpty();
    FireEvent(Ev);
}

// ---------------------------------------------------------------------------------------------
void ACSMissionDirector::Fail(const FText& Reason)
{
    if (bFailed) return;
    bFailed = true; FailReason = Reason;
    UE_LOG(LogCyberSiege, Log, TEXT("[MISSION] failed: %s"), *Reason.ToString());
    FireEvent(TEXT("mission_failed"));
}

void ACSMissionDirector::RestartFromCheckpoint()
{
    UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)), true, TEXT("checkpoint"));
}

void ACSMissionDirector::WriteCheckpoint()
{
    UCSCheckpointSave* S = Cast<UCSCheckpointSave>(UGameplayStatics::CreateSaveGameObject(UCSCheckpointSave::StaticClass()));
    if (!S) return;
    S->MissionId = MissionId; S->BeatIndex = BeatIndex;
    if (const ACharacter* P = UGameplayStatics::GetPlayerCharacter(this, 0)) { S->PlayerLocation = P->GetActorLocation(); S->PlayerYaw = P->GetControlRotation().Yaw; }
    UGameplayStatics::SaveGameToSlot(S, kCheckpointSlot, 0);
}

void ACSMissionDirector::FastForwardToCheckpoint()
{
    UCSCheckpointSave* S = Cast<UCSCheckpointSave>(UGameplayStatics::LoadGameFromSlot(kCheckpointSlot, 0));
    if (!S || S->MissionId != MissionId || !Beats.IsValidIndex(S->BeatIndex)) { if (Beats.Num() > 0) EnterBeat(0, false); return; }

    // Everything before the checkpoint beat is treated as done: events fired silently (so dialogue cut rules
    // see them and triggers stay consumed), no lines, no checkpoints rewritten.
    for (int32 i = 0; i < S->BeatIndex; i++)
    {
        FiredEvents.Add(FName(*FString::Printf(TEXT("beat_%s"), *Beats[i].Name.ToString())));
        if (!Beats[i].OnEnterEvent.IsNone()) FiredEvents.Add(Beats[i].OnEnterEvent);
    }
    for (TActorIterator<ACSMissionTrigger> It(GetWorld()); It; ++It) It->NotifyFastForward(this, S->BeatIndex);

    // Put the player where the checkpoint beat begins: a PlayerStart tagged with the beat's name wins; the saved spot otherwise.
    const FName BeatName = Beats[S->BeatIndex].Name;
    FVector Loc = S->PlayerLocation; float Yaw = S->PlayerYaw; bool bFound = false;
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
        if (It->PlayerStartTag == BeatName) { Loc = It->GetActorLocation(); Yaw = It->GetActorRotation().Yaw; bFound = true; break; }
    if (ACharacter* P = UGameplayStatics::GetPlayerCharacter(this, 0))
    {
        P->SetActorLocation(Loc + FVector(0, 0, 4.f), false, nullptr, ETeleportType::TeleportPhysics);
        if (AController* C = P->GetController()) C->SetControlRotation(FRotator(0.f, Yaw, 0.f));
        // the squad arrives with him
        for (ACSSquadMate* M : SquadMates) if (M) M->TeleportBeside(Loc, Yaw);
    }
    UE_LOG(LogCyberSiege, Log, TEXT("[MISSION] resumed at beat %s (%s)"), *BeatName.ToString(), bFound ? TEXT("tagged start") : TEXT("saved spot"));
    EnterBeat(S->BeatIndex, true);
    FireEvent(TEXT("resumed"));   // lines that only make sense after a death ("Ward. Not straight at it.")
}

// ---------------------------------------------------------------------------------------------
void ACSMissionDirector::RegisterSquadMate(ACSSquadMate* Mate) { if (Mate) SquadMates.AddUnique(Mate); }

ACSSquadMate* ACSMissionDirector::FindSquadMate(FName Callsign) const
{
    for (ACSSquadMate* M : SquadMates) if (M && M->Callsign == Callsign) return M;
    return nullptr;
}
