#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/MobaGameplayAbility.h"
#include "MobaAbilityArchetypes.generated.h"

class AMobaAreaIndicator;
class AMobaSkillshotProjectile;

/** 커서 방향으로 직선 투사체를 쏜다. 처음 맞은 적에게 마법 피해와 둔화를 준다. */
UCLASS()
class VER1_API UMobaAbility_Skillshot : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_Skillshot();

	virtual float GetIndicatorRadius() const override { return ProjectileRadius; }

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Skillshot")
	TSubclassOf<AMobaSkillshotProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Skillshot")
	float ProjectileSpeed = 1800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Skillshot")
	float ProjectileRadius = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float BaseDamage = 80.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamagePerLevel = 45.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AbilityPowerRatio = 0.6f;

	/** 1보다 작으면 둔화. 1이면 둔화 없음. */
	UPROPERTY(EditDefaultsOnly, Category = "Slow")
	float SlowMoveSpeedMultiplier = 0.7f;

	UPROPERTY(EditDefaultsOnly, Category = "Slow")
	float SlowDuration = 2.0f;
};

/** 즉시 발동. 일정 시간 동안 자신의 이동 속도와 공격 속도를 올린다. */
UCLASS()
class VER1_API UMobaAbility_SelfBuff : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_SelfBuff();

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Buff")
	float BuffDuration = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Buff")
	float MoveSpeedMultiplier = 1.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Buff")
	float AttackSpeedMultiplier = 1.4f;
};

/** 커서 방향으로 사거리 안에서 순간이동한다. 벽을 넘을 수 있지만 걸을 수 없는 곳에는 도착할 수 없다. */
UCLASS()
class VER1_API UMobaAbility_Blink : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_Blink();

protected:
	virtual bool ResolveTarget(const AMobaCharacter* Caster, FVector& InOutTargetLocation) const override;
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	/** 도착 지점 주변에서 걸을 수 있는 곳을 찾는 범위. */
	UPROPERTY(EditDefaultsOnly, Category = "Blink")
	FVector NavProjectExtent = FVector(150.0, 150.0, 300.0);
};

/** 지정한 위치에 잠시 뒤 폭발을 일으켜 범위 안의 적에게 마법 피해와 기절을 준다. */
UCLASS()
class VER1_API UMobaAbility_GroundAoE : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_GroundAoE();

	virtual float GetIndicatorRadius() const override { return Radius; }

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	void Detonate(FVector Location);

	UPROPERTY(EditDefaultsOnly, Category = "Area")
	TSubclassOf<AMobaAreaIndicator> IndicatorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Area")
	float Radius = 300.0f;

	/** 시전 후 폭발까지의 시간(초). */
	UPROPERTY(EditDefaultsOnly, Category = "Area")
	float Delay = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float BaseDamage = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamagePerLevel = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AbilityPowerRatio = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	float StunDuration = 1.5f;

	FTimerHandle DetonateTimer;
};
