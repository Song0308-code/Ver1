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

	/** 폭발할 때 적 처리가 끝난 뒤 불린다. 아군에게 효과를 주는 스킬이 덮어쓴다. */
	virtual void OnDetonated(AMobaCharacter* Caster, const FVector& Location) {}

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

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AttackDamageRatio = 0.0f;

	/** true면 물리 피해, false면 마법 피해. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	bool bPhysicalDamage = false;

	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	float StunDuration = 1.5f;

	/** 0보다 크면 맞은 적을 이 시간 동안 띄운다(에어본). */
	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	float AirborneDuration = 0.0f;

	/** 0보다 크면 맞은 적을 이 시간 동안 속박한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	float RootDuration = 0.0f;

	/** 0보다 크면 맞은 적을 이 시간 동안 침묵시킨다. */
	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	float SilenceDuration = 0.0f;

	/** 1보다 작으면 맞은 적을 둔화시킨다. */
	UPROPERTY(EditDefaultsOnly, Category = "Slow")
	float SlowMoveSpeedMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Slow")
	float SlowDuration = 0.0f;

	FTimerHandle DetonateTimer;
};

/** 깡통 Q. 직선으로 갈고리를 던져 처음 맞은 적에게 마법 피해를 주고 자기 앞으로 끌어온다. */
UCLASS()
class VER1_API UMobaAbility_TinHook : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_TinHook();

	virtual float GetIndicatorRadius() const override { return HookRadius; }

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Hook")
	float HookSpeed = 1800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Hook")
	float HookRadius = 70.0f;

	/** 끌어온 대상이 멈추는 곳(시전자 앞 거리). */
	UPROPERTY(EditDefaultsOnly, Category = "Hook")
	float PullStopDistance = 150.0f;

	/** 끌려오는 데 걸리는 시간. 그동안 대상은 띄워져 아무것도 못 한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Hook")
	float PullDuration = 0.45f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float BaseDamage = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamagePerLevel = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AbilityPowerRatio = 0.9f;
};

/** 깡통 W. 잠깐 크게 빨라진 뒤 과열로 느려진다. */
UCLASS()
class VER1_API UMobaAbility_TinOverdrive : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_TinOverdrive();

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Buff")
	float HasteDuration = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Buff")
	float HasteMultiplier = 1.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Buff")
	float AttackSpeedMultiplier = 1.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Buff")
	float OverheatDuration = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Buff")
	float OverheatMultiplier = 0.75f;
};

/** 깡통 E. 5초 안의 다음 기본 공격이 추가 물리 피해를 주고 대상을 띄운다. */
UCLASS()
class VER1_API UMobaAbility_TinPowerPunch : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_TinPowerPunch();

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Punch")
	float Duration = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Punch")
	float AttackDamageRatio = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Punch")
	float AirborneDuration = 1.0f;
};

/** 깡통 R. 주변 적에게 마법 피해를 주고 침묵시킨다. */
UCLASS()
class VER1_API UMobaAbility_TinDischarge : public UMobaAbility_GroundAoE
{
	GENERATED_BODY()

public:
	UMobaAbility_TinDischarge();
};

/** 젖소 Q. 주변 땅을 내리쳐 적에게 마법 피해를 주고 띄운다. */
UCLASS()
class VER1_API UMobaAbility_CowSlam : public UMobaAbility_GroundAoE
{
	GENERATED_BODY()

public:
	UMobaAbility_CowSlam();

protected:
	virtual void OnDetonated(AMobaCharacter* Caster, const FVector& Location) override;
};

/** 젖소 W. 커서 근처의 적에게 달려가 들이받아 반대쪽으로 밀어낸다. */
UCLASS()
class VER1_API UMobaAbility_CowCharge : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_CowCharge();

	virtual float GetIndicatorRadius() const override { return TargetPickRadius; }

protected:
	virtual bool ResolveTarget(const AMobaCharacter* Caster, FVector& InOutTargetLocation) const override;
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Target")
	float TargetPickRadius = 250.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Charge")
	float DashDuration = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Charge")
	float KnockbackDistance = 450.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Charge")
	float KnockbackDuration = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float BaseDamage = 55.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamagePerLevel = 40.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AbilityPowerRatio = 0.7f;

	mutable TWeakObjectPtr<AActor> PendingTarget;
};

/** 젖소 E. 주변 아군 챔피언(자신 포함)에게 보호막을 준다. */
UCLASS()
class VER1_API UMobaAbility_CowBellow : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_CowBellow();

	virtual float GetIndicatorRadius() const override { return Radius; }

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float Radius = 600.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float BaseShield = 70.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float ShieldPerLevel = 35.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float AbilityPowerRatio = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float ShieldDuration = 2.5f;
};

/** 젖소 R. 걸린 군중 제어를 풀고 한동안 받는 피해가 크게 줄어든다. */
UCLASS()
class VER1_API UMobaAbility_CowUnbreakable : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_CowUnbreakable();

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Buff")
	float Duration = 6.0f;
};

