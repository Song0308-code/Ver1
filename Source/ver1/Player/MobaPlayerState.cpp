#include "Player/MobaPlayerState.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaChampionData.h"
#include "Net/UnrealNetwork.h"

AMobaPlayerState::AMobaPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UMobaAttributeSet>(TEXT("AttributeSet"));

	// PlayerState의 기본 갱신 빈도는 체력·상태 복제에 너무 낮다.
	SetNetUpdateFrequency(100.0f);
}

void AMobaPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMobaPlayerState, Team);
	DOREPLIFETIME(AMobaPlayerState, Kills);
	DOREPLIFETIME(AMobaPlayerState, Deaths);
	DOREPLIFETIME(AMobaPlayerState, RespawnServerTime);
	DOREPLIFETIME(AMobaPlayerState, NextWardServerTime);
	DOREPLIFETIME(AMobaPlayerState, Champion);
	DOREPLIFETIME(AMobaPlayerState, Items);
	DOREPLIFETIME(AMobaPlayerState, MinionKills);
	DOREPLIFETIME(AMobaPlayerState, ChampionLevel);
	DOREPLIFETIME(AMobaPlayerState, Experience);
	DOREPLIFETIME(AMobaPlayerState, Gold);
	DOREPLIFETIME(AMobaPlayerState, SkillPoints);
}

UAbilitySystemComponent* AMobaPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AMobaPlayerState::SetTeam(EMobaTeam NewTeam)
{
	if (!HasAuthority() || Team == NewTeam)
	{
		return;
	}

	Team = NewTeam;
	OnRep_Team();
}

void AMobaPlayerState::OnRep_Team()
{
	OnTeamChanged.Broadcast(Team);
}

bool AMobaPlayerState::SpendGold(float Amount)
{
	if (!HasAuthority() || Gold < Amount)
	{
		return false;
	}

	Gold -= Amount;
	return true;
}

FString AMobaPlayerState::GetChampionDisplayName() const
{
	if (Champion && !Champion->DisplayName.IsEmpty())
	{
		return Champion->DisplayName.ToString();
	}
	return TEXT("챔피언");
}

float AMobaPlayerState::GetExperienceForNextLevel(int32 Level)
{
	return 280.0f + 100.0f * FMath::Max(Level - 1, 0);
}

void AMobaPlayerState::AddExperience(float Amount)
{
	if (!HasAuthority() || ChampionLevel >= MaxChampionLevel)
	{
		return;
	}

	Experience += Amount;
	while (ChampionLevel < MaxChampionLevel && Experience >= GetExperienceForNextLevel(ChampionLevel))
	{
		Experience -= GetExperienceForNextLevel(ChampionLevel);
		++ChampionLevel;
		++SkillPoints;
		OnLevelUp.Broadcast(ChampionLevel);
	}

	if (ChampionLevel >= MaxChampionLevel)
	{
		Experience = 0.0f;
	}
}

bool AMobaPlayerState::ConsumeSkillPoint()
{
	if (!HasAuthority() || SkillPoints <= 0)
	{
		return false;
	}

	--SkillPoints;
	return true;
}
