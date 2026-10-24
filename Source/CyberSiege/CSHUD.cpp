#include "CSHUD.h"
#include "CSPlayerCharacter.h"
#include "CSGameMode.h"
#include "AeonSDKSubsystem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"

void ACSHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas) return;

    const float W = Canvas->ClipX, H = Canvas->ClipY;
    const float S = H / 1080.f;   // scale everything against 1080p design
    UFont* Big = GEngine->GetLargeFont();
    UFont* Med = GEngine->GetMediumFont();
    UFont* Small = GEngine->GetSmallFont();

    const FLinearColor Teal(0.13f, 0.83f, 0.93f);
    const FLinearColor White(0.93f, 0.95f, 0.98f);
    const FLinearColor Mute(0.55f, 0.60f, 0.68f);
    const FLinearColor Red(0.95f, 0.30f, 0.30f);

    ACSPlayerCharacter* P = Cast<ACSPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    ACSGameMode* GM = GetWorld()->GetAuthGameMode<ACSGameMode>();

    // Crosshair
    if (P && !P->bDead)
    {
        const float cx = W * 0.5f, cy = H * 0.5f, g = 6.f * S, l = 10.f * S, t = 2.f * S;
        DrawLine(cx - g - l, cy, cx - g, cy, Teal, t);
        DrawLine(cx + g, cy, cx + g + l, cy, Teal, t);
        DrawLine(cx, cy - g - l, cx, cy - g, Teal, t);
        DrawLine(cx, cy + g, cx, cy + g + l, Teal, t);
    }

    // Top-left: wave + score
    if (GM)
    {
        DrawText(FString::Printf(TEXT("WAVE  %02d"), GM->Wave), Teal, 40.f * S, 34.f * S, Med, 1.4f * S);
        DrawText(FString::Printf(TEXT("SCORE  %06d"), GM->Score), White, 40.f * S, 70.f * S, Med, 1.1f * S);
    }

    // Bottom-left: integrity bar
    if (P)
    {
        const float bx = 40.f * S, by = H - 70.f * S, bw = 320.f * S, bh = 10.f * S;
        DrawText(TEXT("INTEGRITY"), Mute, bx, by - 24.f * S, Small, 1.1f * S);
        DrawRect(FLinearColor(1, 1, 1, 0.10f), bx, by, bw, bh);
        const float f = P->GetHealthNormalized();
        DrawRect(f > 0.35f ? Teal : Red, bx, by, bw * f, bh);

        // Bottom-right: ammo
        const FString Ammo = P->bReloading ? TEXT("RELOADING")
                           : FString::Printf(TEXT("%02d  /  %03d"), P->AmmoInMag, P->AmmoReserve);
        float tw = 0, th = 0; GetTextSize(Ammo, tw, th, Big, 1.2f * S);
        DrawText(Ammo, P->AmmoInMag == 0 && !P->bReloading ? Red : White, W - 40.f * S - tw, H - 84.f * S, Big, 1.2f * S);
    }

    // Center status
    if (GM && !GM->StatusText.IsEmpty())
    {
        float tw = 0, th = 0; GetTextSize(GM->StatusText, tw, th, Big, 1.6f * S);
        DrawText(GM->StatusText, White, (W - tw) * 0.5f, H * 0.36f, Big, 1.6f * S);
    }

    // Top-right: platform link
    if (UAeonSDKSubsystem* Aeon = GetGameInstance()->GetSubsystem<UAeonSDKSubsystem>())
    {
        const FString Link = Aeon->IsOnAeon() ? TEXT("AEON  LINK  ONLINE") : TEXT("AEON  LINK  OFFLINE");
        float tw = 0, th = 0; GetTextSize(Link, tw, th, Small, 1.0f * S);
        DrawText(Link, Aeon->IsOnAeon() ? Teal : Mute, W - 40.f * S - tw, 34.f * S, Small, 1.0f * S);
    }
}
