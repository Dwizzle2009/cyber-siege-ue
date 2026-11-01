#include "CSDialogueSubsystem.h"
#include "Mission/CSMissionDirector.h"
#include "CyberSiege.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UCSDialogueSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    if (ACSMissionDirector* D = ACSMissionDirector::Get(&InWorld))
    {
        Director = D;
        D->OnEvent.AddDynamic(this, &UCSDialogueSubsystem::OnMissionEvent);
        if (!Table)
        {
            const FString Path = FString::Printf(TEXT("/Game/Data/%s_Lines.%s_Lines"), *D->MissionId.ToString(), *D->MissionId.ToString());
            Table = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *Path));
            UE_LOG(LogCyberSiege, Log, TEXT("[DIALOGUE] table %s: %s"), *Path, Table ? TEXT("loaded") : TEXT("missing -- no lines will play"));
        }
    }
}

void UCSDialogueSubsystem::LoadTable(UDataTable* InTable) { Table = InTable; }

float UCSDialogueSubsystem::DurationFor(const FCSDialogueLine& L) const
{
    if (L.Duration > 0.f) return L.Duration;
    if (USoundBase* S = L.Voice.Get()) return FMath::Max(0.8f, S->GetDuration() + 0.25f);
    // reading speed: ~15 characters a second, floor 1.4 s, plus a breath
    return FMath::Clamp(L.Text.ToString().Len() / 15.f + 0.6f, 1.4f, 9.f);
}

void UCSDialogueSubsystem::OnMissionEvent(FName EventName) { PlayEvent(EventName); }

void UCSDialogueSubsystem::PlayEvent(FName Event)
{
    if (!Table || Event.IsNone()) return;
    const float Now = GetWorld()->GetTimeSeconds();
    bool bFirst = true;
    for (const auto& It : Table->GetRowMap())
    {
        const FCSDialogueLine* L = reinterpret_cast<const FCSDialogueLine*>(It.Value);
        if (!L || L->Event != Event) continue;
        Queue.Add({ It.Key, L, Now + ((bFirst || !L->bChain) ? L->Delay : 0.f), L->bChain && !bFirst });
        bFirst = false;
    }
}

bool UCSDialogueSubsystem::Bark(FName Trigger, float Cooldown)
{
    if (!Table || Trigger.IsNone()) return false;
    const float Now = GetWorld()->GetTimeSeconds();
    if (Active.EndsAt > Now) return false;                           // something is up; barks never interrupt
    if (Now - LastScriptedEnd < 3.f) return false;                   // breathing room after a scripted line
    if (Queue.Num() > 0) return false;                               // a scripted exchange is about to start
    if (const float* Until = BarkCooldownUntil.Find(Trigger)) if (*Until > Now) return false;

    TArray<const FCSDialogueLine*> Variants;
    for (const auto& It : Table->GetRowMap())
    {
        const FCSDialogueLine* L = reinterpret_cast<const FCSDialogueLine*>(It.Value);
        if (L && L->Bark == Trigger) Variants.Add(L);
    }
    if (Variants.Num() == 0) return false;
    const FCSDialogueLine* Pick = Variants[FMath::RandHelper(Variants.Num())];
    BarkCooldownUntil.Add(Trigger, Now + Cooldown);
    Active = { Pick->Speaker, Pick->Text, Now + DurationFor(*Pick), true };
    if (USoundBase* S = Pick->Voice.LoadSynchronous()) UGameplayStatics::PlaySound2D(this, S);
    return true;
}

void UCSDialogueSubsystem::Start(const FQueued& Q)
{
    const FCSDialogueLine& L = *Q.Line;
    Active = { L.Speaker, L.Text, GetWorld()->GetTimeSeconds() + DurationFor(L), false };
    if (USoundBase* S = L.Voice.LoadSynchronous()) UGameplayStatics::PlaySound2D(this, S);
    UE_LOG(LogCyberSiege, Verbose, TEXT("[DIALOGUE] %s: %s"), *L.Speaker.ToString(), *L.Text.ToString());
}

void UCSDialogueSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UWorld* W = GetWorld(); if (!W) return;
    const float Now = W->GetTimeSeconds();

    if (Active.EndsAt > 0.f && Now >= Active.EndsAt)
    {
        if (!Active.bBark) LastScriptedEnd = Now;
        Active = FCSActiveLine{};
    }
    if (Active.EndsAt > 0.f && !Active.bBark) return;   // a scripted line is talking; wait
    if (Queue.Num() == 0) return;

    // Lines play in the order they were queued. A chained line times its Delay from the end of the line before it.
    FQueued& Q = Queue[0];
    if (Q.bChainFromPrev) { Q.ReadyAt = LastScriptedEnd + Q.Line->Delay; Q.bChainFromPrev = false; }
    if (Q.ReadyAt > Now) return;

    const FCSDialogueLine& L = *Q.Line; const FName Row = Q.Row;
    Queue.RemoveAt(0);
    ACSMissionDirector* D = Director.Get();
    const bool bCut  = D && !L.CutIf.IsNone()  && D->HasFired(L.CutIf);
    const bool bSkip = D && !L.OnlyIf.IsNone() && !D->HasFired(L.OnlyIf);
    if (bCut || bSkip) { if (D && !L.FireOnEnd.IsNone()) D->FireEvent(L.FireOnEnd); LastScriptedEnd = Now; return; }

    if (Active.bBark) Active = FCSActiveLine{};   // a bark yields
    Start({ Row, &L, Now, false });
    if (!L.FireOnEnd.IsNone())
    {
        FTimerHandle H; const FName Ev = L.FireOnEnd; TWeakObjectPtr<ACSMissionDirector> WD = Director;
        W->GetTimerManager().SetTimer(H, [WD, Ev]() { if (WD.IsValid()) WD->FireEvent(Ev); }, Active.EndsAt - Now, false);
    }
}
