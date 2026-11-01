#include "CSWaveDirector.h"
#include "CSEnemy.h"
#include "Mission/CSMissionDirector.h"
#include "Squad/CSSquadMate.h"
#include "CyberSiege.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

ACSWaveDirector::ACSWaveDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    EnemyClass = ACSEnemy::StaticClass();
}

void ACSWaveDirector::BeginPlay()
{
    Super::BeginPlay();
    if (ACSMissionDirector* D = ACSMissionDirector::Get(this)) D->OnEvent.AddDynamic(this, &ACSWaveDirector::OnMissionEvent);
}

void ACSWaveDirector::CollectPoints(const TArray<FName>& Approaches)
{
    WavePoints.Reset();
    for (TActorIterator<ACSSpawnPoint> It(GetWorld()); It; ++It)
        if (Approaches.Num() == 0 || Approaches.Contains(It->Approach)) WavePoints.Add(*It);
}

FName ACSWaveDirector::WeakestApproach(const TArray<FName>& Exclude) const
{
    // The approach whose spawn points are farthest from any squad member or the player.
    TArray<AActor*> Friends;
    if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0)) Friends.Add(P);
    for (TActorIterator<ACSSquadMate> It(GetWorld()); It; ++It) if (!It->IsDown()) Friends.Add(*It);
    TMap<FName, float> Nearest;
    for (TActorIterator<ACSSpawnPoint> It(GetWorld()); It; ++It)
    {
        if (Exclude.Contains(It->Approach)) continue;
        float D = BIG_NUMBER;
        for (AActor* F : Friends) D = FMath::Min(D, FVector::Dist(F->GetActorLocation(), It->GetActorLocation()));
        float& Cur = Nearest.FindOrAdd(It->Approach, 0.f); Cur = FMath::Max(Cur, D);
    }
    FName Best; float BestD = -1.f;
    for (auto& KV : Nearest) if (KV.Value > BestD) { BestD = KV.Value; Best = KV.Key; }
    return Best;
}

void ACSWaveDirector::OnMissionEvent(FName EventName)
{
    if (EventName == TEXT("mission_failed")) { StopAll(); return; }
    for (int32 i = 0; i < Waves.Num(); i++)
    {
        if (Waves[i].StartEvent != EventName) continue;
        const FCSWave& W = Waves[i];
        CurrentWave = i; bWaveActive = true; WaveEnemies.Reset();
        TArray<FName> Appr = W.Approaches;
        if (W.bAddWeakestApproach) { const FName Weak = WeakestApproach(Appr); if (!Weak.IsNone()) { Appr.Add(Weak); if (ACSMissionDirector* D = ACSMissionDirector::Get(this)) D->FireEvent(FName(*FString::Printf(TEXT("wave_from_%s"), *Weak.ToString()))); } }
        CollectPoints(Appr);
        LeftToSpawn = W.Riflemen + W.Heavies + W.Gunners;
        NextSpawnAt = GetWorld()->GetTimeSeconds();
        bHasPush = false;
        if (!W.PushTowardTag.IsNone())
        {
            TArray<AActor*> Found; UGameplayStatics::GetAllActorsWithTag(this, W.PushTowardTag, Found);
            if (Found.Num() > 0) { PushPoint = Found[0]->GetActorLocation(); bHasPush = true; }
        }
        UE_LOG(LogCyberSiege, Log, TEXT("[WAVES] wave %d on %s: %d enemies from %d points"), i, *EventName.ToString(), LeftToSpawn, WavePoints.Num());
        return;
    }
}

void ACSWaveDirector::SpawnOne()
{
    if (WavePoints.Num() == 0 || !EnemyClass) { LeftToSpawn = 0; return; }
    const FCSWave& W = Waves[CurrentWave];
    const int32 Spawned = (W.Riflemen + W.Heavies + W.Gunners) - LeftToSpawn;
    ECSEnemyType T = ECSEnemyType::Rifleman;
    if (Spawned < W.Gunners) T = ECSEnemyType::Gunner;                           // gunners first: they set up
    else if (Spawned >= W.Riflemen + W.Gunners) T = ECSEnemyType::Heavy;         // heavies last: the breaking point

    ACSSpawnPoint* P = WavePoints[FMath::RandHelper(WavePoints.Num())];
    FTransform X = P->GetActorTransform(); X.AddToTranslation(FVector(FMath::FRandRange(-120.f, 120.f), FMath::FRandRange(-120.f, 120.f), 96.f));
    ACSEnemy* E = GetWorld()->SpawnActorDeferred<ACSEnemy>(EnemyClass, X, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    if (!E) { LeftToSpawn--; return; }
    E->Type = T; E->Approach = P->Approach;
    if (bHasPush) E->SetObjective(PushPoint);
    E->FinishSpawning(X);
    WaveEnemies.Add(E);
    LeftToSpawn--;
}

int32 ACSWaveDirector::AliveCount() const
{
    int32 N = 0; for (const auto& E : WaveEnemies) if (E.IsValid() && !E->IsDead()) N++; return N;
}

void ACSWaveDirector::Tick(float)
{
    if (!bWaveActive) return;
    const float Now = GetWorld()->GetTimeSeconds();
    ACSMissionDirector* D = ACSMissionDirector::Get(this);
    const FCSWave& W = Waves[CurrentWave];

    if (LeftToSpawn > 0 && Now >= NextSpawnAt) { SpawnOne(); NextSpawnAt = Now + W.SecondsBetweenSpawns; }

    // An approach left uncovered for five seconds while enemies come from it: "East is open!"
    if (D)
    {
        TArray<AActor*> Friends;
        if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0)) Friends.Add(P);
        for (TActorIterator<ACSSquadMate> It(GetWorld()); It; ++It) if (!It->IsDown()) Friends.Add(*It);
        TSet<FName> Seen;
        const TArray<ACSSpawnPoint*> Points = WavePoints;   // copy: FireEvent below can re-enter OnMissionEvent -> CollectPoints -> WavePoints.Reset()
        for (ACSSpawnPoint* P : Points)
        {
            if (Seen.Contains(P->Approach)) continue; Seen.Add(P->Approach);
            bool bCovered = false;
            for (AActor* F : Friends) if (FVector::Dist(F->GetActorLocation(), P->GetActorLocation()) < CoverDistance) { bCovered = true; break; }
            float& Since = OpenSince.FindOrAdd(P->Approach, -1.f);
            if (bCovered) Since = -1.f;
            else if (Since < 0.f) Since = Now;
            else if (Now - Since > 5.f) { D->FireEvent(FName(*FString::Printf(TEXT("open_%s"), *P->Approach.ToString()))); Since = Now + 20.f; }
        }
        // Reached: an enemy within ReachedRadius of the push point
        if (bHasPush && !W.ReachedEvent.IsNone())
            for (const auto& E : WaveEnemies) if (E.IsValid() && !E->IsDead() && FVector::DistXY(E->GetActorLocation(), PushPoint) < W.ReachedRadius) { D->FireEvent(W.ReachedEvent); break; }
    }

    if (LeftToSpawn == 0 && AliveCount() == 0)
    {
        bWaveActive = false;
        if (D && !W.ClearedEvent.IsNone()) D->FireEvent(W.ClearedEvent);
        UE_LOG(LogCyberSiege, Log, TEXT("[WAVES] wave %d cleared"), CurrentWave);
    }
}

void ACSWaveDirector::StopAll()
{
    bWaveActive = false; LeftToSpawn = 0;
}
