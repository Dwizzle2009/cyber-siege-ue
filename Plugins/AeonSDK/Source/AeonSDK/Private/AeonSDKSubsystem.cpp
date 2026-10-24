#include "AeonSDKSubsystem.h"
#include "Containers/Ticker.h"
#if AEON_SDK_AVAILABLE
extern "C" {
#include "aeon.h"
}
#endif

DEFINE_LOG_CATEGORY_STATIC(LogAeonSDK, Log, All);

void UAeonSDKSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
#if AEON_SDK_AVAILABLE
    bOnAeon = aeon_init() == 1;
    UE_LOG(LogAeonSDK, Log, TEXT("libaeon %s -> %s"), UTF8_TO_TCHAR(aeon_sdk_version()), bOnAeon ? TEXT("on Aeon") : TEXT("off-Aeon (no-op)"));
    TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UAeonSDKSubsystem::Tick), 0.0f);
#else
    UE_LOG(LogAeonSDK, Log, TEXT("Aeon SDK: not available on this platform (editor/non-Linux) -- all calls are no-ops"));
#endif
}

void UAeonSDKSubsystem::Deinitialize()
{
#if AEON_SDK_AVAILABLE
    if (TickHandle.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    aeon_shutdown();
#endif
    Super::Deinitialize();
}

bool UAeonSDKSubsystem::Tick(float)
{
#if AEON_SDK_AVAILABLE
    aeon_tick();
#endif
    return true;
}

FString UAeonSDKSubsystem::GetGamertag() const
{
#if AEON_SDK_AVAILABLE
    return UTF8_TO_TCHAR(aeon_player()->gamertag);
#else
    return TEXT("Player");
#endif
}
FString UAeonSDKSubsystem::GetPlayerId() const
{
#if AEON_SDK_AVAILABLE
    return UTF8_TO_TCHAR(aeon_player()->id);
#else
    return TEXT("");
#endif
}
FString UAeonSDKSubsystem::GetLanguage() const
{
#if AEON_SDK_AVAILABLE
    return UTF8_TO_TCHAR(aeon_player()->lang);
#else
    return TEXT("en");
#endif
}
FString UAeonSDKSubsystem::GetSDKVersion() const
{
#if AEON_SDK_AVAILABLE
    return UTF8_TO_TCHAR(aeon_sdk_version());
#else
    return TEXT("1.0.0");
#endif
}

void UAeonSDKSubsystem::UnlockAchievement(const FString& Id)
{
#if AEON_SDK_AVAILABLE
    aeon_achievement_unlock(TCHAR_TO_UTF8(*Id));
#endif
}
void UAeonSDKSubsystem::AchievementProgress(const FString& Id, int32 Value, int32 Max)
{
#if AEON_SDK_AVAILABLE
    aeon_achievement_progress(TCHAR_TO_UTF8(*Id), Value, Max);
#endif
}
void UAeonSDKSubsystem::SubmitScore(const FString& Category, int64 Score, const FString& MetadataJson)
{
#if AEON_SDK_AVAILABLE
    aeon_leaderboard_submit(TCHAR_TO_UTF8(*Category), Score, MetadataJson.IsEmpty() ? nullptr : TCHAR_TO_UTF8(*MetadataJson));
#endif
}

void UAeonSDKSubsystem::OnLeaderboard(const AeonLeaderboard* Lb, void* User)
{
#if AEON_SDK_AVAILABLE
    FPendingLb* P = static_cast<FPendingLb*>(User);
    TArray<FAeonLeaderboardEntry> Entries;
    for (int i = 0; i < Lb->count; i++)
    {
        FAeonLeaderboardEntry E; E.Rank = Lb->entries[i].rank; E.Gamertag = UTF8_TO_TCHAR(Lb->entries[i].gamertag); E.Score = Lb->entries[i].score; E.bIsMe = Lb->entries[i].me != 0;
        Entries.Add(E);
    }
    P->Delegate.ExecuteIfBound(Entries, Lb->has_me != 0, Lb->my_rank, Lb->my_score);
    delete P;
#endif
}
void UAeonSDKSubsystem::GetLeaderboard(const FString& Category, int32 Count, bool bFriendsOnly, FAeonLeaderboardResult OnResult)
{
#if AEON_SDK_AVAILABLE
    FPendingLb* P = new FPendingLb{ OnResult };
    aeon_leaderboard_get(TCHAR_TO_UTF8(*Category), Count, bFriendsOnly ? 1 : 0, &UAeonSDKSubsystem::OnLeaderboard, P);
#else
    OnResult.ExecuteIfBound(TArray<FAeonLeaderboardEntry>(), false, 0, 0);
#endif
}

bool UAeonSDKSubsystem::Owns(const FString& Sku) const
{
#if AEON_SDK_AVAILABLE
    return aeon_owns(TCHAR_TO_UTF8(*Sku)) != 0;
#else
    return true;
#endif
}

void UAeonSDKSubsystem::OnPurchase(const char* Sku, int Completed, void* User)
{
#if AEON_SDK_AVAILABLE
    FPendingPurchase* P = static_cast<FPendingPurchase*>(User);
    P->Delegate.ExecuteIfBound(UTF8_TO_TCHAR(Sku), Completed != 0);
    delete P;
#endif
}
void UAeonSDKSubsystem::Purchase(const FString& Sku, FAeonPurchaseResult OnResult)
{
#if AEON_SDK_AVAILABLE
    FPendingPurchase* P = new FPendingPurchase{ OnResult };
    aeon_purchase(TCHAR_TO_UTF8(*Sku), &UAeonSDKSubsystem::OnPurchase, P);
#else
    OnResult.ExecuteIfBound(Sku, false);
#endif
}

void UAeonSDKSubsystem::SetPresence(const FString& Status)
{
#if AEON_SDK_AVAILABLE
    aeon_presence(TCHAR_TO_UTF8(*Status));
#endif
}
void UAeonSDKSubsystem::SendEvent(const FString& Name, const FString& PayloadJson)
{
#if AEON_SDK_AVAILABLE
    aeon_event(TCHAR_TO_UTF8(*Name), PayloadJson.IsEmpty() ? nullptr : TCHAR_TO_UTF8(*PayloadJson));
#endif
}
