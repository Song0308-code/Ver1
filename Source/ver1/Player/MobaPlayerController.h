#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
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

protected:
	/** 입력 에셋을 만들기 전까지 쓰는 기본 키 배치를 코드로 구성한다. */
	void CreateDefaultInput();

	void OnMoveCommandStarted();
	void OnMoveCommandHeld();
	void OnCameraLockToggled();
	void OnCameraCenterStarted();
	void OnCameraCenterCompleted();
	void OnCameraZoom(const FInputActionValue& Value);

	/** 커서 아래에 적이 있으면 공격을, 없으면 그 지면으로 이동을 명령한다. */
	void IssueCommandAtCursor();

	/** 지정한 위치와 가장 가까운 내비메시 지점으로 이동을 명령한다. 공격 명령은 취소된다. */
	void IssueMoveTo(const FVector& WorldLocation);

	/** 대상을 사거리까지 쫓아가 기본 공격하도록 명령한다. */
	void IssueAttack(AActor* Target);

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

	/** 우클릭을 누르고 있을 때 이동 명령을 다시 내리는 간격(초). */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float HeldMoveInterval = 0.1f;

	/** 클릭 지점을 내비메시에 투영할 때 허용하는 범위. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	FVector NavProjectExtent = FVector(400.0, 400.0, 300.0);

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

#if WITH_EDITORONLY_DATA
	/** 에디터 자동 검증용. bDebugIssueMove를 켜면 다음 틱에 DebugMoveGoal로 이동 명령을 내린다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	FVector DebugMoveGoal = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugIssueMove = false;

	/** 에디터 자동 검증용. bDebugIssueAttack을 켜면 다음 틱에 DebugAttackTarget을 공격하도록 명령한다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	TObjectPtr<AActor> DebugAttackTarget;

	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugIssueAttack = false;
#endif

	bool bCameraCenterHeld = false;
	float LastHeldMoveTime = 0.0f;
};
