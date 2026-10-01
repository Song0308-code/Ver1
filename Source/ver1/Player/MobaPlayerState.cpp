#include "Player/MobaPlayerState.h"
#include "Net/UnrealNetwork.h"

void AMobaPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMobaPlayerState, Team);
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
