#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CSHUD.generated.h"

UCLASS()
class CYBERSIEGE_API ACSHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};
