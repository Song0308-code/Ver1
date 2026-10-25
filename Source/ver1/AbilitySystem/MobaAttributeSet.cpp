#include "AbilitySystem/MobaAttributeSet.h"
#include "Character/MobaCharacter.h"
#include "Combat/MobaCombatEvents.h"
#include "Game/MobaGameplayTags.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

void UMobaAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, AttackDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, AbilityPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, MagicResist, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, AttackSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, AttackRange, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, LifeSteal, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, CritChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, ArmorPenetration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, MagicPenetration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, BonusHealthRegen, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, BonusManaRegen, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, AbilityHaste, COND_None, REPNOTIFY_Always);
}

void UMobaAttributeSet::InitFromBaseStats(const FMobaBaseStats& Stats)
{
	InitMaxHealth(Stats.MaxHealth);
	InitHealth(Stats.MaxHealth);
	InitMaxMana(Stats.MaxMana);
	InitMana(Stats.MaxMana);
	InitAttackDamage(Stats.AttackDamage);
	InitAbilityPower(Stats.AbilityPower);
	InitArmor(Stats.Armor);
	InitMagicResist(Stats.MagicResist);
	InitAttackSpeed(Stats.AttackSpeed);
	InitMoveSpeed(Stats.MoveSpeed);
	InitAttackRange(Stats.AttackRange);
	InitLifeSteal(0.0f);
	InitCritChance(0.0f);
	InitArmorPenetration(0.0f);
	InitMagicPenetration(0.0f);
	InitBonusHealthRegen(0.0f);
	InitBonusManaRegen(0.0f);
	InitShield(0.0f);
	InitAbilityHaste(0.0f);

	OnMoveSpeedChanged.Broadcast(GetMoveSpeed());
}

void UMobaAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMana());
	}
	else if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UMobaAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMoveSpeedAttribute())
	{
		OnMoveSpeedChanged.Broadcast(NewValue);
	}
}

void UMobaAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		float Damage = GetIncomingDamage();

		// 와드는 기본 공격 한 번에 1씩만 깎이고 스킬 피해는 받지 않는다.
		if (GetOwningAbilitySystemComponentChecked()->HasMatchingGameplayTag(MobaTags::Unit_Ward))
		{
			Damage = Data.EffectSpec.GetDynamicAssetTags().HasTagExact(MobaTags::Damage_BasicAttack) && Damage > 0.0f ? 1.0f : 0.0f;
		}
		else if (GetOwningAbilitySystemComponentChecked()->HasMatchingGameplayTag(MobaTags::Buff_Unbreakable))
		{
			Damage *= UnbreakableDamageTaken;
		}
		SetIncomingDamage(0.0f);

		if (Damage > 0.0f && GetHealth() > 0.0f)
		{
			// 보호막이 먼저 받는다. 처치 기여·생명력 흡수 등은 흡수된 몫까지 포함한 전체 피해로 본다.
			const float Absorbed = FMath::Min(FMath::Max(GetShield(), 0.0f), Damage);
			if (Absorbed > 0.0f)
			{
				SetShield(GetShield() - Absorbed);
			}

			// 방어력 계산의 부동소수 오차로 체력이 0에 못 미치는 극소량 남는 것을 막는다.
			float NewHealth = FMath::Clamp(GetHealth() - (Damage - Absorbed), 0.0f, GetMaxHealth());
			if (NewHealth < 0.01f)
			{
				NewHealth = 0.0f;
			}

			// 장로 드래곤의 힘: 적 챔피언을 체력 20% 아래로 깎으면 바로 처치한다.
			const UAbilitySystemComponent* ExecuteSource = Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent();
			if (NewHealth > 0.0f && NewHealth < GetMaxHealth() * 0.2f && ExecuteSource && ExecuteSource->HasMatchingGameplayTag(MobaTags::Buff_Elder)
				&& Cast<AMobaCharacter>(GetOwningAbilitySystemComponentChecked()->GetAvatarActor()))
			{
				NewHealth = 0.0f;
			}
			SetHealth(NewHealth);

			AActor* DamageInstigator = Data.EffectSpec.GetContext().GetOriginalInstigator();
			if (UMobaCombatEvents* Events = UMobaCombatEvents::Get(GetOwningActor()))
			{
				Events->OnDamageDealt.Broadcast(DamageInstigator, GetOwningAbilitySystemComponentChecked()->GetAvatarActor(), Damage);
			}

			// 기본 공격이면 공격한 쪽이 생명력 흡수만큼 회복한다.
			UAbilitySystemComponent* SourceAbilitySystem = Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent();
			if (SourceAbilitySystem && SourceAbilitySystem != GetOwningAbilitySystemComponent() && Data.EffectSpec.GetDynamicAssetTags().HasTagExact(MobaTags::Damage_BasicAttack)
				&& !SourceAbilitySystem->HasMatchingGameplayTag(MobaTags::State_Dead))
			{
				const float Steal = SourceAbilitySystem->GetNumericAttribute(GetLifeStealAttribute());
				if (Steal > 0.0f)
				{
					const float SourceHealth = SourceAbilitySystem->GetNumericAttributeBase(GetHealthAttribute());
					const float SourceMaxHealth = SourceAbilitySystem->GetNumericAttribute(GetMaxHealthAttribute());
					SourceAbilitySystem->SetNumericAttributeBase(GetHealthAttribute(), FMath::Min(SourceHealth + Damage * Steal, SourceMaxHealth));
				}
			}

			// 챔피언끼리 주고받은 피해는 양쪽 특성에 알린다.
			AMobaCharacter* VictimChampion = Cast<AMobaCharacter>(GetOwningAbilitySystemComponentChecked()->GetAvatarActor());
			AMobaCharacter* SourceChampion = Cast<AMobaCharacter>(DamageInstigator);
			if (VictimChampion && SourceChampion && VictimChampion != SourceChampion && VictimChampion->GetTeam() != SourceChampion->GetTeam())
			{
				SourceChampion->HandleDamageDealtToChampion(VictimChampion, Data.EffectSpec.GetDynamicAssetTags().HasTagExact(MobaTags::Damage_BasicAttack));
				if (NewHealth > 0.0f)
				{
					VictimChampion->HandleDamagedByChampion(SourceChampion);
				}
			}

			if (NewHealth > 0.0f && Data.EffectSpec.GetDynamicAssetTags().HasTagExact(MobaTags::Damage_BasicAttack))
			{
				if (AMobaCharacter* Victim = Cast<AMobaCharacter>(GetOwningAbilitySystemComponentChecked()->GetAvatarActor()))
				{
					Victim->HandleBasicAttackTaken(DamageInstigator);
				}
			}

			if (NewHealth <= 0.0f)
			{
				OnOutOfHealth.Broadcast(DamageInstigator);
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.0f, GetMaxMana()));
	}
}

void UMobaAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, Health, OldValue);
}

void UMobaAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, MaxHealth, OldValue);
}

void UMobaAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, Mana, OldValue);
}

void UMobaAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, MaxMana, OldValue);
}

void UMobaAttributeSet::OnRep_AttackDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, AttackDamage, OldValue);
}

void UMobaAttributeSet::OnRep_AbilityPower(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, AbilityPower, OldValue);
}

void UMobaAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, Armor, OldValue);
}

void UMobaAttributeSet::OnRep_MagicResist(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, MagicResist, OldValue);
}

void UMobaAttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, AttackSpeed, OldValue);
}

void UMobaAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, MoveSpeed, OldValue);

	OnMoveSpeedChanged.Broadcast(GetMoveSpeed());
}

void UMobaAttributeSet::OnRep_AttackRange(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, AttackRange, OldValue);
}

void UMobaAttributeSet::OnRep_LifeSteal(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, LifeSteal, OldValue);
}

void UMobaAttributeSet::OnRep_CritChance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, CritChance, OldValue);
}

void UMobaAttributeSet::OnRep_ArmorPenetration(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, ArmorPenetration, OldValue);
}

void UMobaAttributeSet::OnRep_MagicPenetration(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, MagicPenetration, OldValue);
}

void UMobaAttributeSet::OnRep_BonusHealthRegen(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, BonusHealthRegen, OldValue);
}

void UMobaAttributeSet::OnRep_BonusManaRegen(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, BonusManaRegen, OldValue);
}

void UMobaAttributeSet::OnRep_AbilityHaste(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, AbilityHaste, OldValue);
}

void UMobaAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, Shield, OldValue);
}
