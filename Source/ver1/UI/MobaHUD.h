#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Game/MobaTypes.h"
#include "MobaHUD.generated.h"

class AMobaCharacter;
class UAbilitySystemComponent;

/** UMG 화면이 들어오기 전까지 캔버스로 그리는 임시 HUD. */
UCLASS()
class VER1_API AMobaHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	/** 유닛 머리 위 체력바. bShowMana면 바로 아래에 마나바도 그린다. */
	void DrawUnitBar(const UAbilitySystemComponent* AbilitySystem, const FVector& WorldLocation, const FVector2D& Size, const FLinearColor& FillColor, bool bShowMana, const FString& Label);

	/** 내 유닛은 초록, 아군은 파랑, 적은 빨강. */
	FLinearColor GetRelationColor(EMobaTeam UnitTeam, bool bIsLocalUnit) const;

	void DrawChampionBars();
	void DrawMinionBars();
	void DrawStructureBars();

	void DrawRespawnCountdown();

	/** 화면 위쪽의 경기 시간과 킬/데스. */
	void DrawMatchInfo();

	/** 상대를 기다리는 동안의 안내. 호스트에게는 알려 줄 IP도 보여 준다. */
	void DrawWaitingForPlayers();

	/** 경기가 끝났을 때 승패 문구. */
	void DrawMatchResult();

	/** 화면 아래의 레벨·경험치·골드, 체력·마나 수치, QWER 스킬 칸. */
	void DrawStatusBar(const AMobaCharacter* Character);

	/** 스킬 조준 중일 때 사거리와 범위를 지면에 그린다. */
	void DrawTargetingIndicator(const AMobaCharacter* Character);

	/** 지면 위의 원을 화면에 투영해 선으로 그린다. */
	void DrawGroundCircle(const FVector& Center, float Radius, const FLinearColor& Color, float Thickness);

	void DrawGroundLine(const FVector& Start, const FVector& End, const FLinearColor& Color, float Thickness);

	void DrawCenteredText(const FString& Text, const FLinearColor& Color, float CenterX, float Y, UFont* Font, float Scale);

	UPROPERTY(EditDefaultsOnly, Category = "Health Bar")
	FVector2D ChampionBarSize = FVector2D(110.0, 12.0);

	UPROPERTY(EditDefaultsOnly, Category = "Health Bar")
	FVector2D MinionBarSize = FVector2D(56.0, 6.0);

	UPROPERTY(EditDefaultsOnly, Category = "Health Bar")
	FVector2D StructureBarSize = FVector2D(150.0, 12.0);

	/** 캡슐 꼭대기에서 체력바까지의 높이. */
	UPROPERTY(EditDefaultsOnly, Category = "Health Bar")
	float HealthBarHeightOffset = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Status Bar")
	float AbilityBoxSize = 60.0f;

	/** 호스트 화면에 보여 줄 이 PC의 주소. 처음 필요할 때 한 번만 알아낸다. */
	FString LocalAddress;
	bool bLocalAddressResolved = false;
};
