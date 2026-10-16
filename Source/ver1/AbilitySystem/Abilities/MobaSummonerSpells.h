#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/MobaAbilityArchetypes.h"
#include "MobaSummonerSpells.generated.h"

namespace MobaSummoner
{
	/** 주문에 해당하는 스킬 클래스. */
	VER1_API TSubclassOf<UMobaGameplayAbility> GetAbilityClass(EMobaSummonerSpell Spell);

	/** 화면에 보여 줄 주문 이름. */
	VER1_API FText GetDisplayName(EMobaSummonerSpell Spell);
}

/** 점멸: 짧은 거리를 즉시 순간이동한다. */
UCLASS()
class VER1_API UMobaSummoner_Flash : public UMobaAbility_Blink
{
	GENERATED_BODY()

public:
	UMobaSummoner_Flash();
};

/** 유체화: 한동안 이동 속도가 크게 오른다. */
UCLASS()
class VER1_API UMobaSummoner_Ghost : public UMobaAbility_SelfBuff
{
	GENERATED_BODY()

public:
	UMobaSummoner_Ghost();
};

/** 회복: 챔피언 레벨에 비례해 체력을 회복하고 잠깐 빨라진다. */
UCLASS()
class VER1_API UMobaSummoner_Heal : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaSummoner_Heal();

protected:
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	float BaseHeal = 80.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	float HealPerChampionLevel = 15.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	float MoveSpeedMultiplier = 1.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	float HasteDuration = 1.0f;
};

/**
 * 대상을 고르는 소환사 주문의 기반. 커서 근처(TargetPickRadius)의 알맞은 적 하나를 고르고,
 * 사거리 안에 보이는 대상이 없으면 아무것도 소모하지 않고 취소된다.
 */
UCLASS(Abstract)
class VER1_API UMobaSummoner_Targeted : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaSummoner_Targeted();

	virtual float GetIndicatorRadius() const override { return TargetPickRadius; }

protected:
	virtual bool ResolveTarget(const AMobaCharacter* Caster, FVector& InOutTargetLocation) const override;

	/** 이 주문을 쓸 수 있는 대상인지. 적이고 살아 있는지는 이미 확인한 뒤에 부른다. */
	virtual bool IsAllowedTarget(const AActor* Candidate) const { return false; }

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	float TargetPickRadius = 300.0f;

	/** ResolveTarget이 고른 대상. 스킬 인스턴스가 챔피언마다 하나라서 시전 사이에 들고 있을 수 있다. */
	mutable TWeakObjectPtr<AActor> PendingTarget;
};

/** 점화: 적 챔피언을 5초 동안 불태워 고정 피해를 준다. */
UCLASS()
class VER1_API UMobaSummoner_Ignite : public UMobaSummoner_Targeted
{
	GENERATED_BODY()

public:
	UMobaSummoner_Ignite();

protected:
	virtual bool IsAllowedTarget(const AActor* Candidate) const override;
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	void ApplyBurnTick();

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	float BaseTotalDamage = 70.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	float TotalDamagePerChampionLevel = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	int32 BurnTicks = 5;

	TWeakObjectPtr<AActor> BurnTarget;
	TWeakObjectPtr<AMobaCharacter> BurnSource;
	float DamagePerTick = 0.0f;
	int32 RemainingTicks = 0;
	FTimerHandle BurnTimer;
};

/** 강타: 몬스터나 적 미니언에게 큰 고정 피해. 정글 오브젝트 막타용. */
UCLASS()
class VER1_API UMobaSummoner_Smite : public UMobaSummoner_Targeted
{
	GENERATED_BODY()

public:
	UMobaSummoner_Smite();

protected:
	virtual bool IsAllowedTarget(const AActor* Candidate) const override;
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation) override;

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	float Damage = 600.0f;
};
