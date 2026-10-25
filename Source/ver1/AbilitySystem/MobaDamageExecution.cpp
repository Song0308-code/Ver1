#include "AbilitySystem/MobaDamageExecution.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "Game/MobaGameplayTags.h"

namespace
{
	struct FMobaDamageStatics
	{
		DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);
		DECLARE_ATTRIBUTE_CAPTUREDEF(MagicResist);
		DECLARE_ATTRIBUTE_CAPTUREDEF(ArmorPenetration);
		DECLARE_ATTRIBUTE_CAPTUREDEF(MagicPenetration);

		FMobaDamageStatics()
		{
			DEFINE_ATTRIBUTE_CAPTUREDEF(UMobaAttributeSet, Armor, Target, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UMobaAttributeSet, MagicResist, Target, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UMobaAttributeSet, ArmorPenetration, Source, true);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UMobaAttributeSet, MagicPenetration, Source, true);
		}
	};

	const FMobaDamageStatics& DamageStatics()
	{
		static FMobaDamageStatics Statics;
		return Statics;
	}
}

UMobaDamageExecution::UMobaDamageExecution()
{
	RelevantAttributesToCapture.Add(DamageStatics().ArmorDef);
	RelevantAttributesToCapture.Add(DamageStatics().MagicResistDef);
	RelevantAttributesToCapture.Add(DamageStatics().ArmorPenetrationDef);
	RelevantAttributesToCapture.Add(DamageStatics().MagicPenetrationDef);
}

float UMobaDamageExecution::GetEffectiveResistance(float Resistance, float Penetration)
{
	return Resistance > 0.0f ? FMath::Max(Resistance - FMath::Max(Penetration, 0.0f), 0.0f) : Resistance;
}

float UMobaDamageExecution::GetResistanceMultiplier(float Resistance)
{
	return Resistance >= 0.0f ? 100.0f / (100.0f + Resistance) : 2.0f - 100.0f / (100.0f - Resistance);
}

void UMobaDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	FAggregatorEvaluateParameters EvaluateParams;
	EvaluateParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluateParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float Armor = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorDef, EvaluateParams, Armor);

	float MagicResist = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MagicResistDef, EvaluateParams, MagicResist);

	float ArmorPenetration = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorPenetrationDef, EvaluateParams, ArmorPenetration);

	float MagicPenetration = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MagicPenetrationDef, EvaluateParams, MagicPenetration);

	Armor = GetEffectiveResistance(Armor, ArmorPenetration);
	MagicResist = GetEffectiveResistance(MagicResist, MagicPenetration);

	const float PhysicalDamage = FMath::Max(Spec.GetSetByCallerMagnitude(MobaTags::Data_Damage_Physical, false, 0.0f), 0.0f);
	const float MagicDamage = FMath::Max(Spec.GetSetByCallerMagnitude(MobaTags::Data_Damage_Magic, false, 0.0f), 0.0f);
	const float TrueDamage = FMath::Max(Spec.GetSetByCallerMagnitude(MobaTags::Data_Damage_True, false, 0.0f), 0.0f);

	const float TotalDamage = PhysicalDamage * GetResistanceMultiplier(Armor) + MagicDamage * GetResistanceMultiplier(MagicResist) + TrueDamage;
	if (TotalDamage > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UMobaAttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, TotalDamage));
	}
}

UMobaDamageEffect::UMobaDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition Execution;
	Execution.CalculationClass = UMobaDamageExecution::StaticClass();
	Executions.Add(Execution);
}
