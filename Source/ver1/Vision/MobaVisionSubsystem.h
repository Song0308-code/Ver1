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

	/** 화면에 안개를 그릴 수 있는 상태인지(이 PC의 플레이어 팀이 정해져 있는지). */
	bool HasFog() const { return bFogActive; }

	/** 안개 격자 한 변의 칸 수. 꼭짓점은 한 변에 칸 수 + 1개다. */
	static constexpr int32 FogCells = 96;

	/** 안개 격자가 덮는 월드 범위(한 변). 원점에서 +X, +Y 방향이다. */
	static constexpr float FogWorldSize = 14870.0f;

	/** 격자 꼭짓점이 아군 시야 안인지. 범위를 벗어나면 false. */
	bool IsFogVertexVisible(int32 X, int32 Y) const;

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

	/** 시야 원과 부쉬로 안개 격자를 다시 칠한다. 벽에 가려지는 것은 고려하지 않는다. */
	void UpdateFog(const TArray<FVisionSource>& Sources);

	TArray<TWeakObjectPtr<AMobaBush>> Bushes;

	/** 지금 숨겨 둔 유닛. */
	TSet<TWeakObjectPtr<AActor>> HiddenActors;

	float TimeSinceUpdate = 0.0f;

	/** 꼭짓점마다 1이면 보이고 0이면 안개. (FogCells + 1)^2개. */
	TArray<uint8> FogVisible;

	/** 꼭짓점마다 들어 있는 부쉬 비트. 부쉬가 바뀌면 다시 계산한다. */
	TArray<uint64> FogBushMasks;
	bool bFogBushMasksDirty = true;
	bool bFogActive = false;
};
