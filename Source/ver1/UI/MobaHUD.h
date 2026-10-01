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

	/** 화면 아래의 체력·마나 수치와 QWER 스킬 칸. */
	void DrawStatusBar(const AMobaCharacter* Character);

	/** 스킬 조준 중일 때 사거리와 범위를 지면에 그린다. */
	void DrawTargetingIndicator(const AMobaCharacter* Character);

	/** 지면 위의 원을 화면에 투영해 선으로 그린다. */
	void DrawGroundCircle(const FVector& Center, float Radius, const FLinearColor& Color, float Thickness);

	void DrawGroundLine(const FVector& Start, const FVector& End, const FLinearColor& Color, float Thickness);

	UPROPERTY(EditDefaultsOnly, Category = "Health Bar")
	FVector2D HealthBarSize = FVector2D(110.0, 12.0);

	/** 캡슐 꼭대기에서 체력바까지의 높이. */
	UPROPERTY(EditDefaultsOnly, Category = "Health Bar")
	float HealthBarHeightOffset = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Status Bar")
	float AbilityBoxSize = 60.0f;
};
