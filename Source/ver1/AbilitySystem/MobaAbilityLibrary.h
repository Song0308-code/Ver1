#pragma once

#include "CoreMinimal.h"
#include "Game/MobaTypes.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"

class AActor;
class UAbilitySystemComponent;

/** 버프로 더하는 능력치. */
struct FMobaStatBonus
{
	float AttackDamage = 0.0f;
	float AbilityPower = 0.0f;
	float Armor = 0.0f;
	float MagicResist = 0.0f;
	float MoveSpeed = 0.0f;
};

namespace MobaAbility
{
	VER1_API FGameplayTag GetSlotTag(EMobaAbilitySlot Slot);
	VER1_API FGameplayTag GetCooldownTag(EMobaAbilitySlot Slot);

	/** 태그 묶음에서 스킬 칸 태그를 찾아 칸을 돌려준다. */
	VER1_API bool FindSlotInTags(const FGameplayTagContainer& Tags, EMobaAbilitySlot& OutSlot);

	/** Source가 주는 피해 스펙을 만든다. Instigator가 처치 기록에 남는다. */
	VER1_API FGameplayEffectSpecHandle MakeDamageSpec(UAbilitySystemComponent* Source, AActor* Instigator, float PhysicalDamage, float MagicDamage, float TrueDamage);

	/** 이동·공격 속도 배율 스펙을 만든다. StateTag는 State_Slowed, State_Hasted 등 표시용 태그. */
	VER1_API FGameplayEffectSpecHandle MakeStatModSpec(UAbilitySystemComponent* Source, AActor* Instigator, float Duration, float MoveSpeedMultiplier, float AttackSpeedMultiplier, const FGameplayTag& StateTag);

	/** 일정 시간 공격력·주문력을 더하고 BuffTag를 부여하는 스펙을 만든다. 수치 없이 태그만 주려면 보너스를 0으로 넘긴다. */
	/** 일정 시간 Bonus만큼 능력치를 더하고 Tags를 모두 부여하는 스펙. */
	VER1_API FGameplayEffectSpecHandle MakeStatBuffSpec(UAbilitySystemComponent* Source, AActor* Instigator, float Duration, const FMobaStatBonus& Bonus, const FGameplayTagContainer& Tags);

	VER1_API FGameplayEffectSpecHandle MakeBuffSpec(UAbilitySystemComponent* Source, AActor* Instigator, float Duration, float BonusAttackDamage, float BonusAbilityPower, const FGameplayTag& BuffTag);

	VER1_API FGameplayEffectSpecHandle MakeStunSpec(UAbilitySystemComponent* Source, AActor* Instigator, float Duration);

	/** Duration초 동안 상태 태그 하나를 붙이는 스펙(속박·침묵 등). 에어본은 기절 태그도 함께 붙인다. */
	VER1_API FGameplayEffectSpecHandle MakeCrowdControlSpec(UAbilitySystemComponent* Source, AActor* Instigator, float Duration, const FGameplayTag& StateTag);

	/** 서버 전용. 스펙이 유효하고 대상에게 능력 시스템이 있으면 적용한다. */
	VER1_API void ApplySpecToTarget(const FGameplayEffectSpecHandle& Spec, AActor* Target);

	/**
	 * 커서(Cursor) 근처 PickRadius 안의 적 중, Caster에게서 Range 안에 있고 Caster 팀에게 보이며 Filter를 통과하는 것 중
	 * 커서에 가장 가까운 대상을 고른다. 없으면 nullptr.
	 */
	VER1_API AActor* PickEnemyNearCursor(const AActor* Caster, const FVector& Cursor, float PickRadius, float Range, TFunctionRef<bool(const AActor*)> Filter);

	/** Source의 적 중 Center에서 Radius 안에 있는 살아 있는 대상을 모은다. 건물은 bIncludeStructures가 true일 때만 포함한다. */
	VER1_API void FindEnemiesInRadius(const AActor* Source, const FVector& Center, float Radius, TArray<AActor*>& OutEnemies, bool bIncludeStructures = false);
}
