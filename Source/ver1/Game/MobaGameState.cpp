#include "Game/MobaGameState.h"
#include "Net/UnrealNetwork.h"

void AMobaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMobaGameState, bMatchOver);
	DOREPLIFETIME(AMobaGameState, WinningTeam);
}

void AMobaGameState::SetMatchResult(EMobaTeam InWinningTeam)
{
	if (HasAuthority())
	{
		bMatchOver = true;
		WinningTeam = InWinningTeam;
	}
}
