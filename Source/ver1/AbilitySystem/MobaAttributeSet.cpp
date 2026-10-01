#include "AbilitySystem/MobaAttributeSet.h"
#include "Combat/MobaCombatEvents.h"
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
		const float Damage = GetIncomingDamage();
		SetIncomingDamage(0.0f);

		if (Damage > 0.0f && GetHealth() > 0.0f)
		{
			// 방어력 계산의 부동소수 오차로 체력이 0에 못 미치는 극소량 남는 것을 막는다.
			float NewHealth = FMath::Clamp(GetHealth() - Damage, 0.0f, GetMaxHealth());
			if (NewHealth < 0.01f)
			{
				NewHealth = 0.0f;
			}
			SetHealth(NewHealth);

			AActor* DamageInstigator = Data.EffectSpec.GetContext().GetOriginalInstigator();
			if (UMobaCombatEvents* Events = UMobaCombatEvents::Get(GetOwningActor()))
			{
				Events->OnDamageDealt.Broadcast(DamageInstigator, GetOwningAbilitySystemComponentChecked()->GetAvatarActor(), Damage);
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
