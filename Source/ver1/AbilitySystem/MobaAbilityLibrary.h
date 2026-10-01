#pragma once

#include "CoreMinimal.h"
#include "Game/MobaTypes.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"

class AActor;
class UAbilitySystemComponent;

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

	VER1_API FGameplayEffectSpecHandle MakeStunSpec(UAbilitySystemComponent* Source, AActor* Instigator, float Duration);

	/** 서버 전용. 스펙이 유효하고 대상에게 능력 시스템이 있으면 적용한다. */
	VER1_API void ApplySpecToTarget(const FGameplayEffectSpecHandle& Spec, AActor* Target);

	/** Source의 적 중 Center에서 Radius 안에 있는 살아 있는 대상을 모은다. */
	VER1_API void FindEnemiesInRadius(const AActor* Source, const FVector& Center, float Radius, TArray<AActor*>& OutEnemies);
}
