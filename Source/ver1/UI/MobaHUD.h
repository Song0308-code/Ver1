#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MobaHUD.generated.h"

class AMobaCharacter;

/** UMG 화면이 들어오기 전까지 캔버스로 그리는 임시 HUD. 머리 위 체력바와 부활 대기 시간을 표시한다. */
UCLASS()
class VER1_API AMobaHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	void DrawHealthBar(const AMobaCharacter* Character);
	void DrawRespawnCountdown();

	UPROPERTY(EditDefaultsOnly, Category = "Health Bar")
	FVector2D HealthBarSize = FVector2D(110.0, 12.0);

	/** 캡슐 꼭대기에서 체력바까지의 높이. */
	UPROPERTY(EditDefaultsOnly, Category = "Health Bar")
	float HealthBarHeightOffset = 50.0f;
};
