#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Game/MobaTypes.h"
#include "MobaPlayerController.generated.h"

class AMobaCameraRig;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

class SMobaChatInput;

/** 받은 채팅 한 줄. */
struct FMobaChatLine
{
	FString Sender;
	FString Text;
	bool bAll = false;
	EMobaTeam SenderTeam = EMobaTeam::None;

	/** 받은 시각(이 PC의 월드 시간). */
	double Time = 0.0;
};

/** 팀에게 보내는 위치 신호. */
struct FMobaPing
{
	FVector Location = FVector::ZeroVector;

	EMobaPingType Type = EMobaPingType::Normal;

	/** 핑을 찍은 사람("블루 레인저"). */
	FString Sender;

	/** 받은 시각(이 PC의 월드 시간). */
	double Time = 0.0;
};

/** 사망 요약의 한 줄: 최근에 나에게 피해를 준 쪽과 그 합계. */
USTRUCT()
struct FMobaDeathRecapEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FString Source;

	UPROPERTY()
	float Damage = 0.0f;
};

UCLASS()
class VER1_API AMobaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMobaPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	/** 조준 중인 스킬 칸. 조준 중이 아니면 false를 돌려준다. */
	bool GetTargetingSlot(EMobaAbilitySlot& OutSlot) const;

	/** A키를 눌러 공격 대상을 고르는 중인지. */
	bool IsAttackTargeting() const { return bAttackTargeting; }

	/** 커서가 가리키는 지면 위치. */
	bool GetCursorGroundLocation(FVector& OutLocation) const;

	/** UI 판정에 쓰는 마우스 위치. 에디터 검증 중에는 지정한 좌표를 대신 돌려준다. */
	bool GetPointerPosition(float& OutX, float& OutY) const;

	bool IsShopOpen() const { return bShopOpen; }
	bool IsGameMenuOpen() const { return bGameMenuOpen; }
	void CloseGameMenu() { bGameMenuOpen = false; }

	/** 게임 메뉴의 조작 설정. 바꾸면 바로 적용하고 프로필에 저장한다. */
	bool IsSmartCastEnabled() const { return bSmartCast; }
	void ToggleSmartCast();
	bool IsCameraLockDefault() const;
	void ToggleCameraLockDefault();
	int32 GetCameraSpeedIndex() const;
	void CycleCameraSpeed();

	/** 항복에 동의한다. */
	void RequestSurrender() { ServerRequestSurrender(); }

	/** 경기를 그만두고 메뉴로 돌아간다. 경기 중이면 상대의 승리로 끝난다. */
	void LeaveMatch();
	bool IsScoreboardShown() const { return bScoreboardHeld; }

	/** 최근에 받은 핑. 오래된 것부터 들어 있다. */
	const TArray<FMobaPing>& GetPings() const { return Pings; }

	/** 받은 채팅(오래된 것부터, 최근 20줄). */
	const TArray<FMobaChatLine>& GetChatLog() const { return ChatLog; }

	/** 채팅 입력창이 열려 있는지. */
	bool IsChatOpen() const { return ChatWidget.IsValid(); }

	/** 채팅을 보낸다. "/all "로 시작하면 전체, 아니면 팀에게. */
	void SendChat(const FString& Text);

	UFUNCTION(Client, Reliable)
	void ClientReceiveChat(const FString& Sender, const FString& Text, bool bAll, EMobaTeam SenderTeam);

	/** 마지막으로 죽었을 때의 요약. 처치한 쪽과, 최근에 피해를 준 쪽별 합계(큰 것부터). */
	const FString& GetDeathRecapKiller() const { return DeathRecapKiller; }
	const TArray<FMobaDeathRecapEntry>& GetDeathRecap() const { return DeathRecap; }

	/** 서버에서 호출한다. 이 플레이어의 PC에 사망 요약을 보낸다. */
	UFUNCTION(Client, Reliable)
	void ClientShowDeathRecap(const FString& Killer, const TArray<FMobaDeathRecapEntry>& Entries);

	// 개발 빌드 콘솔(` 키) 명령. 배포 빌드에서는 동작하지 않는다. 결과는 모두에게 "[개발]" 알림으로 보인다.

	/** MobaGold 1000: 골드를 준다. */
	UFUNCTION(Exec)
	void MobaGold(float Amount);

	/** MobaLevel 11: 그 레벨까지 올린다. */
	UFUNCTION(Exec)
	void MobaLevel(int32 Level);

	/** MobaSkipTime 600: 경기 시간을 앞당긴다(정글·포탑 방패·미니언 성장 반영). */
	UFUNCTION(Exec)
	void MobaSkipTime(float Seconds);

	/** MobaRefresh: 체력·마나를 채우고 재사용 대기를 없앤다. */
	UFUNCTION(Exec)
	void MobaRefresh();

	/** MobaSpawnCamps: 비어 있는 정글 캠프를 바로 내보낸다. */
	UFUNCTION(Exec)
	void MobaSpawnCamps();

	/** 로비에서 챔피언을 바꾼다. 고른 것은 다음 경기를 위해 저장해 둔다. */
	void RequestChampion(const FString& AssetName);

	/** 로비에서 준비 상태를 바꾼다. */
	void RequestReady(bool bReady) { ServerRequestReady(bReady); }

	/** 로비에서 소환사 주문을 바꾼다. Index 0이 D, 1이 F. 고른 것은 다음 경기를 위해 저장해 둔다. */
	void RequestSummonerSpell(int32 Index, EMobaSummonerSpell Spell);

	/** 로비에서 특성(bKeystone) 또는 보조 능력치를 고른다. 메뉴 구성에도 저장한다. */
	void RequestTalent(bool bKeystone, int32 Value);

