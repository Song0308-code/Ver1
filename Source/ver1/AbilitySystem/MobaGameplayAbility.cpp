#include "AbilitySystem/MobaGameplayAbility.h"
#include "AbilitySystem/MobaAbilityLibrary.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystem/MobaGameplayEffects.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaCharacter.h"
#include "Game/MobaGameplayTags.h"

UMobaGameplayAbility::UMobaGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	ActivationBlockedTags.AddTag(MobaTags::State_Dead);
	ActivationBlockedTags.AddTag(MobaTags::State_Stunned);
}

bool UMobaGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 아직 한 번도 찍지 않은 스킬은 쓸 수 없다.
	return GetAbilityLevel(Handle, ActorInfo) >= 1 && Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

bool UMobaGameplayAbility::CanLevelUp(int32 CurrentLevel, int32 ChampionLevel) const
{
	if (CurrentLevel >= GetMaxLevel())
	{
		return false;
	}

	return !bUltimate || ChampionLevel >= 6 + 5 * CurrentLevel;
}

bool UMobaGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UAbilitySystemComponent* AbilitySystem = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	return AbilitySystem && AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetManaAttribute()) >= ManaCost;
}

void UMobaGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (ManaCost <= 0.0f)
	{
		return;
	}

	const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, UMobaManaCostEffect::StaticClass(), GetAbilityLevel(Handle, ActorInfo));
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(MobaTags::Data_Cost_Mana, -ManaCost);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	}
}

const FGameplayTagContainer* UMobaGameplayAbility::GetCooldownTags() const
{
	CooldownTagCache.Reset();

	// 쿨다운 태그는 이 스킬이 들어 있는 칸에 따라 정해지므로 인스턴스에서만 알 수 있다.
	if (IsInstantiated())
	{
		EMobaAbilitySlot Slot;
		const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
		if (Spec && MobaAbility::FindSlotInTags(Spec->GetDynamicSpecSourceTags(), Slot))
		{
			CooldownTagCache.AddTag(MobaAbility::GetCooldownTag(Slot));
		}
	}

	return &CooldownTagCache;
}

void UMobaGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const FGameplayTagContainer* CooldownTags = GetCooldownTags();
	if (CooldownDuration <= 0.0f || !CooldownTags || CooldownTags->IsEmpty())
	{
		return;
	}

	const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, UMobaCooldownEffect::StaticClass(), GetAbilityLevel(Handle, ActorInfo));
	if (Spec.IsValid())
	{
		Spec.Data->DynamicGrantedTags.AppendTags(*CooldownTags);
		Spec.Data->SetSetByCallerMagnitude(MobaTags::Data_Cooldown, CooldownDuration);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	}
}

void UMobaGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AMobaCharacter* Caster = ActorInfo ? Cast<AMobaCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Caster || !Caster->HasAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FVector TargetLocation = Caster->GetActorLocation();
	if (TriggerEventData && TriggerEventData->TargetData.Num() > 0)
	{
		TargetLocation = UAbilitySystemBlueprintLibrary::GetTargetDataEndPoint(TriggerEventData->TargetData, 0);
	}

	if (!ResolveTarget(Caster, TargetLocation) || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ExecuteCast(Caster, TargetLocation);
}

bool UMobaGameplayAbility::ResolveTarget(const AMobaCharacter* Caster, FVector& InOutTargetLocation) const
{
	// 위치 지정 스킬은 사거리를 넘는 지점을 사거리 끝으로 당긴다.
	if (TargetingMode == EMobaTargetingMode::Location && CastRange > 0.0f)
	{
		const FVector CasterLocation = Caster->GetActorLocation();
		FVector Offset = InOutTargetLocation - CasterLocation;
		Offset.Z = 0.0;
		InOutTargetLocation = CasterLocation + Offset.GetClampedToMaxSize(CastRange);
	}
	return true;
}

void UMobaGameplayAbility::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	FinishCast();
}

void UMobaGameplayAbility::FinishCast()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

float UMobaGameplayAbility::ScaleByLevel(float Base, float PerLevel) const
{
	return Base + PerLevel * FMath::Max(GetAbilityLevel() - 1, 0);
}
