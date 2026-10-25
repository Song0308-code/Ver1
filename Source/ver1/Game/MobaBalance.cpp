#include "Game/MobaBalance.h"
#include "Game/MobaGameMode.h"
#include "Game/MobaItems.h"
#include "Player/MobaPlayerState.h"

float MobaBalance::GetStatGoldValue(const FMobaItemDefinition& Item)
{
	if (Item.bConsumable)
	{
		return 0.0f;
	}

	return Item.AttackDamage * GoldPerAttackDamage
		+ Item.AbilityPower * GoldPerAbilityPower
		+ Item.MaxHealth * GoldPerHealth
		+ Item.MaxMana * GoldPerMana
		+ Item.Armor * GoldPerArmor
		+ Item.MagicResist * GoldPerMagicResist
		+ Item.AttackSpeedFraction * 100.0f * GoldPerAttackSpeedPercent
		+ Item.MoveSpeed * GoldPerMoveSpeed
		+ Item.LifeSteal * 100.0f * GoldPerLifeStealPercent
		+ Item.AbilityHaste * GoldPerAbilityHaste
		+ Item.CritChance * 100.0f * GoldPerCritPercent;
}

float MobaBalance::GetGoldEfficiency(const FMobaItemDefinition& Item)
{
	return Item.Cost > 0 ? GetStatGoldValue(Item) / Item.Cost : 0.0f;
}

int32 MobaBalance::GetItemTier(int32 ItemIndex)
{
	const FMobaItemDefinition* Item = MobaItems::GetItem(ItemIndex);
	if (!Item)
	{
		return 0;
	}

	int32 Tier = 1;
	for (const int32 Component : Item->Components)
	{
		Tier = FMath::Max(Tier, GetItemTier(Component) + 1);
	}
	return Tier;
}

int32 MobaBalance::GetLevelForExperience(float TotalExperience)
{
	int32 Level = 1;
	while (Level < AMobaPlayerState::MaxChampionLevel && TotalExperience >= AMobaPlayerState::GetExperienceForNextLevel(Level))
	{
		TotalExperience -= AMobaPlayerState::GetExperienceForNextLevel(Level);
		++Level;
	}
	return Level;
}

MobaBalance::FLaneEstimate MobaBalance::EstimateLane(const AMobaGameMode& Mode, float MatchSeconds, float CsFraction)
{
	FLaneEstimate Estimate;
	Estimate.Gold = GetDefault<AMobaPlayerState>()->GetGold() + Mode.GetPassiveGoldPerSecond() * MatchSeconds;

	const FMobaMinionConfig& Melee = Mode.GetMinionConfig(EMobaMinionType::Melee);
	const FMobaMinionConfig& Caster = Mode.GetMinionConfig(EMobaMinionType::Caster);
	const FMobaMinionConfig& Siege = Mode.GetMinionConfig(EMobaMinionType::Siege);
	const int32 PerType = Mode.GetMinionsPerType();

	for (int32 Wave = 1; ; ++Wave)
	{
		const float SpawnTime = Mode.GetFirstWaveDelay() + Mode.GetWaveInterval() * (Wave - 1);
		if (SpawnTime + LaneTravelSeconds > MatchSeconds)
		{
			break;
		}

		const int32 SiegeCount = Mode.GetSiegeMinionCount(Wave, SpawnTime, 0);
		const float GoldScale = 1.0f + Mode.GetMinionGoldPerUpgrade() * Mode.GetMinionUpgradeCount(SpawnTime);
		Estimate.Gold += CsFraction * GoldScale * (PerType * (Melee.GoldReward + Caster.GoldReward) + SiegeCount * Siege.GoldReward);
		Estimate.Experience += PerType * (Melee.ExperienceReward + Caster.ExperienceReward) + SiegeCount * Siege.ExperienceReward;
	}

	Estimate.Level = GetLevelForExperience(Estimate.Experience);
	return Estimate;
}
