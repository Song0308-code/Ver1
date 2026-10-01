#include "Player/MobaPlayerState.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystemComponent.h"
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
