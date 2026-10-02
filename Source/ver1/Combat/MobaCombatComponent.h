#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MobaCombatComponent.generated.h"

class AMobaProjectile;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMobaAttackStarted, AActor* /*Target*/);

/**
 * 기본 공격을 담당한다.
 * 조종하는 쪽(플레이어의 클라이언트, AI는 서버)이 대상을 고르고 사거리 안까지 쫓아가며,
 * 서버는 사거리·공격 속도를 확인해 실제 공격을 수행한다.
 */
UCLASS(ClassGroup = (Moba), meta = (BlueprintSpawnableComponent))
class VER1_API UMobaCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMobaCombatComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * 조종하는 쪽 또는 서버에서 호출하는 직접 명령. 대상을 끝까지 쫓아가 공격한다.
	 * nullptr이면 공격과 공격 이동을 모두 취소한다.
	 */
	void SetAttackTarget(AActor* NewTarget);

	/** 조종하는 쪽에서 호출한다. 목적지로 걸어가면서 도중에 만나는 적을 공격하고, 처치하면 다시 걸어간다. */
	void StartAttackMove(const FVector& Destination);

	AActor* GetAttackTarget() const { return AttackTarget; }
	bool IsAttackMoving() const { return bAttackMoving; }

	float GetAttackRange() const;

	/** 서버 전용. 챔피언 데이터에 따라 기본 공격 방식을 정한다. */
	void ConfigureBasicAttack(bool bInRanged, float InProjectileSpeed);

	/** 투사체의 크기와, 소유자 위치를 기준으로 한 발사 지점. */
	void ConfigureProjectileVisual(float InVisualScale, const FVector& InSpawnOffset);

	/** 명령 없이 가만히 서 있을 때 사거리 안의 적을 알아서 공격할지. 플레이어 챔피언만 켠다. */
	void SetAutoAcquireWhenIdle(bool bEnabled) { bAutoAcquireWhenIdle = bEnabled; }

	/** 공격 선딜레이가 시작될 때 모든 클라이언트에서 호출된다. 애니메이션·사운드 연결용. */
	FOnMobaAttackStarted OnAttackStarted;

protected:
	UFUNCTION(Server, Reliable)
	void ServerSetAttackTarget(AActor* NewTarget);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAttackStarted(AActor* Target);

	/** 대상만 바꾸고 서버에 알린다. 공격 이동이나 자동 공격 상태는 건드리지 않는다. */
	void SetAttackTargetInternal(AActor* NewTarget);

	/** 조종하는 쪽에서 매 틱 실행한다. 대상이 있으면 쫓아가고, 없으면 공격 이동이나 자동 공격 대상을 찾는다. */
	void UpdateOrders();

	void UpdateChase(AController* Controller);
	void UpdateAttackMove(AController* Controller);
	void UpdateIdleAutoAcquire();

	/** 가장자리까지의 거리가 Range 이내인 적 중 가장 가까운 것. 정글 몬스터는 bIncludeNeutral일 때만 포함한다. */
	AActor* FindNearestEnemy(float Range, bool bIncludeStructures, bool bIncludeNeutral) const;

	bool CanOwnerAct() const;

	/** 플레이어가 조종하는 유닛은 시야 밖의 적을 노릴 수 없다. AI는 항상 true. */
	bool CanSeeTarget(const AActor* Target) const;

	/** 서버에서 선딜레이와 공격 주기를 진행한다. */
	void UpdateAttack();

	/** 선딜레이가 끝났을 때 대상에게 투사체를 쏜다. */
	void FireAttack(AActor* Target);

	AController* GetOwnerController() const;

	/** 원거리인지. 근접도 타격이 보이도록 아주 빠른 투사체를 쓴다(애니메이션이 들어오기 전까지). */
	UPROPERTY(EditAnywhere, Category = "Basic Attack")
	bool bRangedAttack = true;

	UPROPERTY(EditAnywhere, Category = "Basic Attack")
	TSubclassOf<AMobaProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Basic Attack", meta = (EditCondition = "bRangedAttack"))
	float ProjectileSpeed = 1600.0f;

	UPROPERTY(EditAnywhere, Category = "Basic Attack", meta = (EditCondition = "!bRangedAttack"))
	float MeleeProjectileSpeed = 3000.0f;

	/** 투사체 구의 크기 배율. 1이면 지름 100. */
	UPROPERTY(EditAnywhere, Category = "Basic Attack")
	float ProjectileVisualScale = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Basic Attack")
	FVector ProjectileSpawnOffset = FVector::ZeroVector;

	/** 공격 주기 중 선딜레이가 차지하는 비율. */
	UPROPERTY(EditAnywhere, Category = "Basic Attack", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WindupFraction = 0.1f;

	/** 레드 버프가 있을 때 기본 공격에 더해지는 고정 피해와 둔화. */
	UPROPERTY(EditAnywhere, Category = "Basic Attack")
	float RedBuffTrueDamage = 12.0f;

	UPROPERTY(EditAnywhere, Category = "Basic Attack")
	float RedBuffSlowMultiplier = 0.85f;

	UPROPERTY(EditAnywhere, Category = "Basic Attack")
	float RedBuffSlowDuration = 1.5f;

	/** 지연 시간 때문에 서버에서 사거리를 조금 넉넉하게 본다. */
	UPROPERTY(EditAnywhere, Category = "Basic Attack")
	float ServerRangeLeeway = 75.0f;

	UPROPERTY(EditAnywhere, Category = "Orders")
	bool bAutoAcquireWhenIdle = false;

	/** 공격 이동 중에는 사거리보다 이만큼 더 먼 적까지 찾아 쫓아간다. */
	UPROPERTY(EditAnywhere, Category = "Orders")
	float AttackMoveAcquisitionBonus = 150.0f;

	UPROPERTY(Transient)
	TObjectPtr<AActor> AttackTarget;

	// --- 조종하는 쪽의 상태 ---
	bool bChasing = false;
	double LastChaseTime = -1.0;

	/** 새 대상을 받은 뒤 아직 발을 멈추지 않았는지. 다른 곳으로 걷던 중에 공격 명령을 받으면 사거리 안에서 바로 멈추기 위해 쓴다. */
	bool bStopPending = false;

	/** 현재 대상이 가만히 있을 때 자동으로 잡힌 것인지. 그렇다면 사거리를 벗어날 때 쫓아가지 않고 놓는다. */
	bool bTargetAutoAcquired = false;

	bool bAttackMoving = false;
	bool bAttackMoveWalking = false;
	FVector AttackMoveDestination = FVector::ZeroVector;

	double LastOrderTime = -1.0;
	double LastScanTime = -1.0;
	double LastMovingTime = 0.0;

	// --- 서버의 상태 ---
	bool bWindingUp = false;
	double WindupEndTime = 0.0;
	double NextAttackTime = 0.0;
};
