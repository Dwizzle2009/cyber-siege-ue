#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AeonSDKSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FAeonLeaderboardEntry
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Aeon") int32 Rank = 0;
    UPROPERTY(BlueprintReadOnly, Category="Aeon") FString Gamertag;
    UPROPERTY(BlueprintReadOnly, Category="Aeon") int64 Score = 0;
    UPROPERTY(BlueprintReadOnly, Category="Aeon") bool bIsMe = false;
};

DECLARE_DYNAMIC_DELEGATE_FourParams(FAeonLeaderboardResult, const TArray<FAeonLeaderboardEntry>&, Entries, bool, bHasMe, int32, MyRank, int64, MyScore);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FAeonPurchaseResult, const FString&, Sku, bool, bCompleted);

/**
 * The Aeon SDK as a Game Instance subsystem. Initializes from the environment the console provides
 * at launch; in the editor and on non-Linux platforms every call succeeds silently (IsOnAeon() is false).
 * Reports (unlocks, progress, scores, events) are queued, retried, and survive offline play.
 */
UCLASS()
class AEONSDK_API UAeonSDKSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** True when running on an Aeon console with a live session. */
    UFUNCTION(BlueprintPure, Category="Aeon") bool IsOnAeon() const { return bOnAeon; }
    UFUNCTION(BlueprintPure, Category="Aeon") FString GetGamertag() const;
    UFUNCTION(BlueprintPure, Category="Aeon") FString GetPlayerId() const;
    UFUNCTION(BlueprintPure, Category="Aeon") FString GetLanguage() const;
    UFUNCTION(BlueprintPure, Category="Aeon") FString GetSDKVersion() const;

    /** Unlock an achievement by its id (as defined for this title in the Aeon developer portal). Idempotent. */
    UFUNCTION(BlueprintCallable, Category="Aeon|Achievements") void UnlockAchievement(const FString& Id);
    /** Report progress toward an incremental achievement; unlocks when Value >= Max. */
    UFUNCTION(BlueprintCallable, Category="Aeon|Achievements") void AchievementProgress(const FString& Id, int32 Value, int32 Max);

    /** Submit a score; the server keeps the player's best. Metadata is optional JSON. */
    UFUNCTION(BlueprintCallable, Category="Aeon|Leaderboards") void SubmitScore(const FString& Category, int64 Score, const FString& MetadataJson = TEXT(""));
    /** Fetch the top N (optionally friends only) with the player's own rank. Result arrives on the game thread. */
    UFUNCTION(BlueprintCallable, Category="Aeon|Leaderboards") void GetLeaderboard(const FString& Category, int32 Count, bool bFriendsOnly, FAeonLeaderboardResult OnResult);

    /** Does the player own this SKU (DLC / content pack)? Cached; refreshed in the background. Off-Aeon: always true. */
    UFUNCTION(BlueprintPure, Category="Aeon|Entitlements") bool Owns(const FString& Sku) const;
    /** Ask the console to sell this SKU. The console shows the purchase UI; the game never does. */
    UFUNCTION(BlueprintCallable, Category="Aeon|Store") void Purchase(const FString& Sku, FAeonPurchaseResult OnResult);

    /** Rich presence shown to friends ("Lap 3 of 5 - Neon Bay"). */
    UFUNCTION(BlueprintCallable, Category="Aeon|Presence") void SetPresence(const FString& Status);
    /** Telemetry the studio opted into. Payload is optional JSON. */
    UFUNCTION(BlueprintCallable, Category="Aeon|Telemetry") void SendEvent(const FString& Name, const FString& PayloadJson = TEXT(""));

private:
    bool Tick(float DeltaTime);
    FTSTicker::FDelegateHandle TickHandle;
    bool bOnAeon = false;
    struct FPendingLb { FAeonLeaderboardResult Delegate; };
    struct FPendingPurchase { FAeonPurchaseResult Delegate; };
    static void OnLeaderboard(const struct AeonLeaderboard* Lb, void* User);
    static void OnPurchase(const char* Sku, int Completed, void* User);
};
