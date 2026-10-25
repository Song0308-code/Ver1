#include "Character/MobaPassive.h"
#include "AbilitySystem/MobaAbilityLibrary.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaCharacter.h"
#include "Player/MobaPlayerState.h"
#include "Game/MobaGameplayTags.h"

namespace
{
	float GetAttribute(const AMobaCharacter* Owner, const FGameplayAttribute& Attribute)
	{
		const UAbilitySystemComponent* AbilitySystem = Owner ? Owner->GetAbilitySystemComponent() : nullptr;
		return AbilitySystem ? AbilitySystem->GetNumericAttribute(Attribute) : 0.0f;
	}
}

void UMobaPassive::SetStacks(int32 Stacks) const
{
	if (Owner)
	{
		Owner->SetPassiveStacks(Stacks);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// 깡통

UMobaPassive_ManaBarrier::UMobaPassive_ManaBarrier()
{
	DisplayName = NSLOCTEXT("Moba", "ManaBarrierName", "마나 방벽");
	Description = NSLOCTEXT("Moba", "ManaBarrierDesc", "체력이 30% 아래로 떨어지면 현재 마나의 40%를 써서 그만큼 6초 동안 보호막을 얻습니다. 재사용 대기 90초.");
}

void UMobaPassive_ManaBarrier::OnDamageTaken(float Damage)
{
	UAbilitySystemComponent* AbilitySystem = Owner ? Owner->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystem || Owner->IsDead() || Owner->GetWorld()->GetTimeSeconds() < ReadyTime)
	{
		return;
	}

	const float MaxHealth = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxHealthAttribute());
	const float Health = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetHealthAttribute());
	if (MaxHealth <= 0.0f || Health / MaxHealth >= HealthThreshold)
	{
		return;
	}

	const float Mana = AbilitySystem->GetNumericAttributeBase(UMobaAttributeSet::GetManaAttribute());
	const float Spent = Mana * ManaFraction;
	if (Spent <= 0.0f)
	{
		return;
	}

	AbilitySystem->SetNumericAttributeBase(UMobaAttributeSet::GetManaAttribute(), Mana - Spent);
	Owner->AddShield(Spent, ShieldDuration);
	ReadyTime = Owner->GetWorld()->GetTimeSeconds() + Cooldown;
	SetStacks(0);
}

void UMobaPassive_ManaBarrier::Tick(float DeltaSeconds)
{
	SetStacks(Owner && Owner->GetWorld()->GetTimeSeconds() >= ReadyTime ? 1 : 0);
}

void UMobaPassive_ManaBarrier::OnRespawned()
{
	Tick(0.0f);
}

// ---------------------------------------------------------------------------------------------------------------------
// 젖소

UMobaPassive_Rumination::UMobaPassive_Rumination()
{
	DisplayName = NSLOCTEXT("Moba", "RuminationName", "되새김");
	Description = NSLOCTEXT("Moba", "RuminationDesc", "스킬로 적 챔피언을 띄우거나 밀치면 주변 아군 챔피언(자신 포함)이 체력 20(+젖소 최대 체력의 2%)을 회복합니다. 2초에 한 번.");
}

void UMobaPassive_Rumination::OnCrowdControlApplied(AActor* Target)
{
	if (!Owner || Owner->IsDead() || Owner->GetWorld()->GetTimeSeconds() < ReadyTime)
	{
		return;
	}
	ReadyTime = Owner->GetWorld()->GetTimeSeconds() + InternalCooldown;

	const float Heal = BaseHeal + GetAttribute(Owner, UMobaAttributeSet::GetMaxHealthAttribute()) * MaxHealthRatio;
	TArray<AMobaCharacter*> Allies;
	MobaAbility::FindAlliedChampionsInRadius(Owner, Owner->GetActorLocation(), Radius, Allies);
	for (AMobaCharacter* Ally : Allies)
	{
		MobaAbility::HealTarget(Ally, Heal);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// 노머고

UMobaPassive_Overclock::UMobaPassive_Overclock()
{
	DisplayName = NSLOCTEXT("Moba", "OverclockName", "가속 회로");
	Description = NSLOCTEXT("Moba", "OverclockDesc", "스킬을 맞힐 때마다 6초 동안 공격 속도가 8% 오릅니다(최대 5중첩). W의 표식이 붙은 적을 기본 공격이나 다른 스킬로 맞히면 표식이 터져 마법 피해 60(+챔피언 레벨당 10, 주문력 60%)을 줍니다.");
	MaxStacks = 5;
}

float UMobaPassive_Overclock::GetMarkDamage() const
{
	const AMobaPlayerState* PlayerState = Owner ? Owner->GetPlayerState<AMobaPlayerState>() : nullptr;
	const int32 Level = PlayerState ? PlayerState->GetChampionLevel() : 1;
	return MarkBaseDamage + MarkDamagePerLevel * (Level - 1) + GetAttribute(Owner, UMobaAttributeSet::GetAbilityPowerAttribute()) * MarkAbilityPowerRatio;
}

FMobaAttackBonus UMobaPassive_Overclock::OnBasicAttack(AActor* Target)
{
	FMobaAttackBonus Bonus;
	if (MobaAbility::ConsumeMark(Target))
	{
		Bonus.BonusMagicDamage = GetMarkDamage();
	}
	return Bonus;
}

void UMobaPassive_Overclock::OnAbilityHit(AActor* Target)
{
	UAbilitySystemComponent* AbilitySystem = Owner ? Owner->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystem)
	{
		return;
	}

	if (MobaAbility::ConsumeMark(Target))
	{
		MobaAbility::ApplySpecToTarget(MobaAbility::MakeDamageSpec(AbilitySystem, Owner, 0.0f, GetMarkDamage(), 0.0f), Target);
	}

	// 중첩은 효과 하나로 유지한다: 이전 것을 지우고 새 배율로 다시 건다.
	Stacks = FMath::Min(Stacks + 1, MaxStacks);
	ExpireTime = Owner->GetWorld()->GetTimeSeconds() + StackDuration;
	AbilitySystem->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(MobaTags::Buff_Overclock));
	MobaAbility::ApplySpecToTarget(MobaAbility::MakeStatModSpec(AbilitySystem, Owner, StackDuration, 1.0f, 1.0f + AttackSpeedPerStack * Stacks, MobaTags::Buff_Overclock), Owner);
	SetStacks(Stacks);
}

