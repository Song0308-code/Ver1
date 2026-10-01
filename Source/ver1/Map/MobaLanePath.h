#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Game/MobaTypes.h"
#include "MobaLanePath.generated.h"

class USplineComponent;

/** 한 라인의 경로. 블루 넥서스에서 레드 넥서스 방향으로 정의하며 미니언이 따라간다. */
UCLASS()
class VER1_API AMobaLanePath : public AActor
{
	GENERATED_BODY()

public:
	AMobaLanePath();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	EMobaLane GetLane() const { return Lane; }
	USplineComponent* GetSpline() const { return Spline; }

protected:
	void RebuildSpline();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USplineComponent> Spline;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lane")
	EMobaLane Lane = EMobaLane::Mid;

	/** 월드 좌표 경유점. 블루 쪽에서 레드 쪽 순서. 비어 있으면 스플라인을 직접 편집한 것으로 본다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lane")
	TArray<FVector> Waypoints;
};
