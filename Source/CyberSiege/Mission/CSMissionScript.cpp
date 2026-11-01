#include "CSMissionScript.h"
#include "CSMissionDirector.h"
#include "Squad/CSSquadMate.h"
#include "Squad/CSRescuee.h"
#include "AI/CSEnemy.h"
#include "Squad/CSBreachWall.h"
#include "GameFramework/Character.h"
#include "AeonSDKSubsystem.h"
#include "CyberSiege.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ACSMissionScript::ACSMissionScript() { PrimaryActorTick.bCanEverTick = false; }

void ACSMissionScript::BeginPlay()
{
    Super::BeginPlay();
    if (ACSMissionDirector* D = ACSMissionDirector::Get(this)) D->OnEvent.AddDynamic(this, &ACSMissionScript::OnMissionEvent);
}

void ACSMissionScript::OnMissionEvent(FName EventName)
{
    for (int32 i = 0; i < Rules.Num(); i++)
    {
        if (Rules[i].OnEvent != EventName) continue;
        if (Rules[i].bOnce && Used.Contains(i)) continue;
        if (ACSMissionDirector* D = ACSMissionDirector::Get(this))
        {
            if (!Rules[i].OnlyIf.IsNone() && !D->HasFired(Rules[i].OnlyIf)) continue;
            if (!Rules[i].CutIf.IsNone()  &&  D->HasFired(Rules[i].CutIf))  continue;
        }
        Used.Add(i);
        Run(Rules[i]);
    }
}

static AActor* FindTagged(UWorld* W, FName Tag)
{
    if (Tag.IsNone()) return nullptr;
    TArray<AActor*> Found; UGameplayStatics::GetAllActorsWithTag(W, Tag, Found);
    return Found.Num() ? Found[0] : nullptr;
}

static ACSRescuee* FindRescuee(UWorld* W, FName Id)
{
    for (TActorIterator<ACSRescuee> It(W); It; ++It) if (It->Id == Id) return *It;
    return nullptr;
}

