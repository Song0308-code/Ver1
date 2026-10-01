#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Game/MobaTypes.h"
#include "MobaGameplayAbility.generated.h"

class AMobaCharacter;

/**
 * 챔피언 스킬의 기반 클래스. 서버에서만 실행되며, 클라이언트는 캐릭터의 ServerCastAbility로 시전을 요청한다.
 * 마나 소모와 재사용 대기는 범용 이펙트로 처리하므로 스킬마다 이펙트 에셋을 만들 필요가 없다.
 */
UCLASS(Abstract)
class VER1_API UMobaGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaGameplayAbility();

	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	const FText& GetDisplayName() const { return DisplayName; }
	EMobaTargetingMode GetTargetingMode() const { return TargetingMode; }
	float GetCastRange() const { return CastRange; }
	float GetManaCost() const { return ManaCost; }
	float GetCooldownDuration() const { return CooldownDuration; }

	/** 조준 표시에 쓰는 크기. 방향 스킬은 폭의 절반, 위치 스킬은 범위 반지름. */
	virtual float GetIndicatorRadius() const { return 0.0f; }

protected:
	/**
	 * 마나와 쿨다운이 소모되기 전에 대상 위치를 확정한다. 사거리 제한 등을 여기서 적용한다.
	 * false를 돌려주면 시전이 취소되고 아무것도 소모되지 않는다.
	 */
	virtual bool ResolveTarget(const AMobaCharacter* Caster, FVector& InOutTargetLocation) const;

	/** 실제 효과. 끝나면 반드시 FinishCast를 호출해야 한다. */
	virtual void ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation);

	void FinishCast();

	/** 스킬 레벨에 따른 값. Base + PerLevel * (레벨 - 1). */
	float ScaleByLevel(float Base, float PerLevel) const;

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	EMobaTargetingMode TargetingMode = EMobaTargetingMode::None;

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	float CastRange = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	float ManaCost = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	float CooldownDuration = 0.0f;

private:
	/** GetCooldownTags가 돌려줄 태그를 담아 두는 곳. */
	mutable FGameplayTagContainer CooldownTagCache;
};
