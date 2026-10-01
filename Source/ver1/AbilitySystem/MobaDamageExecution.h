#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayEffectExecutionCalculation.h"
#include "MobaDamageExecution.generated.h"

/**
 * SetByCaller로 받은 물리·마법·고정 피해에 대상의 방어력과 마법 저항력을 적용해
 * 최종 피해량을 IncomingDamage로 내보낸다.
 */
UCLASS()
class VER1_API UMobaDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UMobaDamageExecution();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

	/** 저항 수치에 따른 피해 배율. 100이면 절반, 음수면 피해가 늘어난다. */
	static float GetResistanceMultiplier(float Resistance);
};

/** 모든 피해에 쓰는 즉시 적용 이펙트. 피해량은 MobaTags::Data_Damage_* 로 넘긴다. */
UCLASS()
class VER1_API UMobaDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UMobaDamageEffect();
};
