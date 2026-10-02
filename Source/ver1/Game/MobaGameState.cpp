#include "Game/MobaGameState.h"
#include "Net/UnrealNetwork.h"

void AMobaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMobaGameState, bMatchStarted);
	DOREPLIFETIME(AMobaGameState, MatchStartServerTime);
	DOREPLIFETIME(AMobaGameState, bMatchOver);
	DOREPLIFETIME(AMobaGameState, MatchEndServerTime);
	DOREPLIFETIME(AMobaGameState, WinningTeam);
	DOREPLIFETIME(AMobaGameState, bEndedByForfeit);
}

float AMobaGameState::GetMatchTimeSeconds() const
{
	if (!bMatchStarted)
	{
		return 0.0f;
	}

	const float Now = bMatchOver ? MatchEndServerTime : static_cast<float>(GetServerWorldTimeSeconds());
	return FMath::Max(Now - MatchStartServerTime, 0.0f);
}

void AMobaGameState::StartMatch()
{
	if (HasAuthority() && !bMatchStarted)
	{
		bMatchStarted = true;
		MatchStartServerTime = GetServerWorldTimeSeconds();
	}
}

void AMobaGameState::SetMatchResult(EMobaTeam InWinningTeam, bool bForfeit)
{
	if (HasAuthority())
	{
		bMatchOver = true;
		MatchEndServerTime = GetServerWorldTimeSeconds();
		WinningTeam = InWinningTeam;
		bEndedByForfeit = bForfeit;
	}
}
