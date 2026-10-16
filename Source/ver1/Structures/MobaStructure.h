#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "Game/MobaTypes.h"
#include "MobaStructure.generated.h"

class UMobaCombatComponent;
class UStaticMeshComponent;

/**
 * 타워·억제기·넥서스의 공통 기반. 레벨에 직접 배치하며 팀과 보호 관계를 인스턴스에서 지정한다.
 * 앞선 건물이 남아 있는 동안에는 무적이라 공격 대상이 되지 않는다.
 */
UCLASS(Abstract)
class VER1_API AMobaStructure : public AActor, public IMobaTeamInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMobaStructure();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** 모든 PC에서 사망 태그가 붙는 순간 사라지는 이펙트를 보여 준다. */
	void HandleDeadTagForEffects(const FGameplayTag Tag, int32 NewCount);

	virtual EMobaTeam GetTeam() const override { return Team; }
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }

	EMobaStructureType GetStructureType() const { return StructureType; }
	EMobaLane GetLane() const { return Lane; }

	bool IsDestroyed() const;
	bool IsInvulnerable() const;

	/** 머리 위 체력바를 그릴 높이(지면 기준). */
	float GetHealthBarHeight() const { return HealthBarHeight; }

	float GetGoldReward() const { return GoldReward; }

	/** 서버 전용. 보호해 주는 건물이 남아 있는지 다시 확인해 무적 여부를 갱신한다. */
	void UpdateProtection();

protected:
	UFUNCTION()
	void OnRep_Team();

	void ApplyTeamColor();

	virtual void HandleOutOfHealth(AActor* DamageInstigator);

	/** 서버 전용. 파괴된 건물을 체력을 가득 채워 되살리고 보호 관계를 다시 계산한다. */
	void Revive();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UMobaAttributeSet> AttributeSet;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Team, Category = "Structure")
	EMobaTeam Team = EMobaTeam::None;

	UPROPERTY(EditAnywhere, Category = "Structure")
	EMobaLane Lane = EMobaLane::Mid;

	/** Outer, Inner, Inhibitor 등 사람이 알아보기 위한 이름. */
	UPROPERTY(EditAnywhere, Category = "Structure")
	FName Tier;

	/** 이 건물들이 전부 파괴되어야 공격할 수 있다. */
	UPROPERTY(EditInstanceOnly, Category = "Structure")
	TArray<TObjectPtr<AMobaStructure>> RequiresAllDestroyed;

	/** 이 건물들 중 하나라도 파괴되어야 공격할 수 있다. 비어 있으면 조건 없음. */
	UPROPERTY(EditInstanceOnly, Category = "Structure")
	TArray<TObjectPtr<AMobaStructure>> RequiresAnyDestroyed;

	UPROPERTY(EditAnywhere, Category = "Structure")
	FMobaBaseStats Stats;

	/** 파괴한 쪽 챔피언 전원이 받는 골드. */
	UPROPERTY(EditAnywhere, Category = "Structure")
	float GoldReward = 0.0f;

	EMobaStructureType StructureType = EMobaStructureType::Tower;
	float HealthBarHeight = 300.0f;
};

/** 사거리 안의 적을 공격하는 타워. 미니언을 먼저 노리고, 아군 챔피언을 공격한 적 챔피언으로 대상을 바꾼다. */
UCLASS()
class VER1_API AMobaTower : public AMobaStructure
{
	GENERATED_BODY()

public:
	AMobaTower();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void HandleOutOfHealth(AActor* DamageInstigator) override;

	void UpdateTargeting();
	AActor* FindBestTarget(float Range) const;
	void HandleDamageDealt(AActor* DamageInstigator, AActor* Victim, float Damage);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMobaCombatComponent> CombatComponent;

	UPROPERTY(EditAnywhere, Category = "Tower")
	float ProjectileSpeed = 1200.0f;

	FTimerHandle TargetingTimer;
	FDelegateHandle DamageDealtHandle;
};

UCLASS()
class VER1_API AMobaInhibitor : public AMobaStructure
{
	GENERATED_BODY()

public:
	AMobaInhibitor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 다시 생길 때까지 남은 시간(초). 파괴되지 않았으면 0. */
	float GetSecondsUntilRespawn() const;

protected:
	virtual void HandleOutOfHealth(AActor* DamageInstigator) override;

	/** 파괴된 뒤 다시 생기기까지의 시간(초). */
	UPROPERTY(EditAnywhere, Category = "Inhibitor")
	float RespawnDelay = 300.0f;

	/** 다시 생기는 시각(서버 월드 시간). 파괴되지 않았으면 0. */
	UPROPERTY(Replicated)
	float RespawnServerTime = 0.0f;

	FTimerHandle RespawnTimer;
};

/** 파괴되면 경기가 끝난다. */
UCLASS()
class VER1_API AMobaNexus : public AMobaStructure
{
	GENERATED_BODY()

public:
	AMobaNexus();
};
