#include "CSHUD.h"
#include "CSPlayerCharacter.h"
#include "CSGameMode.h"
#include "AeonSDKSubsystem.h"
#include "Mission/CSMissionDirector.h"
#include "Dialogue/CSDialogueSubsystem.h"
#include "Squad/CSSquadMate.h"
#include "Squad/CSBreachWall.h"
#include "Squad/CSRescuee.h"
#include "Lens/CSLensMarker.h"
#include "AI/CSEnemy.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    const FLinearColor Teal(0.13f, 0.83f, 0.93f);
    const FLinearColor White(0.93f, 0.95f, 0.98f);
    const FLinearColor Mute(0.55f, 0.60f, 0.68f);
    const FLinearColor Red(0.95f, 0.30f, 0.30f);
    const FLinearColor Amber(0.98f, 0.76f, 0.25f);
    const FLinearColor Shade(0.f, 0.f, 0.f, 0.55f);

    FLinearColor SpeakerColor(FName S)
    {
        const FString N = S.ToString();
        if (N == TEXT("alex"))   return Teal;
        if (N == TEXT("bell"))   return FLinearColor(0.55f, 0.90f, 0.60f);
        if (N == TEXT("chen"))   return FLinearColor(0.75f, 0.70f, 1.00f);
        if (N == TEXT("mercer")) return FLinearColor(1.00f, 0.72f, 0.45f);
        if (N == TEXT("rourke")) return Mute;
        return Amber;   // the rescued
    }
    FString SpeakerName(FName S)
    {
        const FString N = S.ToString();
        if (N == TEXT("alex")) return TEXT("WARD"); if (N == TEXT("bell")) return TEXT("BELL"); if (N == TEXT("chen")) return TEXT("CHEN");
        if (N == TEXT("mercer")) return TEXT("MERCER"); if (N == TEXT("reyes")) return TEXT("REYES"); if (N == TEXT("halvorsen")) return TEXT("HALVORSEN");
        if (N == TEXT("okafor")) return TEXT("OKAFOR"); if (N == TEXT("rourke")) return TEXT("ANVIL");
        return N.ToUpper();
    }
}

void ACSHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas) return;

    const float W = Canvas->ClipX, H = Canvas->ClipY;
    const float S = H / 1080.f;
    UFont* Big = GEngine->GetLargeFont();
    UFont* Med = GEngine->GetMediumFont();
    UFont* Small = GEngine->GetSmallFont();

    ACSPlayerCharacter* P = Cast<ACSPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    ACSGameMode* GM = GetWorld()->GetAuthGameMode<ACSGameMode>();
    ACSMissionDirector* D = ACSMissionDirector::Get(this);
    UCSDialogueSubsystem* Dlg = GetWorld()->GetSubsystem<UCSDialogueSubsystem>();

    // ---- crosshair ----
    if (P && !P->bDead)
    {
        const float cx = W * 0.5f, cy = H * 0.5f, g = 6.f * S, l = 10.f * S, t = 2.f * S;
        const FLinearColor C = P->bLensUp ? White : Teal;
        DrawLine(cx - g - l, cy, cx - g, cy, C, t); DrawLine(cx + g, cy, cx + g + l, cy, C, t);
        DrawLine(cx, cy - g - l, cx, cy - g, C, t); DrawLine(cx, cy + g, cx, cy + g + l, C, t);
    }

    // ---- integrity + ammo (both modes) ----
    if (P)
    {
        const float bx = 40.f * S, by = H - 70.f * S, bw = 320.f * S, bh = 10.f * S;
        DrawText(TEXT("WARD"), Mute, bx, by - 24.f * S, Small, 1.1f * S);
        DrawRect(FLinearColor(1, 1, 1, 0.10f), bx, by, bw, bh);
        const float f = P->GetHealthNormalized();
        DrawRect(f > 0.35f ? Teal : Red, bx, by, bw * f, bh);

        const FString Ammo = P->bReloading ? TEXT("RELOADING") : FString::Printf(TEXT("%02d  /  %03d"), P->AmmoInMag, P->AmmoReserve);
        float tw = 0, th = 0; GetTextSize(Ammo, tw, th, Big, 1.2f * S);
        DrawText(Ammo, P->AmmoInMag == 0 && !P->bReloading ? Red : White, W - 40.f * S - tw, H - 84.f * S, Big, 1.2f * S);
    }

    // ---- arena mode (the old test map) ----
    if (!D)
    {
        if (GM)
        {
            DrawText(FString::Printf(TEXT("WAVE  %02d"), GM->Wave), Teal, 40.f * S, 34.f * S, Med, 1.4f * S);
            DrawText(FString::Printf(TEXT("SCORE  %06d"), GM->Score), White, 40.f * S, 70.f * S, Med, 1.1f * S);
            if (!GM->StatusText.IsEmpty()) { float tw = 0, th = 0; GetTextSize(GM->StatusText, tw, th, Big, 1.6f * S); DrawText(GM->StatusText, White, (W - tw) * 0.5f, H * 0.36f, Big, 1.6f * S); }
        }
        DrawAeonLink(W, S, Small);
        return;
    }

    // ================= mission mode =================
    // Objective, top-left. The beat's text; the clock beside it when Bell is working.
    {
        const FString Obj = D->CurrentObjective().ToString().ToUpper();
        if (!Obj.IsEmpty()) { DrawRect(Shade, 32.f * S, 30.f * S, 8.f * S, 26.f * S); DrawRect(Teal, 32.f * S, 30.f * S, 4.f * S, 26.f * S); DrawText(Obj, White, 50.f * S, 34.f * S, Med, 1.2f * S); }
        if (D->ClockRunning())
        {
            const int32 Sec = FMath::CeilToInt(D->ClockRemaining());
            const FString Clock = FString::Printf(TEXT("%d:%02d"), Sec / 60, Sec % 60);
            float tw = 0, th = 0; GetTextSize(Clock, tw, th, Big, 1.5f * S);
            DrawText(Clock, Sec <= 30 ? Red : White, (W - tw) * 0.5f, 34.f * S, Big, 1.5f * S);
            const FString Who = TEXT("BELL");
            float ww = 0, wh = 0; GetTextSize(Who, ww, wh, Small, 1.0f * S);
            DrawText(Who, Mute, (W - ww) * 0.5f, 34.f * S + th, Small, 1.0f * S);
        }
    }

    // Squad, bottom-left above the integrity bar: name, bar, DOWN with the bleed-out.
    {
        float y = H - 110.f * S;
        for (ACSSquadMate* M : D->Squad())
        {
            if (!M) continue;
            y -= 30.f * S;
            const FString Name = M->DisplayName.IsEmpty() ? M->Callsign.ToString().ToUpper() : M->DisplayName.ToString().ToUpper();
            DrawText(Name, M->IsDown() ? Red : White, 40.f * S, y, Small, 1.05f * S);
            const float bx = 150.f * S, bw = 160.f * S, bh = 6.f * S;
            DrawRect(FLinearColor(1, 1, 1, 0.10f), bx, y + 6.f * S, bw, bh);
            if (M->IsDown()) DrawText(FString::Printf(TEXT("DOWN  %d"), FMath::CeilToInt(M->BleedOutRemaining())), Red, bx, y, Small, 1.0f * S);
            else DrawRect(SpeakerColor(M->Callsign), bx, y + 6.f * S, bw * M->GetHealthNormalized(), bh);
            const TCHAR* OrderTxt = TEXT("");
            switch (M->GetOrder()) { case ECSSquadOrder::Hold: OrderTxt = TEXT("HOLDING"); break; case ECSSquadOrder::MoveTo: OrderTxt = TEXT("MOVING"); break; case ECSSquadOrder::Breach: OrderTxt = TEXT("BREACHING"); break; case ECSSquadOrder::Carry: OrderTxt = TEXT("CARRYING"); break; default: break; }
            if (*OrderTxt) DrawText(OrderTxt, Mute, bx + bw + 14.f * S, y, Small, 0.95f * S);
        }
    }

    // Context prompt under the crosshair; the strike confirm pulses.
    if (P && !P->bDead && !P->ContextPrompt.IsEmpty())
    {
        const FString T = P->ContextPrompt.ToString();
        float tw = 0, th = 0; GetTextSize(T, tw, th, Small, 1.1f * S);
        const bool bConfirm = D->ConfirmPending();
        const float a = bConfirm ? 0.6f + 0.4f * FMath::Sin(GetWorld()->GetTimeSeconds() * 6.f) : 1.f;
        DrawText(T, FLinearColor(bConfirm ? Amber.R : Mute.R, bConfirm ? Amber.G : Mute.G, bConfirm ? Amber.B : Mute.B, a), (W - tw) * 0.5f, H * 0.5f + 40.f * S, Small, 1.1f * S);
        if (P->bInteracting && P->InteractHeld > 0.f)
        {
            const float frac = FMath::Clamp(P->InteractHeld / (bConfirm ? 0.6f : 3.f), 0.f, 1.f);
            DrawRect(FLinearColor(1, 1, 1, 0.15f), (W - 160.f * S) * 0.5f, H * 0.5f + 70.f * S, 160.f * S, 5.f * S);
            DrawRect(bConfirm ? Amber : Teal, (W - 160.f * S) * 0.5f, H * 0.5f + 70.f * S, 160.f * S * frac, 5.f * S);
        }
    }

    // The lens: AEGIS markers. Objective always; the rest while LB is held.
    DrawLens(P, D, W, H, S, Small, Med);

    // Breach fuse over an armed wall
    for (TActorIterator<ACSBreachWall> It(GetWorld()); It; ++It)
        if (It->IsArmed())
        {
            const FVector Scr = Project(It->GetActorLocation() + FVector(0, 0, 160.f));
            if (Scr.Z > 0.f) { const FString F = FString::Printf(TEXT("%d"), FMath::CeilToInt(It->FuseRemaining())); float tw = 0, th = 0; GetTextSize(F, tw, th, Big, 1.4f * S); DrawText(F, Amber, Scr.X - tw * 0.5f, Scr.Y - th * 0.5f, Big, 1.4f * S); }
        }

    // Subtitles, bottom center, above the bars.
    if (Dlg) if (const FCSActiveLine* L = Dlg->Current())
    {
        const FString Name = SpeakerName(L->Speaker);
        const FString Text = L->Text.ToString();
        float nw = 0, nh = 0; GetTextSize(Name, nw, nh, Small, 1.1f * S);
        float tw = 0, th = 0; GetTextSize(Text, tw, th, Med, 1.15f * S);
        const float total = nw + 18.f * S + tw;
        const float x = (W - total) * 0.5f, y = H - 150.f * S;
        DrawRect(Shade, x - 16.f * S, y - 8.f * S, total + 32.f * S, FMath::Max(nh, th) + 16.f * S);
        DrawText(Name, SpeakerColor(L->Speaker), x, y + (th - nh) * 0.5f, Small, 1.1f * S);
        DrawText(Text, L->bBark ? Mute : White, x + nw + 18.f * S, y, Med, 1.15f * S);
    }

    // Fail card
    if (D->IsFailed())
    {
        DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.72f), 0, 0, W, H);
        const FString T1 = TEXT("MISSION FAILED");
        const FString T2 = D->FailReason.ToString();
        const FString T3 = TEXT("Y  /  A     RESTART FROM CHECKPOINT");
        float w1, h1, w2, h2, w3, h3; GetTextSize(T1, w1, h1, Big, 1.8f * S); GetTextSize(T2, w2, h2, Med, 1.2f * S); GetTextSize(T3, w3, h3, Small, 1.1f * S);
        DrawText(T1, Red, (W - w1) * 0.5f, H * 0.40f, Big, 1.8f * S);
        DrawText(T2, White, (W - w2) * 0.5f, H * 0.40f + h1 + 10.f * S, Med, 1.2f * S);
        DrawText(T3, Mute, (W - w3) * 0.5f, H * 0.40f + h1 + h2 + 40.f * S, Small, 1.1f * S);
    }

    DrawAeonLink(W, S, Small);
}

