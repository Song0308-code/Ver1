#pragma once

#include "CoreMinimal.h"
#include "Game/MobaTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "MobaVisionSubsystem.generated.h"

class AMobaBush;

/**
 * 이 PC의 플레이어 팀이 무엇을 볼 수 있는지 계산하고, 보이지 않는 적 유닛을 화면에서 숨긴다.
 *
 * 아군 챔피언·미니언·건물·와드가 시야를 제공한다. 벽 너머와, 아군 시야가 함께 들어가 있지 않은 부쉬 안은 보이지 않는다.
 * 계산은 각 PC에서 따로 하며 서버의 판정에는 영향을 주지 않는다(위치 정보 자체는 계속 복제된다).
 */
UCLASS()
class VER1_API UMobaVisionSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UMobaVisionSubsystem* Get(const UObject* WorldContextObject);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/** 이 PC의 플레이어에게 지금 보이는 유닛인지. 아군과 건물은 항상 보인다. */
	bool IsVisibleToLocalPlayer(const AActor* Actor) const;

	void RegisterBush(AMobaBush* Bush);
	void UnregisterBush(AMobaBush* Bush);

protected:
	struct FVisionSource
	{
		FVector Location;
		float Radius;
		uint64 BushMask;
	};

	void UpdateVision(EMobaTeam LocalTeam);

	/** 해당 위치가 들어 있는 부쉬들을 비트로 돌려준다. */
	uint64 GetBushMask(const FVector& Location) const;

	bool CanAnySourceSee(const TArray<FVisionSource>& Sources, const FVector& TargetLocation) const;

	/** 숨김 상태가 바뀐 유닛의 외형을 켜거나 끈다. */
	void SetActorShown(AActor* Actor, bool bShown) const;

	TArray<TWeakObjectPtr<AMobaBush>> Bushes;

	/** 지금 숨겨 둔 유닛. */
	TSet<TWeakObjectPtr<AActor>> HiddenActors;

	float TimeSinceUpdate = 0.0f;
};
