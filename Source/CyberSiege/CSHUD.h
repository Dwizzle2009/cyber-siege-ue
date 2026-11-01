#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CSHUD.generated.h"

class ACSPlayerCharacter;
class ACSMissionDirector;

/** Canvas HUD: crosshair, integrity, ammo, objective, squad, subtitles, the AEGIS lens, the fail card. */
UCLASS()
class CYBERSIEGE_API ACSHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
private:
    void DrawAeonLink(float W, float S, UFont* Small);
    void DrawDiamond(float x, float y, float r, const FLinearColor& C, float t);
    void DrawLens(ACSPlayerCharacter* P, ACSMissionDirector* D, float W, float H, float S, UFont* Small, UFont* Med);
};
