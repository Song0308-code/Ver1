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

	/** 1보다 작으면 맞은 적을 둔화시킨다. */
	UPROPERTY(EditDefaultsOnly, Category = "Slow")
	float SlowMoveSpeedMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Slow")
	float SlowDuration = 0.0f;

	FTimerHandle DetonateTimer;
};

/** 커서 쪽으로 짧게 돌진해 도착 지점 주변의 적에게 물리 피해를 준다. */
UCLASS()
class VER1_API UMobaAbility_DashStrike : public UMobaAbility_Blink
{
	GENERATED_BODY()

public:
	UMobaAbility_DashStrike();

	virtual float GetIndicatorRadius() const override { return Radius; }

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float Radius = 220.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float BaseDamage = 70.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamagePerLevel = 40.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AttackDamageRatio = 0.8f;

	/** 0보다 크면 맞은 적을 이 시간 동안 침묵시킨다. */
	UPROPERTY(EditDefaultsOnly, Category = "Stun")
	float SilenceDuration = 0.0f;
};

/** 즉시 발동. 자신의 체력을 회복한다. 잃은 체력이 많을수록 더 많이 회복한다. */
UCLASS()
class VER1_API UMobaAbility_SelfHeal : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_SelfHeal();

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Heal")
	float BaseHeal = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Heal")
	float HealPerLevel = 30.0f;

	/** 잃은 체력 중 추가로 회복하는 비율. */
	UPROPERTY(EditDefaultsOnly, Category = "Heal")
	float MissingHealthFraction = 0.1f;
};

/** 즉시 발동. 자기 주변의 적에게 물리 피해와 둔화를 준다. */
UCLASS()
class VER1_API UMobaAbility_GroundSlam : public UMobaAbility_GroundAoE
{
	GENERATED_BODY()

public:
	UMobaAbility_GroundSlam();
};

/** 궁극기. 한동안 이동 속도와 공격 속도가 크게 오른다. */
UCLASS()
class VER1_API UMobaAbility_Frenzy : public UMobaAbility_SelfBuff
{
	GENERATED_BODY()

public:
	UMobaAbility_Frenzy();
};

/** 미스틱 Q. 사거리가 길고 둔화 없는 마법 투사체. */
UCLASS()
class VER1_API UMobaAbility_ArcaneBolt : public UMobaAbility_Skillshot
{
	GENERATED_BODY()

public:
	UMobaAbility_ArcaneBolt();
};

/** 미스틱 W. 잠시 뒤 범위 안의 적에게 마법 피해를 주고 크게 둔화시킨다. */
UCLASS()
class VER1_API UMobaAbility_GravityField : public UMobaAbility_GroundAoE
{
	GENERATED_BODY()

public:
	UMobaAbility_GravityField();
};

/** 미스틱 E. 짧은 순간이동. */
UCLASS()
class VER1_API UMobaAbility_PhaseShift : public UMobaAbility_Blink
{
	GENERATED_BODY()

public:
	UMobaAbility_PhaseShift();
};

/** 미스틱 R. 넓은 범위에 큰 마법 피해와 기절. */
UCLASS()
class VER1_API UMobaAbility_Meteor : public UMobaAbility_GroundAoE
{
	GENERATED_BODY()

public:
	UMobaAbility_Meteor();
};

/** 섀도우 Q. 커서 근처의 적 등 뒤로 순간이동해 물리 피해를 준다. */
UCLASS()
class VER1_API UMobaAbility_ShadowStep : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_ShadowStep();

	virtual float GetIndicatorRadius() const override { return TargetPickRadius; }

protected:
	virtual bool ResolveTarget(const AMobaCharacter* Caster, FVector& InOutTargetLocation) const override;
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Target")
	float TargetPickRadius = 250.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float BaseDamage = 70.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamagePerLevel = 35.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AttackDamageRatio = 0.7f;

	mutable TWeakObjectPtr<AActor> PendingTarget;
};

/** 섀도우 W. 잠시 은신하고 빨라진다. */
UCLASS()
class VER1_API UMobaAbility_ShadowCloak : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_ShadowCloak();

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Buff")
	float StealthDuration = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Buff")
	float MoveSpeedMultiplier = 1.2f;
};

/** 섀도우 E. 지정한 곳에 연막을 터뜨려 피해와 둔화를 준다. */
UCLASS()
class VER1_API UMobaAbility_SmokeBomb : public UMobaAbility_GroundAoE
{
	GENERATED_BODY()

public:
	UMobaAbility_SmokeBomb();
};

/** 섀도우 R. 커서 근처의 적 챔피언에게 큰 물리 피해와 잃은 체력에 비례한 추가 피해를 준다. */
UCLASS()
class VER1_API UMobaAbility_Execute : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_Execute();

	virtual float GetIndicatorRadius() const override { return TargetPickRadius; }

protected:
	virtual bool ResolveTarget(const AMobaCharacter* Caster, FVector& InOutTargetLocation) const override;
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Target")
	float TargetPickRadius = 250.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float BaseDamage = 150.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamagePerLevel = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AttackDamageRatio = 0.6f;

	/** 대상이 잃은 체력에 곱해 더하는 물리 피해 비율. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float MissingHealthRatio = 0.3f;

	mutable TWeakObjectPtr<AActor> PendingTarget;
};

/** 세이지 Q. 지정한 곳의 적에게 마법 피해를 주고, 범위 안의 아군 챔피언(자신 포함)을 회복한다. */
UCLASS()
class VER1_API UMobaAbility_HealingWave : public UMobaAbility_GroundAoE
{
	GENERATED_BODY()

public:
	UMobaAbility_HealingWave();

protected:
	virtual void OnDetonated(AMobaCharacter* Caster, const FVector& Location) override;

	UPROPERTY(EditDefaultsOnly, Category = "Heal")
	float BaseHeal = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Heal")
	float HealPerLevel = 25.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Heal")
	float HealAbilityPowerRatio = 0.3f;
};

/** 세이지 W. 커서 근처의 아군 챔피언과 자신에게 보호막을 준다. 커서 근처에 아군이 없으면 자신에게만 준다. */
UCLASS()
class VER1_API UMobaAbility_GuardianLight : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_GuardianLight();

	virtual float GetIndicatorRadius() const override { return AllyPickRadius; }

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Target")
	float AllyPickRadius = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float BaseShield = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float ShieldPerLevel = 30.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float AbilityPowerRatio = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Shield")
	float ShieldDuration = 2.5f;
};

/** 세이지 E. 자신과 주변 아군 챔피언의 이동 속도를 잠시 올린다. */
UCLASS()
class VER1_API UMobaAbility_BlessedWind : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_BlessedWind();

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Buff")
	float Radius = 600.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Buff")
	float Duration = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Buff")
	float MoveSpeedMultiplier = 1.3f;
};

/** 세이지 R. 넓은 범위의 아군 챔피언(자신 포함)을 크게 회복하고 기절·속박·침묵·둔화를 풀어 준다. */
UCLASS()
class VER1_API UMobaAbility_Sanctuary : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaAbility_Sanctuary();

	virtual float GetIndicatorRadius() const override { return Radius; }

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Heal")
	float Radius = 900.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Heal")
	float BaseHeal = 150.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Heal")
	float HealPerLevel = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Heal")
	float AbilityPowerRatio = 0.5f;
};
