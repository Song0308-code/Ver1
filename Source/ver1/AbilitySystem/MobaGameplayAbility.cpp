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
	const UAbilitySystemComponent* AbilitySystem = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	return GetAbilityLevel(Handle, ActorInfo) >= 1 && !IsBlockedByCrowdControl(AbilitySystem)
		&& Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
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
		const UAbilitySystemComponent* AbilitySystem = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

		Spec.Data->DynamicGrantedTags.AppendTags(*CooldownTags);
		Spec.Data->SetSetByCallerMagnitude(MobaTags::Data_Cooldown, GetEffectiveCooldown(AbilitySystem));
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
	// 대상을 지정하지 않는 스킬은 클라이언트가 보낸 위치를 믿지 않고 서버가 아는 시전자 위치를 쓴다.
	if (TargetingMode != EMobaTargetingMode::None && TriggerEventData && TriggerEventData->TargetData.Num() > 0)
	{
		TargetLocation = UAbilitySystemBlueprintLibrary::GetTargetDataEndPoint(TriggerEventData->TargetData, 0);
	}

	if (!ResolveTarget(Caster, TargetLocation) || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 애니메이션·사운드가 반응할 수 있게 어느 칸의 스킬이 발동됐는지 알린다.
	EMobaAbilitySlot Slot;
	const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
	if (Spec && MobaAbility::FindSlotInTags(Spec->GetDynamicSpecSourceTags(), Slot))
	{
		Caster->NotifyAbilityCast(Slot);
	}

	ExecuteCast(Caster, TargetLocation);
}

bool UMobaGameplayAbility::IsBlockedByCrowdControl(const UAbilitySystemComponent* AbilitySystem) const
{
	if (!AbilitySystem || bUsableWhileCrowdControlled)
	{
		return false;
	}

	// 소환사 주문은 침묵에 막히지 않는다(점멸은 속박에는 막힌다).
	if (!bSummonerSpell && AbilitySystem->HasMatchingGameplayTag(MobaTags::State_Silenced))
	{
		return true;
	}
	return bMovementAbility && AbilitySystem->HasMatchingGameplayTag(MobaTags::State_Rooted);
}

float UMobaGameplayAbility::GetEffectiveCooldown(const UAbilitySystemComponent* AbilitySystem) const
{
	// 소환사 주문은 스킬 가속과 블루 버프의 영향을 받지 않는다.
	if (bSummonerSpell || !AbilitySystem)
	{
		return CooldownDuration;
	}

	// 블루 버프가 있으면 재사용 대기 시간이 10% 줄어든다.
	const float BlueScale = AbilitySystem->HasMatchingGameplayTag(MobaTags::Buff_Blue) ? 0.9f : 1.0f;
	return CooldownDuration * BlueScale * GetHasteMultiplier(AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetAbilityHasteAttribute()));
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
