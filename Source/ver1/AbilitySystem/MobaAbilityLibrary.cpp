#include "AbilitySystem/MobaAbilityLibrary.h"
#include "AbilitySystem/MobaDamageExecution.h"
#include "AbilitySystem/MobaGameplayEffects.h"
#include "AbilitySystemComponent.h"
#include "Combat/MobaCombatLibrary.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Game/MobaGameplayTags.h"
#include "GameFramework/Actor.h"

namespace
{
	FGameplayEffectSpecHandle MakeSpec(UAbilitySystemComponent* Source, AActor* Instigator, TSubclassOf<UGameplayEffect> EffectClass)
	{
		if (!Source)
		{
			return FGameplayEffectSpecHandle();
		}

		FGameplayEffectContextHandle Context = Source->MakeEffectContext();
		Context.AddInstigator(Instigator, Instigator);
		return Source->MakeOutgoingSpec(EffectClass, 1.0f, Context);
	}
}

FGameplayTag MobaAbility::GetSlotTag(EMobaAbilitySlot Slot)
{
	switch (Slot)
	{
	case EMobaAbilitySlot::Q: return MobaTags::Ability_Slot_Q;
	case EMobaAbilitySlot::W: return MobaTags::Ability_Slot_W;
	case EMobaAbilitySlot::E: return MobaTags::Ability_Slot_E;
	default: return MobaTags::Ability_Slot_R;
	}
}

FGameplayTag MobaAbility::GetCooldownTag(EMobaAbilitySlot Slot)
{
	switch (Slot)
	{
	case EMobaAbilitySlot::Q: return MobaTags::Cooldown_Slot_Q;
	case EMobaAbilitySlot::W: return MobaTags::Cooldown_Slot_W;
	case EMobaAbilitySlot::E: return MobaTags::Cooldown_Slot_E;
	default: return MobaTags::Cooldown_Slot_R;
	}
}

bool MobaAbility::FindSlotInTags(const FGameplayTagContainer& Tags, EMobaAbilitySlot& OutSlot)
{
	for (const EMobaAbilitySlot Slot : { EMobaAbilitySlot::Q, EMobaAbilitySlot::W, EMobaAbilitySlot::E, EMobaAbilitySlot::R })
	{
		if (Tags.HasTagExact(GetSlotTag(Slot)))
		{
			OutSlot = Slot;
			return true;
		}
	}
	return false;
}

FGameplayEffectSpecHandle MobaAbility::MakeDamageSpec(UAbilitySystemComponent* Source, AActor* Instigator, float PhysicalDamage, float MagicDamage, float TrueDamage)
{
	const FGameplayEffectSpecHandle Spec = MakeSpec(Source, Instigator, UMobaDamageEffect::StaticClass());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(MobaTags::Data_Damage_Physical, PhysicalDamage);
		Spec.Data->SetSetByCallerMagnitude(MobaTags::Data_Damage_Magic, MagicDamage);
		Spec.Data->SetSetByCallerMagnitude(MobaTags::Data_Damage_True, TrueDamage);
	}
	return Spec;
}

FGameplayEffectSpecHandle MobaAbility::MakeStatModSpec(UAbilitySystemComponent* Source, AActor* Instigator, float Duration, float MoveSpeedMultiplier, float AttackSpeedMultiplier, const FGameplayTag& StateTag)
{
	const FGameplayEffectSpecHandle Spec = MakeSpec(Source, Instigator, UMobaStatModEffect::StaticClass());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(MobaTags::Data_Duration, Duration);
		Spec.Data->SetSetByCallerMagnitude(MobaTags::Data_Multiplier_MoveSpeed, MoveSpeedMultiplier);
		Spec.Data->SetSetByCallerMagnitude(MobaTags::Data_Multiplier_AttackSpeed, AttackSpeedMultiplier);
		if (StateTag.IsValid())
		{
			Spec.Data->DynamicGrantedTags.AddTag(StateTag);
		}
	}
	return Spec;
}

FGameplayEffectSpecHandle MobaAbility::MakeStunSpec(UAbilitySystemComponent* Source, AActor* Instigator, float Duration)
{
	const FGameplayEffectSpecHandle Spec = MakeSpec(Source, Instigator, UMobaTimedTagEffect::StaticClass());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(MobaTags::Data_Duration, Duration);
		Spec.Data->DynamicGrantedTags.AddTag(MobaTags::State_Stunned);
	}
	return Spec;
}

void MobaAbility::ApplySpecToTarget(const FGameplayEffectSpecHandle& Spec, AActor* Target)
{
	UAbilitySystemComponent* TargetAbilitySystem = MobaCombat::GetAbilitySystem(Target);
	if (Spec.IsValid() && TargetAbilitySystem)
	{
		TargetAbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void MobaAbility::FindEnemiesInRadius(const AActor* Source, const FVector& Center, float Radius, TArray<AActor*>& OutEnemies)
{
	OutEnemies.Reset();
	if (!Source || !Source->GetWorld())
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	Source->GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Radius));

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (MobaCombat::IsValidEnemyTarget(Source, Actor))
		{
			OutEnemies.AddUnique(Actor);
		}
	}
}
