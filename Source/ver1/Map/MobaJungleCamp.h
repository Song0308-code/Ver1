#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Game/MobaTypes.h"
#include "Units/MobaMonster.h"
#include "MobaJungleCamp.generated.h"

/** 캠프 종류별 기본 구성. */
struct FMobaCampDefinition
{
	FString DisplayName;

	/** 경기 시작 후 처음 나오는 시각(초). */
	float InitialSpawnTime = 90.0f;

	/** 전멸한 뒤 다시 나오기까지의 시간(초). */
	float RespawnTime = 135.0f;

	TArray<FMobaMonsterConfig> Monsters;
};

/**
 * 정글 캠프. 레벨에 배치하고 종류만 지정하면 정해진 시각에 몬스터를 내보낸다.
 * 몬스터가 전멸하면 보상을 주고 재생성 시간을 센다.
 */
UCLASS()
class VER1_API AMobaJungleCamp : public AActor
{
	GENERATED_BODY()

public:
	AMobaJungleCamp();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	static FMobaCampDefinition GetDefinition(EMobaCampType Type);

	EMobaCampType GetCampType() const { return CampType; }

	/** 몬스터가 한 마리라도 살아 있는지. */
	bool IsAlive() const { return AliveCount > 0; }

	/** 다음 몬스터가 나오기까지 남은 시간(초). 살아 있거나 경기 시작 전이면 0. */
	float GetSecondsUntilSpawn() const;

	/** 서버 전용. 이 캠프의 몬스터가 죽었을 때 몬스터가 호출한다. */
	void HandleMonsterKilled(AMobaMonster* Monster, AActor* Killer);

	/** 서버 전용. 경기가 끝났을 때 더 나오지 않게 한다. */
	void StopSpawning();

protected:
	/** 나올 시각이 됐는지 확인한다. */
	void UpdateSpawn();

	void SpawnMonsters();

	UPROPERTY(EditAnywhere, Replicated, Category = "Camp")
	EMobaCampType CampType = EMobaCampType::Gromp;

	/** 0 이상이면 종류별 기본값 대신 이 값을 쓴다. */
	UPROPERTY(EditAnywhere, Category = "Camp")
	float InitialSpawnTimeOverride = -1.0f;

	UPROPERTY(EditAnywhere, Category = "Camp")
	float RespawnTimeOverride = -1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camp")
	TSubclassOf<AMobaMonster> MonsterClass;

	/** 다음에 나올 시각(경기 시간 기준). */
	UPROPERTY(Replicated)
	float NextSpawnMatchTime = 0.0f;

	UPROPERTY(Replicated)
	int32 AliveCount = 0;

#if WITH_EDITORONLY_DATA
	/** 에디터 자동 검증용. 켜면 기다리지 않고 바로 내보낸다. */
	UPROPERTY(EditAnywhere, Transient, Category = "Debug")
	bool bDebugSpawnNow = false;
#endif

	FTimerHandle SpawnTimer;
};
