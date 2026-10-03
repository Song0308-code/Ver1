#include "Game/MobaGameState.h"
#include "Net/UnrealNetwork.h"
#include "Player/MobaPlayerState.h"

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

void AMobaGameState::MulticastAnnounce_Implementation(const FString& Message, EMobaTeam Team)
{
	// 화면에는 최근 몇 줄만 보이므로 오래된 것은 버린다.
	if (Announcements.Num() >= 8)
	{
		Announcements.RemoveAt(0);
	}

	Announcements.Add({ Message, Team, GetWorld()->GetTimeSeconds() });
}

int32 AMobaGameState::GetTeamKills(EMobaTeam Team) const
{
	int32 Total = 0;
	for (const APlayerState* PlayerState : PlayerArray)
	{
		const AMobaPlayerState* MobaPlayerState = Cast<AMobaPlayerState>(PlayerState);
		if (MobaPlayerState && MobaPlayerState->GetTeam() == Team)
		{
			Total += MobaPlayerState->GetKills();
		}
	}
	return Total;
}