/** 노머고 Q. 물리 피해 직선탄. 맞으면 다른 스킬의 재사용 대기가 줄어든다. */
UCLASS()
class VER1_API UMobaAbility_NomagoPulse : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_NomagoPulse();

	virtual float GetIndicatorRadius() const override { return ProjectileRadius; }

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Skillshot")
	float ProjectileSpeed = 2000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Skillshot")
	float ProjectileRadius = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float BaseDamage = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamagePerLevel = 25.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AttackDamageRatio = 1.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AbilityPowerRatio = 0.3f;

	/** 맞았을 때 W·E·R의 남은 재사용 대기에서 빼는 시간. */
	UPROPERTY(EditDefaultsOnly, Category = "Skillshot")
	float CooldownRefund = 1.5f;
};

/** 노머고 W. 적을 꿰뚫고 날아가며 챔피언에게 표식을 남긴다. */
UCLASS()
class VER1_API UMobaAbility_NomagoMark : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_NomagoMark();

	virtual float GetIndicatorRadius() const override { return ProjectileRadius; }

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Skillshot")
	float ProjectileSpeed = 1600.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Skillshot")
	float ProjectileRadius = 80.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mark")
	float MarkDuration = 4.0f;
};

/** 노머고 E. 짧게 순간이동한 뒤 근처의 적(챔피언 우선)에게 유도탄을 쏜다. */
UCLASS()
class VER1_API UMobaAbility_NomagoPhase : public UMobaAbility_Blink
{
	GENERATED_BODY()

public:
	UMobaAbility_NomagoPhase();

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Bolt")
	float BoltSearchRadius = 750.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bolt")
	float BoltSpeed = 2000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float BaseDamage = 80.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamagePerLevel = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AttackDamageRatio = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AbilityPowerRatio = 0.75f;
};

/** 노머고 R. 1초 충전한 뒤 맵을 가로지르는 넓은 포격을 쏜다. 챔피언이 아닌 대상은 절반 피해. */
UCLASS()
class VER1_API UMobaAbility_NomagoBarrage : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_NomagoBarrage();

	virtual float GetIndicatorRadius() const override { return BeamRadius; }

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	float ChargeTime = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	float BeamSpeed = 2400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	float BeamRadius = 160.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float BaseDamage = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamagePerLevel = 150.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AttackDamageRatio = 0.9f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AbilityPowerRatio = 0.9f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float NonChampionDamageScale = 0.5f;
};

/** 진진자라 Q. 커서 근처의 적에게 탄을 쏘고, 맞을 때마다 근처의 다른 적에게 튕긴다(최대 4명). */
UCLASS()
class VER1_API UMobaAbility_JinRicochet : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_JinRicochet();

	virtual float GetIndicatorRadius() const override { return TargetPickRadius; }

protected:
	virtual bool ResolveTarget(const AMobaCharacter* Caster, FVector& InOutTargetLocation) const override;
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Target")
	float TargetPickRadius = 250.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bounce")
	int32 MaxTargets = 4;

	UPROPERTY(EditDefaultsOnly, Category = "Bounce")
	float BounceRadius = 450.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bounce")
	float BoltSpeed = 1800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float BaseDamage = 45.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamagePerLevel = 25.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AttackDamageRatio = 0.5f;

	mutable TWeakObjectPtr<AActor> PendingTarget;
};

/** 진진자라 W. 잠시 조준한 뒤 아주 긴 사거리로 저격한다. 미니언은 꿰뚫고 처음 맞은 챔피언에서 멈추며, 표식이 있으면 속박한다. */
UCLASS()
class VER1_API UMobaAbility_JinAimedShot : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_JinAimedShot();

	virtual float GetIndicatorRadius() const override { return ShotRadius; }

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	float AimTime = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	float ShotSpeed = 5000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	float ShotRadius = 45.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	float RootDuration = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float BaseDamage = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamagePerLevel = 40.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AttackDamageRatio = 0.5f;

	/** 미니언·몬스터가 받는 피해 비율. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float NonChampionDamageScale = 0.75f;
};

/** 진진자라 E. 지정한 곳에 덫을 놓는다(최대 2개, 60초). */
UCLASS()
class VER1_API UMobaAbility_JinTrap : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_JinTrap();

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Trap")
	int32 MaxTraps = 2;

	UPROPERTY(EditDefaultsOnly, Category = "Trap")
	float ArmDelay = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Trap")
	float Lifetime = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float BaseDamage = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamagePerLevel = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AttackDamageRatio = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AbilityPowerRatio = 1.0f;

	TArray<TWeakObjectPtr<AActor>> ActiveTraps;
};

/** 진진자라 R. 제자리에 서서 조준한 방향으로 강한 탄 세 발을 쏜다. 대상이 잃은 체력이 많을수록 아프고 세 번째 발은 더 강하다. */
UCLASS()
class VER1_API UMobaAbility_JinCurtainCall : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_JinCurtainCall();

	virtual float GetIndicatorRadius() const override { return ShotRadius; }

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	int32 ShotCount = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	float FirstShotDelay = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	float ShotInterval = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	float ShotSpeed = 4000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	float ShotRadius = 80.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float BaseDamage = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamagePerLevel = 70.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AttackDamageRatio = 0.25f;

	/** 마지막 발의 피해 배율. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float LastShotMultiplier = 1.5f;
};