protected:
	/** 입력 에셋을 만들기 전까지 쓰는 기본 키 배치를 코드로 구성한다. */
	void CreateDefaultInput();

	void OnMoveCommandStarted();
	void OnMoveCommandHeld();
	void OnCameraLockToggled();
	void OnCameraCenterStarted();
	void OnCameraCenterCompleted();
	void OnCameraZoom(const FInputActionValue& Value);
	void OnAbilityQ();
	void OnAbilityW();
	void OnAbilityE();
	void OnAbilityR();
	void OnSummonerD();
	void OnSummonerF();
	void OnAttackModeStarted();
	void OnConfirmTarget();
	void OnReturnToMenu();
	void OnPlaceWard();
	void OnUsePotion();
	void OnUseActiveItem();
	void OnUseHeraldEye();
	void OnPlaceControlWard();
	void OnRecall();
	void OnToggleShop();
	void OnToggleGameMenu();
	void OnScoreboardStarted();
	void OnScoreboardCompleted();

	/** Alt + 좌클릭. 커서 위치(미니맵 위라면 그 지점)를 같은 팀에게 알린다. */
	void SendPingAtCursor(EMobaPingType Type = EMobaPingType::Normal);

	UFUNCTION(Server, Reliable)
	void ServerPing(FVector_NetQuantize Location, EMobaPingType Type);

	UFUNCTION(Server, Reliable)
	void ServerRequestChampion(const FString& AssetName);

	UFUNCTION(Server, Reliable)
	void ServerRequestReady(bool bReady);

	UFUNCTION(Server, Reliable)
	void ServerRequestSummonerSpell(int32 Index, EMobaSummonerSpell Spell);

	UFUNCTION(Server, Reliable)
	void ServerRequestTalent(bool bKeystone, int32 Value);

	UFUNCTION(Server, Reliable)
	void ServerRequestSurrender();

	UFUNCTION(Server, Reliable)
	void ServerDevCommand(EMobaDevCommand Command, float Value);

	UFUNCTION(Server, Reliable)
	void ServerChat(const FString& Text, bool bAll);

	void OpenChat();
	void CloseChat();

	/** 게임 조작용 입력 모드로 되돌린다. */
	void RestoreGameInputMode();

	TSharedPtr<SMobaChatInput> ChatWidget;
	TArray<FMobaChatLine> ChatLog;

	UFUNCTION(Client, Reliable)
	void ClientShowPing(FVector_NetQuantize Location, EMobaPingType Type, const FString& Sender);

	/** 서버 전용. 최근에 받은 핑 시각(도배 제한용). */
	TArray<double> RecentPingTimes;

	FString DeathRecapKiller;
	TArray<FMobaDeathRecapEntry> DeathRecap;

	bool IsAltKeyDown() const;

	/** 스킬 키를 눌렀을 때. Ctrl과 함께면 스킬 단계를 올리고, 아니면 즉시 시전하거나 조준 모드로 들어간다. */
	void HandleAbilityKey(EMobaAbilitySlot Slot);

	/** 해당 칸의 스킬을 커서 위치를 향해 시전하도록 요청한다. */
	void CastAbilityAtCursor(EMobaAbilitySlot Slot);

	/**
	 * 커서가 가리키는 적을 찾는다. 정확히 겹치지 않아도 커서 근처에 있으면 잡아 준다.
	 * 적이 없으면 nullptr을 돌려주고, 커서가 닿은 지점은 어느 쪽이든 OutCursorLocation에 담는다.
	 */
	AActor* FindEnemyAtCursor(FVector& OutCursorLocation) const;

	/** 우클릭. 커서에 적이 있으면 공격을, 없으면 그 지면으로 이동을 명령한다. */
	void IssueCommandAtCursor();

	/** A키 후 좌클릭. 커서에 적이 있으면 공격을, 없으면 그 지면으로 공격 이동을 명령한다. */
	void IssueAttackCommandAtCursor();

	/** 지정한 위치와 가장 가까운 내비메시 지점으로 이동을 명령한다. 공격 명령은 취소된다. */
	void IssueMoveTo(const FVector& WorldLocation);

	/** 지정한 위치로 걸어가면서 도중에 만나는 적을 공격하도록 명령한다. */
	void IssueAttackMoveTo(const FVector& WorldLocation);

	/** 대상을 사거리까지 쫓아가 기본 공격하도록 명령한다. */
	void IssueAttack(AActor* Target);

	bool ProjectToNavigation(const FVector& WorldLocation, FVector& OutLocation) const;

	/** 커서가 미니맵 위에 있으면 그 지점의 월드 위치를 돌려준다. */
	bool GetCursorMinimapLocation(FVector& OutLocation) const;

	bool IsControlKeyDown() const;

	void UpdateCamera(float DeltaTime);

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TSubclassOf<AMobaCameraRig> CameraRigClass;

	UPROPERTY(Transient)
	TObjectPtr<AMobaCameraRig> CameraRig;

	/** true면 카메라가 챔피언을 따라간다. false면 화면 가장자리로 스크롤한다. */
	UPROPERTY(EditAnywhere, Category = "Camera")
	bool bCameraLocked = true;

	/** 화면 가장자리에서 이 픽셀 안으로 커서가 들어오면 스크롤한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float EdgeScrollMargin = 20.0f;

	/**
	 * true면 스킬 키를 누르는 즉시 커서 위치로 시전한다(스마트캐스트).
	 * false면 범위 표시가 나오고 좌클릭으로 확정, 우클릭으로 취소한다.
	 */
	UPROPERTY(EditAnywhere, Category = "Abilities")
	bool bSmartCast = true;

	/** 우클릭을 누르고 있을 때 이동 명령을 다시 내리는 간격(초). */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float HeldMoveInterval = 0.1f;

	/** 클릭 지점을 내비메시에 투영할 때 허용하는 범위. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	FVector NavProjectExtent = FVector(400.0, 400.0, 300.0);

	/** 커서가 닿은 지점에서 적의 가장자리까지 이 거리 안이면 그 적을 클릭한 것으로 본다. */
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float ClickSelectMargin = 90.0f;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveCommandAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> CameraLockAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> CameraCenterAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> CameraZoomAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> AbilityQAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> AbilityWAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> AbilityEAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> AbilityRAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SummonerDAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SummonerFAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> AttackModeAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ConfirmTargetAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ReturnToMenuAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PlaceWardAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> UsePotionAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> UseActiveItemAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PlaceControlWardAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> UseHeraldEyeAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RecallAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ShopAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ScoreboardAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> GameMenuAction;

