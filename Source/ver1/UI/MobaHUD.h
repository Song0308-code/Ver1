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

	/** 화면 좌표가 미니맵 위에 있으면 그 지점의 월드 위치(지면 높이)를 돌려준다. */
	bool GetMinimapWorldLocation(const FVector2D& ScreenPosition, FVector& OutWorldLocation) const;

	/** 좌클릭을 상점 창이나 가방이 받았으면 처리하고 true를 돌려준다. 상점이 열려 있을 때만 받는다. */
	bool HandleClick(const FVector2D& ScreenPosition);

	/** 화면 좌표가 열려 있는 상점 창 위인지. */
	bool IsOverPanel(const FVector2D& ScreenPosition) const;

protected:
	/** 유닛 머리 위 체력바. bShowMana면 바로 아래에 마나바도 그린다. */
	/** Segments가 2 이상이면 막대를 그 수만큼 나누는 눈금을 긋는다(포탑 방패). */
	void DrawUnitBar(const UAbilitySystemComponent* AbilitySystem, const FVector& WorldLocation, const FVector2D& Size, const FLinearColor& FillColor, bool bShowMana, const FString& Label, int32 Segments = 0);

	/** 내 유닛은 초록, 아군은 파랑, 적은 빨강. */
	FLinearColor GetRelationColor(EMobaTeam UnitTeam, bool bIsLocalUnit) const;

	void DrawChampionBars();
	void DrawMinionBars();
	void DrawStructureBars();
	void DrawWardBars();
	void DrawMonsterBars();

	/** 정글 캠프와 억제기가 다시 나오기까지 남은 시간을 그 자리에 표시한다. */
	void DrawWorldTimers();

	/** 화면 오른쪽 아래의 미니맵. 시야 밖의 적은 그리지 않는다. */
	void DrawMinimap();

	/** 미니맵 위에 월드 위치를 점으로 찍는다. */
	void DrawMinimapDot(const FVector& WorldLocation, float Size, const FLinearColor& Color);

	FVector2D WorldToMinimap(const FVector& WorldLocation) const;

	/** 가지고 있는 정글 버프와 와드 상태. */
	void DrawBuffsAndWard(const AMobaCharacter* Character);

	/** 이 PC의 플레이어에게 보이는 유닛인지. */
	bool IsSeen(const AActor* Actor) const;

	/** 화면 오른쪽 위의 알림(처치, 건물 파괴 등). */
	void DrawAnnouncements();

	/** 귀환을 준비하는 동안의 진행 막대. */
	void DrawRecall(const AMobaCharacter* Character);

	/** 가방 여섯 칸. 상점이 열려 있으면 눌러서 팔 수 있다. */
	void DrawInventory(const AMobaCharacter* Character);

	/** 화면 왼쪽 아래의 능력치 패널. */
	void DrawStatsPanel(const AMobaCharacter* Character);

	/** 상점 창. P로 여닫는다. */
	void DrawShop(const AMobaCharacter* Character);

	/** Tab을 누르고 있는 동안 보이는 점수판. */
	void DrawScoreboard();

	/** 플레이어별 챔피언·레벨·K/D·CS·피해량·아이템 표. 점수판과 결과 화면이 같이 쓴다. */
	void DrawPlayerTable(float Top);

	/** 경기 전 챔피언 선택 화면. 챔피언 버튼과 준비 버튼을 누를 수 있다. */
	void DrawLobby();

	/** F10 게임 메뉴(항복, 메뉴로 나가기, 계속하기, 화면 설정, 조작 안내). */
	void DrawGameMenu();

	/** 피해를 받은 챔피언·몬스터·건물 위로 떠오르는 숫자. 체력이 줄어든 만큼을 표시한다. */
	void UpdateDamageNumbers();
	void DrawDamageNumbers();

	/** 마우스를 올린 스킬 칸의 설명. */
	void DrawAbilityTooltip(const AMobaCharacter* Character);

	struct FDamageNumber
	{
		FVector Location;
		FString Text;
		FLinearColor Color;
		double Time;
	};

	/** 유닛마다 지난 프레임의 체력. */
	TMap<TWeakObjectPtr<AActor>, float> LastHealth;
	TArray<FDamageNumber> DamageNumbers;

	/** 스킬 칸 묶음(QWER)의 왼쪽 끝. 화면 가운데에 두되 오른쪽의 가방이 미니맵에 닿으면 왼쪽으로 민다. */
	float GetAbilityBarLeft() const;

	/** 이번 프레임에 그린 스킬 칸의 화면 위치. */
	FBox2D AbilityRects[6];

	/** 이번 프레임에 그린 패시브 칸의 화면 위치. */
	FBox2D PassiveRect = FBox2D(ForceInit);

	/** 호스트에게 보여 줄, 상대에게 알려 줄 주소 안내. */
	FString GetHostAddressHint();

	/** 팀이 보낸 위치 신호. */
	void DrawPings();

	/** 아군 시야가 닿지 않는 지면을 어둡게 덮는다. */
	void DrawFogOfWar();

	/** 미니맵 위의 안개. */
	void DrawMinimapFog();

	UPROPERTY(EditDefaultsOnly, Category = "Fog")
	float FogOpacity = 0.55f;

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

	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	float MinimapSize = 250.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	float MinimapMargin = 14.0f;

	/** 미니맵이 나타내는 월드 범위(한 변). 월드 +X가 미니맵의 위쪽, +Y가 오른쪽이다. */
	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	float MapWorldSize = 14870.0f;

	/** 이번 프레임에 그린 로비의 챔피언 버튼(에셋 이름과 위치)과 준비 버튼. */
	TArray<TPair<FString, FBox2D>> LobbyChampionRects;

	/** 이번 프레임에 그린 로비의 소환사 주문 버튼. */
	struct FLobbySpellButton
	{
		int32 Index;
		EMobaSummonerSpell Spell;
		FBox2D Rect;
	};
	TArray<FLobbySpellButton> LobbySpellRects;
	FBox2D LobbyReadyRect = FBox2D(ForceInit);
	bool bLobbyDrawn = false;

	/** 이번 프레임에 그린 게임 메뉴 버튼. */
	FBox2D GameMenuSurrenderRect = FBox2D(ForceInit);
	FBox2D GameMenuLeaveRect = FBox2D(ForceInit);
	FBox2D GameMenuResumeRect = FBox2D(ForceInit);
	FBox2D GameMenuWindowModeRect = FBox2D(ForceInit);
	FBox2D GameMenuResolutionRect = FBox2D(ForceInit);
	bool bGameMenuDrawn = false;
	bool bSurrenderAllowed = false;

	/** 이번 프레임에 그린 상점 창과 그 안의 줄, 가방 칸의 화면 위치. 클릭 판정에 쓴다. */
	FBox2D ShopPanelRect = FBox2D(ForceInit);
	TArray<FBox2D> ShopItemRects;
	TArray<FBox2D> InventoryRects;
	bool bShopDrawn = false;

	/** 마지막으로 그린 캔버스 크기. 에디터 검증에서 화면 좌표를 계산할 때 읽는다. */
	UPROPERTY(VisibleAnywhere, Transient, Category = "Debug")
	FVector2D LastCanvasSize = FVector2D::ZeroVector;

	/** 이번 프레임에 그린 미니맵의 화면 위치. */
	FVector2D MinimapOrigin = FVector2D::ZeroVector;
	bool bMinimapDrawn = false;

	/** 호스트 화면에 보여 줄 이 PC의 주소. 처음 필요할 때 한 번만 알아낸다. */
	FString LocalAddress;
	bool bLocalAddressResolved = false;
};
