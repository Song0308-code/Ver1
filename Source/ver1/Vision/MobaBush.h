#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MobaBush.generated.h"

class UBoxComponent;

/**
 * 부쉬 영역. 안에 있는 유닛은 같은 부쉬 안에 아군 시야가 없는 적에게 보이지 않는다.
 * 상자의 기본 크기는 한 변 100이고, 액터 스케일로 실제 크기를 정한다.
 */
UCLASS()
class VER1_API AMobaBush : public AActor
{
	GENERATED_BODY()

public:
	AMobaBush();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 높이는 무시하고 수평 위치만으로 판정한다. */
	bool ContainsLocation(const FVector& WorldLocation) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Box;
};
