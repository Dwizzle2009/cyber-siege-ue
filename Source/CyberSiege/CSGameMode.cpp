#include "CSGameMode.h"
#include "CSPlayerCharacter.h"
#include "CSEnemy.h"
#include "CSHUD.h"
#include "AeonSDKSubsystem.h"
#include "CyberSiege.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"

ACSGameMode::ACSGameMode()
{
    DefaultPawnClass = ACSPlayerCharacter::StaticClass();
    HUDClass = ACSHUD::StaticClass();
    EnemyClass = ACSEnemy::StaticClass();
}

void ACSGameMode::BeginPlay()
{
    Super::BeginPlay();
    // Tell the platform we're playing -- friends see "Playing Cyber Siege" on their consoles.
    if (UAeonSDKSubsystem* Aeon = GetGameInstance()->GetSubsystem<UAeonSDKSubsystem>()) Aeon->SetPresence(TEXT("In the arena"));

    StatusText = TEXT("SYSTEMS ONLINE");
    GetWorldTimerManager().SetTimer(NextWaveTimer, this, &ACSGameMode::StartWave, 1.5f, false);
}

void ACSGameMode::StartWave()
{
    if (bGameOver) return;
    Wave++;
    EnemiesRemainingToSpawn = 4 + Wave * 2;
    EnemiesAlive = 0;
    StatusText = FString::Printf(TEXT("WAVE %d"), Wave);
    if (UAeonSDKSubsystem* Aeon = GetGameInstance()->GetSubsystem<UAeonSDKSubsystem>()) { Aeon->AchievementProgress(TEXT("wave_10"), Wave, 10); Aeon->SetPresence(FString::Printf(TEXT("Wave %d"), Wave)); }
    GetWorldTimerManager().SetTimer(StatusTimer, this, &ACSGameMode::ClearStatus, 1.4f, false);

    const float Interval = FMath::Max(0.28f, TimeBetweenSpawns - Wave * 0.05f);
    GetWorldTimerManager().SetTimer(SpawnTimer, this, &ACSGameMode::SpawnOneEnemy, Interval, true, 0.2f);
}

void ACSGameMode::SpawnOneEnemy()
{
    if (bGameOver || EnemiesRemainingToSpawn <= 0) { GetWorldTimerManager().ClearTimer(SpawnTimer); return; }

    APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!Player || !EnemyClass) return;

    // Ring around the player, outside the immediate fight, snapped to the floor.
    const float Angle  = FMath::FRandRange(0.f, 2.f * PI);
    const float Radius = FMath::FRandRange(SpawnRadius * 0.7f, SpawnRadius);
    FVector Loc = Player->GetActorLocation() + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 200.f);
    FHitResult Floor;
    if (GetWorld()->LineTraceSingleByChannel(Floor, Loc + FVector(0, 0, 800.f), Loc - FVector(0, 0, 2000.f), ECC_Visibility))
        Loc.Z = Floor.Location.Z + 96.f;

    FActorSpawnParameters P;
    P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    if (GetWorld()->SpawnActor<ACSEnemy>(EnemyClass, Loc, FRotator::ZeroRotator, P))
    {
        EnemiesAlive++;
        EnemiesRemainingToSpawn--;
    }
}

void ACSGameMode::RegisterEnemyKill(AActor*, int32 Points)
{
    Kills++;
    Score += Points * FMath::Max(1, Wave);
    EnemiesAlive = FMath::Max(0, EnemiesAlive - 1);

    // First kill on the account: the backend has a "First Blood" achievement seeded for this SKU.
    if (Kills == 1)
        if (UAeonSDKSubsystem* Aeon = GetGameInstance()->GetSubsystem<UAeonSDKSubsystem>()) Aeon->UnlockAchievement(TEXT("first_kill"));

    if (EnemiesAlive == 0 && EnemiesRemainingToSpawn == 0 && !bGameOver)
        GetWorldTimerManager().SetTimer(NextWaveTimer, this, &ACSGameMode::StartWave, 2.0f, false);
}

void ACSGameMode::OnPlayerDied()
{
    if (bGameOver) return;
    bGameOver = true;
    GetWorldTimerManager().ClearTimer(SpawnTimer);
    GetWorldTimerManager().ClearTimer(NextWaveTimer);
    StatusText = FString::Printf(TEXT("SYSTEM BREACH\nWAVE %d   SCORE %06d\n\nFIRE / A  TO REBOOT"), Wave, Score);

    // The run is over: post the score to the Cyber Siege leaderboard.
    if (UAeonSDKSubsystem* Aeon = GetGameInstance()->GetSubsystem<UAeonSDKSubsystem>()) { Aeon->SubmitScore(TEXT("score"), (int64)Score); Aeon->SubmitScore(TEXT("waves"), (int64)Wave); Aeon->UnlockAchievement(TEXT("first_breach")); }
}

void ACSGameMode::RequestRestart()
{
    if (!bGameOver) return;
    UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}

void ACSGameMode::ClearStatus() { StatusText.Empty(); }
