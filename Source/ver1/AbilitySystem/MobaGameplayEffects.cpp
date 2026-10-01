#include "AbilitySystem/MobaGameplayEffects.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "Game/MobaGameplayTags.h"

namespace
{
	FGameplayEffectModifierMagnitude MakeSetByCaller(const FGameplayTag& DataTag)
	{
		FSetByCallerFloat SetByCaller;
		SetByCaller.DataTag = DataTag;
		return FGameplayEffectModifierMagnitude(SetByCaller);
	}

	FGameplayModifierInfo MakeModifier(const FGameplayAttribute& Attribute, EGameplayModOp::Type Op, const FGameplayTag& DataTag)
	{
		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = Op;
		Modifier.ModifierMagnitude = MakeSetByCaller(DataTag);
		return Modifier;
	}
}

UMobaCooldownEffect::UMobaCooldownEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = MakeSetByCaller(MobaTags::Data_Cooldown);
}

UMobaManaCostEffect::UMobaManaCostEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeModifier(UMobaAttributeSet::GetManaAttribute(), EGameplayModOp::Additive, MobaTags::Data_Cost_Mana));
}

UMobaStatModEffect::UMobaStatModEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = MakeSetByCaller(MobaTags::Data_Duration);
	Modifiers.Add(MakeModifier(UMobaAttributeSet::GetMoveSpeedAttribute(), EGameplayModOp::MultiplyCompound, MobaTags::Data_Multiplier_MoveSpeed));
	Modifiers.Add(MakeModifier(UMobaAttributeSet::GetAttackSpeedAttribute(), EGameplayModOp::MultiplyCompound, MobaTags::Data_Multiplier_AttackSpeed));
}

UMobaTimedTagEffect::UMobaTimedTagEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = MakeSetByCaller(MobaTags::Data_Duration);
}