void ACSHUD::DrawAeonLink(float W, float S, UFont* Small)
{
    if (UAeonSDKSubsystem* Aeon = GetGameInstance()->GetSubsystem<UAeonSDKSubsystem>())
    {
        const FString Link = Aeon->IsOnAeon() ? TEXT("AEON  LINK  ONLINE") : TEXT("AEON  LINK  OFFLINE");
        float tw = 0, th = 0; GetTextSize(Link, tw, th, Small, 1.0f * S);
        DrawText(Link, Aeon->IsOnAeon() ? Teal : Mute, W - 40.f * S - tw, 34.f * S, Small, 1.0f * S);
    }
}

void ACSHUD::DrawDiamond(float x, float y, float r, const FLinearColor& C, float t)
{
    DrawLine(x, y - r, x + r, y, C, t); DrawLine(x + r, y, x, y + r, C, t);
    DrawLine(x, y + r, x - r, y, C, t); DrawLine(x - r, y, x, y - r, C, t);
}

void ACSHUD::DrawLens(ACSPlayerCharacter* P, ACSMissionDirector* D, float W, float H, float S, UFont* Small, UFont* Med)
{
    const bool bLens = P && P->bLensUp && !P->bDead;
    const FVector Eye = P ? P->GetActorLocation() : FVector::ZeroVector;

    if (bLens)
    {
        // a faint frame so the overlay reads as a mode
        DrawRect(FLinearColor(Teal.R, Teal.G, Teal.B, 0.06f), 0, 0, W, H);
        DrawText(TEXT("AEGIS"), Teal, 40.f * S, H * 0.5f - 10.f * S, Small, 1.0f * S);
        // contacts AEGIS knows about: every live enemy within 60 m, seen or not
        for (TActorIterator<ACSEnemy> It(GetWorld()); It; ++It)
        {
            if (It->IsDead() || FVector::Dist(Eye, It->GetActorLocation()) > 6000.f) continue;
            const FVector Scr = Project(It->GetActorLocation() + FVector(0, 0, 90.f));
            if (Scr.Z > 0.f) DrawDiamond(Scr.X, Scr.Y, 7.f * S, Red, 1.5f * S);
        }
        // exposed rescuees: amber
        for (TActorIterator<ACSRescuee> It(GetWorld()); It; ++It)
        {
            const FVector Scr = Project(It->GetActorLocation() + FVector(0, 0, 110.f));
            if (Scr.Z > 0.f) { DrawDiamond(Scr.X, Scr.Y, 6.f * S, Amber, 1.5f * S); const FString N = It->DisplayName.ToString().ToUpper(); float tw, th; GetTextSize(N, tw, th, Small, 0.9f * S); DrawText(N, Amber, Scr.X - tw * 0.5f, Scr.Y - 24.f * S, Small, 0.9f * S); }
        }
    }

    for (TActorIterator<ACSLensMarker> It(GetWorld()); It; ++It)
    {
        ACSLensMarker* M = *It;
        if (!M->IsShown()) continue;
        if (M->Kind != ECSMarkerKind::Objective && !bLens) continue;

        if (M->Kind == ECSMarkerKind::Route)
        {
            TArray<FVector> Pts; Pts.Add(M->GetActorLocation()); Pts.Append(M->RoutePoints);
            for (int32 i = 0; i + 1 < Pts.Num(); i++)
            {
                const FVector A = Project(Pts[i] + FVector(0, 0, 30.f)), B = Project(Pts[i + 1] + FVector(0, 0, 30.f));
                if (A.Z > 0.f && B.Z > 0.f) DrawLine(A.X, A.Y, B.X, B.Y, Teal, 2.f * S);
                if (A.Z > 0.f) DrawDiamond(A.X, A.Y, 4.f * S, Teal, 1.f * S);
            }
            continue;
        }

        const FVector Scr = Project(M->GetActorLocation() + FVector(0, 0, 120.f));
        if (Scr.Z <= 0.f) continue;
        FLinearColor C = Teal; float r = 9.f * S;
        switch (M->Kind)
        {
        case ECSMarkerKind::Threat: C = Red; break;
        case ECSMarkerKind::Strike: C = Amber; r = 13.f * S; break;
        case ECSMarkerKind::Hold:   C = FLinearColor(0.6f, 0.9f, 0.7f); r = 7.f * S; break;
        case ECSMarkerKind::Label:  C = White; r = 0.f; break;
        default: break;
        }
        if (r > 0.f) DrawDiamond(Scr.X, Scr.Y, r, C, 2.f * S);
        FString Text = M->Label.ToString().ToUpper();
        if (M->Kind == ECSMarkerKind::Objective) Text += FString::Printf(TEXT("   %d m"), FMath::RoundToInt(FVector::Dist(Eye, M->GetActorLocation()) / 100.f));
        if (M->Kind == ECSMarkerKind::Strike && D && D->ConfirmPending()) Text += TEXT("   --   CONFIRM");
        if (!Text.IsEmpty()) { float tw, th; GetTextSize(Text, tw, th, Small, 1.0f * S); DrawText(Text, C, Scr.X - tw * 0.5f, Scr.Y - r - th - 4.f * S, Small, 1.0f * S); }
    }
}
