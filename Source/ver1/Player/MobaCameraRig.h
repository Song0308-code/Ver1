#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MobaCameraRig.generated.h"

class UCameraComponent;
class USpringArmComponent;

/** 챔피언과 분리된 탑다운 카메라. 각 클라이언트에서 로컬로만 생성된다. */
UCLASS()
class VER1_API AMobaCameraRig : public AActor
{
	GENERATED_BODY()

public:
	AMobaCameraRig();

	/** 카메라가 바라보는 지면 위치를 옮긴다. 맵 범위를 벗어나지 않는다. */
	void SetFocusLocation(const FVector& NewLocation);

	/** 화면 기준 방향(X = 오른쪽, Y = 위)으로 이동한다. */
	void Pan(const FVector2D& ScreenDirection, float DeltaSeconds);

	void Zoom(float Amount);

	/** 가장자리 스크롤 속도(초당 거리). */
	void SetPanSpeed(float InSpeed) { PanSpeed = InSpeed; }
	float GetPanSpeed() const { return PanSpeed; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float PanSpeed = 3000.0f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float ZoomStep = 150.0f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float MinArmLength = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float MaxArmLength = 2600.0f;

	/** 카메라 초점이 머물 수 있는 맵 범위 (월드 XY). */
	UPROPERTY(EditAnywhere, Category = "Camera")
	FBox2D FocusBounds = FBox2D(FVector2D(0.0, 0.0), FVector2D(14870.0, 14870.0));
};
