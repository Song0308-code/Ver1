#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MobaCombatComponent.generated.h"

class AMobaProjectile;
class UGameplayEffect;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMobaAttackStarted, AActor* /*Target*/);

/**
 * 기본 공격을 담당한다.
 * 조종하는 클라이언트는 대상을 사거리 안까지 쫓아가고, 서버는 사거리·공격 속도를 확인해 실제 공격을 수행한다.
 */
UCLASS(ClassGroup = (Moba), meta = (BlueprintSpawnableComponent))
class VER1_API UMobaCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMobaCombatComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 조종하는 클라이언트 또는 서버에서 호출한다. nullptr이면 공격 명령을 취소한다. */
	void SetAttackTarget(AActor* NewTarget);

	AActor* GetAttackTarget() const { return AttackTarget; }

	float GetAttackRange() const;

	/** 공격 선딜레이가 시작될 때 모든 클라이언트에서 호출된다. 애니메이션·사운드 연결용. */
	FOnMobaAttackStarted OnAttackStarted;

protected:
	UFUNCTION(Server, Reliable)
	void ServerSetAttackTarget(AActor* NewTarget);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAttackStarted(AActor* Target);

	/** 조종하는 쪽에서 대상을 사거리 안까지 쫓아간다. */
	void UpdateChase();

	/** 서버에서 선딜레이와 공격 주기를 진행한다. */
	void UpdateAttack();

	/** 선딜레이가 끝났을 때 피해를 주거나 투사체를 쏜다. */
	void FireAttack(AActor* Target);

	AController* GetOwnerController() const;

	/** true면 투사체를 쏘고, false면 선딜레이 직후 바로 피해를 준다. */
	UPROPERTY(EditAnywhere, Category = "Basic Attack")
	bool bRangedAttack = true;

	UPROPERTY(EditAnywhere, Category = "Basic Attack", meta = (EditCondition = "bRangedAttack"))
	TSubclassOf<AMobaProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Basic Attack", meta = (EditCondition = "bRangedAttack"))
	float ProjectileSpeed = 2000.0f;

	/** 공격 주기 중 선딜레이가 차지하는 비율. */
	UPROPERTY(EditAnywhere, Category = "Basic Attack", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WindupFraction = 0.25f;

	/** 지연 시간 때문에 서버에서 사거리를 조금 넉넉하게 본다. */
	UPROPERTY(EditAnywhere, Category = "Basic Attack")
	float ServerRangeLeeway = 75.0f;

	UPROPERTY(EditAnywhere, Category = "Basic Attack")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(Transient)
	TObjectPtr<AActor> AttackTarget;

	bool bChasing = false;
	double LastChaseTime = -1.0;

	bool bWindingUp = false;
	double WindupEndTime = 0.0;
	double NextAttackTime = 0.0;
};
