#include "AbilitySystem/MobaAbilityLibrary.h"
#include "AbilitySystem/MobaDamageExecution.h"
#include "AbilitySystem/MobaGameplayEffects.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "Character/MobaCharacter.h"
#include "EngineUtils.h"
#include "Combat/MobaCombatLibrary.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Game/MobaGameplayTags.h"
#include "GameFramework/Actor.h"
#include "Vision/MobaVisionSubsystem.h"

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
	case EMobaAbilitySlot::D: return MobaTags::Ability_Slot_D;
	case EMobaAbilitySlot::F: return MobaTags::Ability_Slot_F;
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
	case EMobaAbilitySlot::D: return MobaTags::Cooldown_Slot_D;
	case EMobaAbilitySlot::F: return MobaTags::Cooldown_Slot_F;
	default: return MobaTags::Cooldown_Slot_R;
	}
}

bool MobaAbility::FindSlotInTags(const FGameplayTagContainer& Tags, EMobaAbilitySlot& OutSlot)
{
	for (const EMobaAbilitySlot Slot : { EMobaAbilitySlot::Q, EMobaAbilitySlot::W, EMobaAbilitySlot::E, EMobaAbilitySlot::R, EMobaAbilitySlot::D, EMobaAbilitySlot::F })
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

FGameplayEffectSpecHandle MobaAbility::MakeStatBuffSpec(UAbilitySystemComponent* Source, AActor* Instigator, float Duration, const FMobaStatBonus& Bonus, const FGameplayTagContainer& Tags)
{
	const FGameplayEffectSpecHandle Spec = MakeSpec(Source, Instigator, UMobaAttributeBuffEffect::StaticClass());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(MobaTags::Data_Duration, Duration);
		Spec.Data->SetSetByCallerMagnitude(MobaTags::Data_Bonus_AttackDamage, Bonus.AttackDamage);
		Spec.Data->SetSetByCallerMagnitude(MobaTags::Data_Bonus_AbilityPower, Bonus.AbilityPower);
		Spec.Data->SetSetByCallerMagnitude(MobaTags::Data_Bonus_Armor, Bonus.Armor);
		Spec.Data->SetSetByCallerMagnitude(MobaTags::Data_Bonus_MagicResist, Bonus.MagicResist);
		Spec.Data->SetSetByCallerMagnitude(MobaTags::Data_Bonus_MoveSpeed, Bonus.MoveSpeed);
		Spec.Data->DynamicGrantedTags.AppendTags(Tags);
	}
	return Spec;
}

FGameplayEffectSpecHandle MobaAbility::MakeBuffSpec(UAbilitySystemComponent* Source, AActor* Instigator, float Duration, float BonusAttackDamage, float BonusAbilityPower, const FGameplayTag& BuffTag)
{
	FMobaStatBonus Bonus;
	Bonus.AttackDamage = BonusAttackDamage;
	Bonus.AbilityPower = BonusAbilityPower;
	FGameplayTagContainer Tags;
	if (BuffTag.IsValid())
	{
		Tags.AddTag(BuffTag);
	}
	return MakeStatBuffSpec(Source, Instigator, Duration, Bonus, Tags);
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

FGameplayEffectSpecHandle MobaAbility::MakeCrowdControlSpec(UAbilitySystemComponent* Source, AActor* Instigator, float Duration, const FGameplayTag& StateTag)
{
	const FGameplayEffectSpecHandle Spec = MakeSpec(Source, Instigator, UMobaTimedTagEffect::StaticClass());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(MobaTags::Data_Duration, Duration);
		Spec.Data->DynamicGrantedTags.AddTag(StateTag);
		if (StateTag == MobaTags::State_Airborne)
		{
			Spec.Data->DynamicGrantedTags.AddTag(MobaTags::State_Stunned);
		}
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

void MobaAbility::FindEnemiesInRadius(const AActor* Source, const FVector& Center, float Radius, TArray<AActor*>& OutEnemies, bool bIncludeStructures)
{
	OutEnemies.Reset();
	if (!Source || !Source->GetWorld())
	{
		return;
	}

	// 챔피언과 미니언은 폰, 건물은 동적 오브젝트로 분류되어 있다.
	FCollisionObjectQueryParams ObjectParams(ECC_Pawn);
	if (bIncludeStructures)
	{
		ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	}

	TArray<FOverlapResult> Overlaps;
	Source->GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(Radius));

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (MobaCombat::IsValidEnemyTarget(Source, Actor))
		{
			OutEnemies.AddUnique(Actor);
		}
	}
}

void MobaAbility::FindAlliedChampionsInRadius(const AActor* Source, const FVector& Center, float Radius, TArray<AMobaCharacter*>& OutAllies)
{
	OutAllies.Reset();
	if (!Source || !Source->GetWorld())
	{
		return;
	}

	const EMobaTeam Team = MobaCombat::GetTeam(Source);
	for (TActorIterator<AMobaCharacter> It(Source->GetWorld()); It; ++It)
	{
		if (It->GetTeam() == Team && !It->IsDead() && FVector::DistSquared2D(It->GetActorLocation(), Center) <= FMath::Square(Radius))
		{
			OutAllies.Add(*It);
		}
	}
}

float MobaAbility::HealTarget(AActor* Target, float Amount)
{
	const IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(Target);
	UAbilitySystemComponent* AbilitySystem = AbilityInterface ? AbilityInterface->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystem || Amount <= 0.0f || MobaCombat::IsDead(Target))
	{
		return 0.0f;
	}

	const float Health = AbilitySystem->GetNumericAttributeBase(UMobaAttributeSet::GetHealthAttribute());
	const float MaxHealth = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxHealthAttribute());
	const float NewHealth = FMath::Min(Health + Amount, MaxHealth);
	AbilitySystem->SetNumericAttributeBase(UMobaAttributeSet::GetHealthAttribute(), NewHealth);
	return FMath::Max(NewHealth - Health, 0.0f);
}

AActor* MobaAbility::PickEnemyNearCursor(const AActor* Caster, const FVector& Cursor, float PickRadius, float Range, TFunctionRef<bool(const AActor*)> Filter)
{
	TArray<AActor*> Candidates;
	FindEnemiesInRadius(Caster, Cursor, PickRadius, Candidates);

	const UMobaVisionSubsystem* Vision = UMobaVisionSubsystem::Get(Caster);
	const EMobaTeam CasterTeam = MobaCombat::GetTeam(Caster);
	AActor* Best = nullptr;
	float BestDistance = TNumericLimits<float>::Max();

	for (AActor* Candidate : Candidates)
	{
		if (!Filter(Candidate) || MobaCombat::GetDistanceToTargetEdge(Caster, Candidate) > Range)
		{
			continue;
		}
		if ((Vision && Vision->IsHiddenFromTeam(Candidate, CasterTeam)) || MobaCombat::IsStealthedFrom(Candidate, CasterTeam))
		{
			continue;
		}

		const float Distance = FVector::DistSquared2D(Candidate->GetActorLocation(), Cursor);
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Candidate;
		}
	}
	return Best;
}