void UMobaPassive_Overclock::Tick(float DeltaSeconds)
{
	if (Stacks > 0 && Owner && Owner->GetWorld()->GetTimeSeconds() > ExpireTime)
	{
		Stacks = 0;
		SetStacks(0);
	}
}

void UMobaPassive_Overclock::OnRespawned()
{
	Stacks = 0;
	SetStacks(0);
}

// ---------------------------------------------------------------------------------------------------------------------
// 진진자라

UMobaPassive_Magazine::UMobaPassive_Magazine()
{
	DisplayName = NSLOCTEXT("Moba", "MagazineName", "네 박자");
	Description = NSLOCTEXT("Moba", "MagazineDesc", "탄창이 3발입니다. 다 쏘면 2초 동안 재장전하고, 세 번째 발은 공격력의 100%만큼 추가 피해를 줍니다. 공격 속도는 오르지 않는 대신 늘어난 공격 속도 1%당 공격력 0.4를 얻습니다. 기본 공격에 맞은 적 챔피언에게 4초 동안 표식을 남깁니다.");
	MaxStacks = 3;
}

float UMobaPassive_Magazine::GetBaseAttackSpeed() const
{
	return Owner ? Owner->GetBaseStats().AttackSpeed : 0.0f;
}

float UMobaPassive_Magazine::ModifyAttackSpeed(float AttackSpeed) const
{
	// 둔화는 그대로 받고, 늘어난 몫은 쓰지 않는다.
	const float Base = GetBaseAttackSpeed();
	return Base > 0.0f ? FMath::Min(AttackSpeed, Base) : AttackSpeed;
}

FMobaAttackBonus UMobaPassive_Magazine::OnBasicAttack(AActor* Target)
{
	FMobaAttackBonus Bonus;
	UAbilitySystemComponent* AbilitySystem = Owner ? Owner->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystem || Ammo <= 0)
	{
		return Bonus;
	}

	--Ammo;
	if (Ammo == 0)
	{
		Bonus.BonusPhysicalDamage = GetAttribute(Owner, UMobaAttributeSet::GetAttackDamageAttribute()) * LastShotAttackDamageRatio;
		ReloadEndTime = Owner->GetWorld()->GetTimeSeconds() + ReloadTime;
	}
	SetStacks(Ammo);

	if (Target && Target->IsA<AMobaCharacter>())
	{
		MobaAbility::ApplySpecToTarget(MobaAbility::MakeCrowdControlSpec(AbilitySystem, Owner, MarkDuration, MobaTags::State_Marked), Target);
	}
	return Bonus;
}

void UMobaPassive_Magazine::Tick(float DeltaSeconds)
{
	UAbilitySystemComponent* AbilitySystem = Owner ? Owner->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystem)
	{
		return;
	}

	if (Ammo <= 0 && Owner->GetWorld()->GetTimeSeconds() >= ReloadEndTime)
	{
		Ammo = MagazineSize;
		SetStacks(Ammo);
	}

	// 늘어난 공격 속도를 공격력으로 바꾼다. 값이 바뀌었을 때만 효과를 갈아 끼운다.
	const float Base = GetBaseAttackSpeed();
	const float Current = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetAttackSpeedAttribute());
	const float BonusPercent = Base > 0.0f ? FMath::Max(Current / Base - 1.0f, 0.0f) * 100.0f : 0.0f;
	const float Conversion = FMath::RoundToFloat(BonusPercent * AttackDamagePerAttackSpeedPercent);
	if (!FMath::IsNearlyEqual(Conversion, AppliedConversion))
	{
		AbilitySystem->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(MobaTags::Buff_Conversion));
		if (Conversion > 0.0f)
		{
			FMobaStatBonus StatBonus;
			StatBonus.AttackDamage = Conversion;
			MobaAbility::ApplySpecToTarget(MobaAbility::MakeStatBuffSpec(AbilitySystem, Owner, 36000.0f, StatBonus, FGameplayTagContainer(MobaTags::Buff_Conversion)), Owner);
		}
		AppliedConversion = Conversion;
	}
}

void UMobaPassive_Magazine::OnRespawned()
{
	Ammo = MagazineSize;
	SetStacks(Ammo);
}
