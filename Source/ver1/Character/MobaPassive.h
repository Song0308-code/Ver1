#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Game/MobaTypes.h"
#include "MobaPassive.generated.h"

class AMobaCharacter;

/** 패시브가 기본 공격 한 번에 더하는 피해와 효과. */
struct FMobaAttackBonus
{
	float BonusPhysicalDamage = 0.0f;
	float BonusMagicDamage = 0.0f;
	float ManaRestore = 0.0f;

	/** 0보다 크면 맞은 대상을 이 시간 동안 띄운다. */
	float AirborneDuration = 0.0f;
};

/**
 * 챔피언의 고유 패시브. 서버에서 챔피언마다 하나씩 만들어지고, 챔피언이 전투 사건을 알려 준다.
 * 화면에 보여 줄 중첩 수는 챔피언의 복제 값(PassiveStacks)으로 클라이언트에 전해진다.
 */
UCLASS(Abstract)
class VER1_API UMobaPassive : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AMobaCharacter* InOwner) { Owner = InOwner; }

	const FText& GetDisplayName() const { return DisplayName; }
	const FText& GetDescription() const { return Description; }

	/** HUD 패시브 칸에 숫자로 보여 줄 최대 중첩. 0이면 숫자 대신 켜짐·꺼짐만 보여 준다. */
	int32 GetMaxStacks() const { return MaxStacks; }

	/** 기본 공격 투사체를 쏘기 직전. 이번 공격에 더할 것을 돌려준다. */
	virtual FMobaAttackBonus OnBasicAttack(AActor* Target) { return FMobaAttackBonus(); }

	/** 스킬이 발동됐을 때. */
	virtual void OnAbilityCast(EMobaAbilitySlot Slot) {}

	/** 피해를 받았을 때. */
	virtual void OnDamageTaken(float Damage) {}

	/** 0.25초마다(챔피언의 재생 주기). 죽어 있을 때는 오지 않는다. */
	virtual void Tick(float DeltaSeconds) {}

	/** 부활했을 때. */
	virtual void OnRespawned() {}

	/** 이 챔피언의 스킬이 적 챔피언에게 군중 제어를 걸었을 때. */
	virtual void OnCrowdControlApplied(AActor* Target) {}

	/** 이 챔피언의 스킬이 적에게 맞았을 때. */
	virtual void OnAbilityHit(AActor* Target) {}

	/** 기본 공격을 시작해도 되는지(탄창이 비어 재장전 중이면 false). */
	virtual bool CanStartBasicAttack() const { return true; }

	/** 기본 공격 주기에 쓸 공격 속도. 기본은 능력치 그대로. */
	virtual float ModifyAttackSpeed(float AttackSpeed) const { return AttackSpeed; }

protected:
	/** 챔피언의 중첩 표시를 바꾼다. */
	void SetStacks(int32 Stacks) const;

	UPROPERTY(Transient)
	TObjectPtr<AMobaCharacter> Owner;

	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	FText Description;

	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	int32 MaxStacks = 0;
};

/** 깡통: 체력이 30% 아래로 떨어지면 현재 마나의 40%를 써서 그만큼 보호막을 얻는다(90초마다). */
UCLASS()
class VER1_API UMobaPassive_ManaBarrier : public UMobaPassive
{
	GENERATED_BODY()

public:
	UMobaPassive_ManaBarrier();

	virtual void OnDamageTaken(float Damage) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnRespawned() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float HealthThreshold = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float ManaFraction = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float ShieldDuration = 6.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float Cooldown = 90.0f;

	double ReadyTime = 0.0;
};

/** 젖소: 스킬로 적 챔피언에게 군중 제어를 걸면 주변 아군 챔피언(자신 포함)을 회복한다. */
UCLASS()
class VER1_API UMobaPassive_Rumination : public UMobaPassive
{
	GENERATED_BODY()

public:
	UMobaPassive_Rumination();

	virtual void OnCrowdControlApplied(AActor* Target) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float Radius = 700.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float BaseHeal = 20.0f;

	/** 젖소 최대 체력에 곱해 더하는 회복량. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float MaxHealthRatio = 0.02f;

	/** 다시 발동하기까지의 최소 간격(초). */
	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float InternalCooldown = 2.0f;

	double ReadyTime = 0.0;
};

/**
 * 노머고: 스킬을 맞힐 때마다 6초 동안 공격 속도가 8% 오른다(최대 5중첩).
 * W의 표식이 붙은 적을 기본 공격이나 다른 스킬로 맞히면 표식이 터져 마법 피해를 준다.
 */
UCLASS()
class VER1_API UMobaPassive_Overclock : public UMobaPassive
{
	GENERATED_BODY()

public:
	UMobaPassive_Overclock();

	virtual FMobaAttackBonus OnBasicAttack(AActor* Target) override;
	virtual void OnAbilityHit(AActor* Target) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnRespawned() override;

	/** 표식이 터질 때의 마법 피해. */
	float GetMarkDamage() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float AttackSpeedPerStack = 0.08f;

	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float StackDuration = 6.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mark")
	float MarkBaseDamage = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mark")
	float MarkDamagePerLevel = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mark")
	float MarkAbilityPowerRatio = 0.6f;

	int32 Stacks = 0;
	double ExpireTime = 0.0;
};

/**
 * 진진자라: 탄창 3발. 다 쏘면 2초 동안 재장전하고, 세 번째 발은 공격력 100%만큼 더 강하다.
 * 공격 속도는 기본값에 고정되고, 늘어난 공격 속도 1%당 공격력 0.4를 얻는다. 기본 공격에 맞은 적 챔피언에게 표식.
 */
UCLASS()
class VER1_API UMobaPassive_Magazine : public UMobaPassive
{
	GENERATED_BODY()

public:
	UMobaPassive_Magazine();

	virtual FMobaAttackBonus OnBasicAttack(AActor* Target) override;
	virtual bool CanStartBasicAttack() const override { return Ammo > 0; }
	virtual float ModifyAttackSpeed(float AttackSpeed) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnRespawned() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	int32 MagazineSize = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float ReloadTime = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float LastShotAttackDamageRatio = 1.0f;

	/** 늘어난 공격 속도 1%당 얻는 공격력. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float AttackDamagePerAttackSpeedPercent = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float MarkDuration = 4.0f;

	float GetBaseAttackSpeed() const;

	int32 Ammo = 3;
	double ReloadEndTime = 0.0;
	float AppliedConversion = 0.0f;
};