void ACSMissionScript::Run(const FCSRule& R)
{
    UWorld* W = GetWorld();
    ACSMissionDirector* D = ACSMissionDirector::Get(this); if (!D) return;
    UE_LOG(LogCyberSiege, Verbose, TEXT("[SCRIPT] %s -> %d %s %s"), *R.OnEvent.ToString(), (int32)R.Action, *R.Name.ToString(), *R.Who.ToString());

    auto ForSquad = [&](TFunctionRef<void(ACSSquadMate*)> Fn) { for (ACSSquadMate* M : D->Squad()) if (M && (R.Who.IsNone() || M->Callsign == R.Who)) Fn(M); };

    switch (R.Action)
    {
    case ECSRuleAction::FireEvent:
        if (R.Seconds <= 0.f) D->FireEvent(R.Name);
        else { FTimerHandle H; const FName Ev = R.Name; TWeakObjectPtr<ACSMissionDirector> WD = D; W->GetTimerManager().SetTimer(H, [WD, Ev]() { if (WD.IsValid()) WD->FireEvent(Ev); }, R.Seconds, false); }
        break;
    case ECSRuleAction::AdvanceBeat: D->AdvanceTo(R.Name); break;
    case ECSRuleAction::StartClock: D->StartClock(R.Seconds, R.Name); break;
    case ECSRuleAction::SquadScripted:
        if (AActor* At = FindTagged(W, R.Name)) ForSquad([&](ACSSquadMate* M) { M->OrderScripted(At->GetActorLocation(), At->GetActorRotation().Yaw); });
        break;
    case ECSRuleAction::SquadFollow: ForSquad([&](ACSSquadMate* M) { M->OrderFollow(); }); break;
    case ECSRuleAction::SquadCarry:
        if (ACSRescuee* Rs = FindRescuee(W, R.Name)) ForSquad([&](ACSSquadMate* M) { M->OrderCarry(Rs); Rs->CarriedBy(M); });
        break;
    case ECSRuleAction::RescueeState:
        if (ACSRescuee* Rs = FindRescuee(W, R.Name))
        {
            const FString St = R.Who.ToString();
            if (St == TEXT("trapped")) Rs->SetState(ECSRescueeState::Trapped); else if (St == TEXT("waiting")) Rs->SetState(ECSRescueeState::Waiting);
            else if (St == TEXT("following")) Rs->SetState(ECSRescueeState::Following); else if (St == TEXT("carried")) Rs->SetState(ECSRescueeState::Carried);
            else if (St == TEXT("sheltered")) Rs->SetState(ECSRescueeState::Sheltered); else if (St == TEXT("aboard")) Rs->SetState(ECSRescueeState::Aboard);
        }
        break;
    case ECSRuleAction::RescueeFollow:
        if (ACSRescuee* Rs = FindRescuee(W, R.Name))
        {
            AActor* L = R.Who == TEXT("player") ? (AActor*)UGameplayStatics::GetPlayerPawn(this, 0) : (AActor*)D->FindSquadMate(R.Who);
            if (L) Rs->Follow(L);
        }
        break;
    case ECSRuleAction::RescueeShelter:
        if (ACSRescuee* Rs = FindRescuee(W, R.Name)) if (AActor* At = FindTagged(W, R.Who)) Rs->ShelterAt(At->GetActorLocation());
        break;
    case ECSRuleAction::KillEnemiesFrom:
        for (TActorIterator<ACSEnemy> It(W); It; ++It) if (!It->IsDead() && (R.Name.IsNone() || It->Approach == R.Name)) UGameplayStatics::ApplyDamage(*It, 9999.f, nullptr, this, nullptr);
        break;
    case ECSRuleAction::HideTagged:
    case ECSRuleAction::ShowTagged:
    {
        const bool bShow = R.Action == ECSRuleAction::ShowTagged;
        TArray<AActor*> Found; UGameplayStatics::GetAllActorsWithTag(W, R.Name, Found);
        for (AActor* A : Found) { A->SetActorHiddenInGame(!bShow); A->SetActorEnableCollision(bShow); }
        break;
    }
    case ECSRuleAction::SquadHold:
        if (AActor* At = FindTagged(W, R.Name)) ForSquad([&](ACSSquadMate* M) { M->OrderHold(At->GetActorLocation()); });
        break;
    case ECSRuleAction::SquadMoveTo:
        if (AActor* At = FindTagged(W, R.Name)) ForSquad([&](ACSSquadMate* M) { M->OrderMoveTo(At->GetActorLocation()); });
        break;
    case ECSRuleAction::SquadBreach:
        if (ACSBreachWall* Wall = Cast<ACSBreachWall>(FindTagged(W, R.Name))) ForSquad([&](ACSSquadMate* M) { M->OrderBreach(Wall); });
        break;
    case ECSRuleAction::TeleportPlayerTo:
        if (AActor* At = FindTagged(W, R.Name))
        {
            if (ACharacter* P = UGameplayStatics::GetPlayerCharacter(this, 0))
            {
                P->SetActorLocation(At->GetActorLocation() + FVector(0, 0, 4.f), false, nullptr, ETeleportType::TeleportPhysics);
                if (AController* C = P->GetController()) C->SetControlRotation(FRotator(0.f, At->GetActorRotation().Yaw, 0.f));
            }
            for (ACSSquadMate* M : D->Squad()) if (M) M->TeleportBeside(At->GetActorLocation(), At->GetActorRotation().Yaw);
        }
        break;
    case ECSRuleAction::Fail: D->Fail(R.Text); break;
    case ECSRuleAction::Presence:
        if (UAeonSDKSubsystem* Aeon = GetGameInstance()->GetSubsystem<UAeonSDKSubsystem>()) Aeon->SetPresence(R.Text.ToString());
        break;
    case ECSRuleAction::Achievement:
        if (UAeonSDKSubsystem* Aeon = GetGameInstance()->GetSubsystem<UAeonSDKSubsystem>()) Aeon->UnlockAchievement(R.Name.ToString());
        break;
    case ECSRuleAction::Objective:
        if (D->Beats.IsValidIndex(D->CurrentBeatIndex())) D->Beats[D->CurrentBeatIndex()].Objective = R.Text;
        break;
    case ECSRuleAction::EnemyPush:
        if (AActor* At = FindTagged(W, R.Name)) for (TActorIterator<ACSEnemy> It(W); It; ++It) if (!It->IsDead()) It->SetObjective(At->GetActorLocation());
        break;
    }
}