#if WITH_EDITORONLY_DATA
	/** 에디터 자동 검증용. bDebugIssueMove를 켜면 다음 틱에 DebugMoveGoal로 이동 명령을 내린다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	FVector DebugMoveGoal = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugIssueMove = false;

	/** 에디터 자동 검증용. 켜면 다음 틱에 DebugMoveGoal로 공격 이동 명령을 내린다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugIssueAttackMove = false;

	/** 에디터 자동 검증용. bDebugIssueAttack을 켜면 다음 틱에 DebugAttackTarget을 공격하도록 명령한다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	TObjectPtr<AActor> DebugAttackTarget;

	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugIssueAttack = false;

	/** 에디터 자동 검증용. bDebugIssueCast를 켜면 다음 틱에 DebugCastSlot 스킬을 DebugCastLocation으로 시전한다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	EMobaAbilitySlot DebugCastSlot = EMobaAbilitySlot::Q;

	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	FVector DebugCastLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugIssueCast = false;

	/** 에디터 자동 검증용. 켜면 다음 틱에 DebugCastSlot 스킬을 한 단계 올린다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugLevelUp = false;

	/** 에디터 자동 검증용. 켜면 다음 틱에 DebugCastLocation에 제어 와드를 놓는다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugPlaceControlWard = false;

	/** 에디터 자동 검증용. 비어 있지 않으면 다음 틱에 콘솔 명령으로 실행한다(예: "MobaSkipTime 600"). */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	FString DebugConsoleCommand;

	/** 에디터 자동 검증용. 비어 있지 않으면 다음 틱에 채팅으로 보낸다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	FString DebugChatMessage;

	/** 에디터 자동 검증용. 켜면 다음 틱에 채팅 입력창을 연다(Enter와 같음). */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugOpenChat = false;

	/** 에디터 자동 검증용. 켜면 다음 틱에 전령의 눈을 쓴다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugUseHeraldEye = false;

	/** 에디터 자동 검증용. 켜면 다음 틱에 체력 물약을 마신다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugUsePotion = false;

	/** 에디터 자동 검증용. 켜면 다음 틱에 사용 아이템(키 5)을 쓴다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugUseActiveItem = false;

	/** 에디터 자동 검증용. 켜면 다음 틱에 DebugCastLocation에 와드를 놓는다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugPlaceWard = false;

	/** 에디터 자동 검증용. 켜면 다음 틱에 DebugItemIndex번 아이템을 사거나, 가방의 DebugItemIndex번 칸을 판다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	int32 DebugItemIndex = 0;

	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugBuyItem = false;

	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugSellItem = false;

	/** 에디터 자동 검증용. 호스트의 컨트롤러에 0보다 큰 값을 넣으면 다음 틱에 모든 플레이어에게 그만큼 경험치를 준다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	float DebugGrantExperience = 0.0f;

	/** 에디터 자동 검증용. 0보다 크면 다음 틱에 모든 플레이어에게 그만큼 골드를 준다(호스트 컨트롤러에서만 동작). */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	float DebugGrantGold = 0.0f;

	/** 에디터 자동 검증용. 비어 있지 않으면 다음 틱에 로비에서 이 챔피언으로 바꾼다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	FString DebugLobbyChampion;

	/** 에디터 자동 검증용. 비어 있지 않으면 다음 틱에 로비에서 소환사 주문을 바꾼다. 예: "Smite,Flash"(D, F 순서). */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	FString DebugLobbySpells;

	/** 에디터 자동 검증용. 켜면 다음 틱에 로비 준비 상태를 뒤집는다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugToggleReady = false;

	/** 에디터 자동 검증용. 켜면 UI 판정(상점·메뉴·미니맵·툴팁)에 실제 마우스 대신 DebugPointerPosition을 쓴다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugPointerOverride = false;

	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	FVector2D DebugPointerPosition = FVector2D::ZeroVector;

	/** 에디터 자동 검증용. 켜면 다음 틱에 그 위치를 좌클릭·우클릭한 것처럼 처리한다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugLeftClick = false;

	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugRightClick = false;

	/** 에디터 자동 검증용. 켜면 다음 틱에 그 위치로 핑을 보낸다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugPing = false;

	/** 에디터 자동 검증용. bDebugPing으로 보낼 핑 종류. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	EMobaPingType DebugPingType = EMobaPingType::Normal;

	/** 에디터 자동 검증용. 켜면 다음 틱에 상점·게임 메뉴를 여닫는다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugToggleShop = false;

	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugToggleGameMenu = false;

	/** 에디터 자동 검증용. 켜면 다음 틱에 점수판을 켜거나 끈다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugShowScoreboard = false;

	/** 에디터 자동 검증용. 켜면 다음 틱에 항복에 동의한다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugSurrender = false;

	/** 에디터 자동 검증용. 켜면 다음 틱에 귀환을 시작한다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugRecall = false;
#endif

	bool bTargeting = false;
	EMobaAbilitySlot TargetingSlot = EMobaAbilitySlot::Q;

	bool bAttackTargeting = false;

	bool bShopOpen = false;
	bool bGameMenuOpen = false;
	bool bScoreboardHeld = false;

	TArray<FMobaPing> Pings;

	/** 서버에서 핑을 너무 자주 보내지 못하게 막는 데 쓴다. */
	double LastPingTime = -10.0;

	bool bCameraCenterHeld = false;
	float LastHeldMoveTime = 0.0f;
};
