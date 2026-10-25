#include "Character/MobaPassive.h"
#include "AbilitySystem/MobaAbilityLibrary.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaCharacter.h"

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
// 레인저

UMobaPassive_Volley::UMobaPassive_Volley()
{
	DisplayName = NSLOCTEXT("Moba", "VolleyName", "연발 사격");
	Description = NSLOCTEXT("Moba", "VolleyDesc", "기본 공격 세 번째마다 공격력의 40%만큼 추가 물리 피해를 줍니다.");
	MaxStacks = 3;
}

FMobaAttackBonus UMobaPassive_Volley::OnBasicAttack(AActor* Target)
{
	FMobaAttackBonus Bonus;

	AttackCount = (AttackCount + 1) % AttacksPerProc;
	if (AttackCount == 0)
	{
		Bonus.BonusPhysicalDamage = GetAttribute(Owner, UMobaAttributeSet::GetAttackDamageAttribute()) * BonusAttackDamageRatio;
	}

	// 다음 강화 공격까지 몇 번 남았는지가 아니라 지금까지 쌓은 수를 보여 준다(세 번째에 0으로 돌아간다).
	SetStacks(AttackCount);
	return Bonus;
}

// ---------------------------------------------------------------------------------------------------------------------
// 뱅가드

UMobaPassive_Resolve::UMobaPassive_Resolve()
{
	DisplayName = NSLOCTEXT("Moba", "ResolveName", "불굴");
	Description = NSLOCTEXT("Moba", "ResolveDesc", "5초 동안 피해를 받지 않으면 1초마다 최대 체력의 1.5%를 회복합니다.");
}

void UMobaPassive_Resolve::OnDamageTaken(float Damage)
{
	TimeSinceDamage = 0.0f;
	SetStacks(0);
}

void UMobaPassive_Resolve::OnRespawned()
{
	TimeSinceDamage = OutOfCombatDelay;
}

void UMobaPassive_Resolve::Tick(float DeltaSeconds)
{
	TimeSinceDamage += DeltaSeconds;
	const bool bActive = TimeSinceDamage >= OutOfCombatDelay;
	SetStacks(bActive ? 1 : 0);

	UAbilitySystemComponent* AbilitySystem = Owner ? Owner->GetAbilitySystemComponent() : nullptr;
	if (!bActive || !AbilitySystem)
	{
		return;
	}

	const float Health = AbilitySystem->GetNumericAttributeBase(UMobaAttributeSet::GetHealthAttribute());
	const float MaxHealth = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxHealthAttribute());
	if (Health < MaxHealth)
	{
		AbilitySystem->SetNumericAttributeBase(UMobaAttributeSet::GetHealthAttribute(), FMath::Min(Health + MaxHealth * HealFractionPerSecond * DeltaSeconds, MaxHealth));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// 미스틱

UMobaPassive_Charge::UMobaPassive_Charge()
{
	DisplayName = NSLOCTEXT("Moba", "ChargeName", "마력 충전");
	Description = NSLOCTEXT("Moba", "ChargeDesc", "스킬을 쓰면 다음 기본 공격이 마법 피해 30(+주문력의 40%)을 더하고 마나 15를 회복합니다.");
}

void UMobaPassive_Charge::OnAbilityCast(EMobaAbilitySlot Slot)
{
	bCharged = true;
	SetStacks(1);
}

void UMobaPassive_Charge::OnRespawned()
{
	bCharged = false;
	SetStacks(0);
}

FMobaAttackBonus UMobaPassive_Charge::OnBasicAttack(AActor* Target)
{
	FMobaAttackBonus Bonus;
	if (bCharged)
	{
		bCharged = false;
		SetStacks(0);
		Bonus.BonusMagicDamage = BonusMagicDamage + GetAttribute(Owner, UMobaAttributeSet::GetAbilityPowerAttribute()) * AbilityPowerRatio;
		Bonus.ManaRestore = ManaRestore;
	}
	return Bonus;
}

// ---------------------------------------------------------------------------------------------------------------------
// 섀도우

UMobaPassive_Ambush::UMobaPassive_Ambush()
{
	DisplayName = NSLOCTEXT("Moba", "AmbushName", "암습");
	Description = NSLOCTEXT("Moba", "AmbushDesc", "그림자 숨기(W)를 쓴 뒤 첫 기본 공격이 공격력의 50%만큼 추가 물리 피해를 줍니다.");
}

void UMobaPassive_Ambush::OnAbilityCast(EMobaAbilitySlot Slot)
{
	if (Slot == EMobaAbilitySlot::W)
	{
		bReady = true;
		SetStacks(1);
	}
}

void UMobaPassive_Ambush::OnRespawned()
{
	bReady = false;
	SetStacks(0);
}

FMobaAttackBonus UMobaPassive_Ambush::OnBasicAttack(AActor* Target)
{
	FMobaAttackBonus Bonus;
	if (bReady)
	{
		bReady = false;
		SetStacks(0);
		Bonus.BonusPhysicalDamage = GetAttribute(Owner, UMobaAttributeSet::GetAttackDamageAttribute()) * BonusAttackDamageRatio;
	}
	return Bonus;
}

// ---------------------------------------------------------------------------------------------------------------------
// 세이지

UMobaPassive_Grace::UMobaPassive_Grace()
{
	DisplayName = NSLOCTEXT("Moba", "GraceName", "은총");
	Description = NSLOCTEXT("Moba", "GraceDesc", "스킬을 쓰면 주변에서 체력 비율이 가장 낮은 아군 챔피언(자신 포함)이 체력 20(+주문력 15%)을 회복합니다.");
}

void UMobaPassive_Grace::OnAbilityCast(EMobaAbilitySlot Slot)
{
	if (!Owner)
	{
		return;
	}

	TArray<AMobaCharacter*> Allies;
	MobaAbility::FindAlliedChampionsInRadius(Owner, Owner->GetActorLocation(), Radius, Allies);

	AMobaCharacter* Lowest = nullptr;
	float LowestFraction = 2.0f;
	for (AMobaCharacter* Ally : Allies)
	{
		const float MaxHealth = GetAttribute(Ally, UMobaAttributeSet::GetMaxHealthAttribute());
		const float Fraction = MaxHealth > 0.0f ? GetAttribute(Ally, UMobaAttributeSet::GetHealthAttribute()) / MaxHealth : 1.0f;
		if (Fraction < LowestFraction)
		{
			Lowest = Ally;
			LowestFraction = Fraction;
		}
	}

	if (Lowest)
	{
		MobaAbility::HealTarget(Lowest, BaseHeal + GetAttribute(Owner, UMobaAttributeSet::GetAbilityPowerAttribute()) * AbilityPowerRatio);
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
