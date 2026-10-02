#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Game/MobaTypes.h"
#include "MobaPlayerController.generated.h"

class AMobaCameraRig;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

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
	void OnAttackModeStarted();
	void OnConfirmTarget();
	void OnReturnToMenu();

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

	bool IsControlKeyDown() const;

	void UpdateCamera(float DeltaTime);

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TSubclassOf<AMobaCameraRig> CameraRigClass;

	UPROPERTY(Transient)
	TObjectPtr<AMobaCameraRig> CameraRig;

	/** true면 카메라가 챔피언을 따라간다. false면 화면 가장자리로 스크롤한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	bool bCameraLocked = true;

	/** 화면 가장자리에서 이 픽셀 안으로 커서가 들어오면 스크롤한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float EdgeScrollMargin = 20.0f;

	/**
	 * true면 스킬 키를 누르는 즉시 커서 위치로 시전한다(스마트캐스트).
	 * false면 범위 표시가 나오고 좌클릭으로 확정, 우클릭으로 취소한다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
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
	TObjectPtr<UInputAction> AttackModeAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ConfirmTargetAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ReturnToMenuAction;

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
#endif

	bool bTargeting = false;
	EMobaAbilitySlot TargetingSlot = EMobaAbilitySlot::Q;

	bool bAttackTargeting = false;

	bool bCameraCenterHeld = false;
	float LastHeldMoveTime = 0.0f;
};
