#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Game/MobaTypes.h"
#include "MobaGameState.generated.h"

/** 화면 구석에 잠깐 띄우는 알림 한 줄(처치, 건물 파괴 등). */
struct FMobaAnnouncement
{
	FString Message;

	/** 이 일을 해낸 팀. 글자 색에 쓴다. */
	EMobaTeam Team = EMobaTeam::None;

	/** 받은 시각(이 PC의 월드 시간). */
	double Time = 0.0;
};

UCLASS()
class VER1_API AMobaGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 필요한 인원이 모여 경기가 시작됐는지. 시작 전에는 미니언이 나오지 않는다. */
	bool HasMatchStarted() const { return bMatchStarted; }

	/** 경기 전에 챔피언을 고르는 로비를 거치는 방인지. */
	bool UsesLobby() const { return bUsesLobby; }

	/** 모두 준비돼 시작을 기다리는 중이면 남은 시간(초), 아니면 0. */
	float GetCountdownRemaining() const;

	/** 경기 시작을 위해 모여야 하는 인원. */
	int32 GetRequiredPlayers() const { return RequiredPlayers; }

	/** 서버 전용. */
	void SetLobbyInfo(bool bInUsesLobby, int32 InRequiredPlayers) { bUsesLobby = bInUsesLobby; RequiredPlayers = InRequiredPlayers; }
	void SetCountdownEndServerTime(float InTime) { CountdownEndServerTime = InTime; }

	bool IsMatchOver() const { return bMatchOver; }

	/** 경기가 끝났을 때 이긴 팀. 진행 중이면 None. */
	EMobaTeam GetWinningTeam() const { return WinningTeam; }

	/** 상대가 나가서 끝난 경기인지. */
	bool WasEndedByForfeit() const { return bEndedByForfeit; }

	/** 진 팀이 항복해서 끝난 경기인지. */
	bool WasEndedBySurrender() const { return bEndedBySurrender; }

	/** 경기 시작부터 흐른 시간(초). 시작 전에는 0, 끝난 뒤에는 멈춘다. */
	float GetMatchTimeSeconds() const;

	/** 서버에서 호출하면 모든 PC에 알림이 뜬다. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastAnnounce(const FString& Message, EMobaTeam Team);

	/** 이 PC가 받은 알림. 오래된 것부터 들어 있다. */
	const TArray<FMobaAnnouncement>& GetAnnouncements() const { return Announcements; }

	/** 해당 팀 챔피언들의 처치 수 합계. */
	int32 GetTeamKills(EMobaTeam Team) const;

	/** 서버 전용. */
	void StartMatch();

	/** 서버 전용. */
	void SetMatchResult(EMobaTeam InWinningTeam, bool bForfeit, bool bSurrender = false);

protected:
	UPROPERTY(Replicated)
	bool bMatchStarted = false;

	UPROPERTY(Replicated)
	float MatchStartServerTime = 0.0f;

	UPROPERTY(Replicated)
	bool bMatchOver = false;

	UPROPERTY(Replicated)
	float MatchEndServerTime = 0.0f;

	UPROPERTY(Replicated)
	EMobaTeam WinningTeam = EMobaTeam::None;

	UPROPERTY(Replicated)
	bool bEndedByForfeit = false;

	UPROPERTY(Replicated)
	bool bEndedBySurrender = false;

	UPROPERTY(Replicated)
	bool bUsesLobby = false;

	UPROPERTY(Replicated)
	int32 RequiredPlayers = 1;

	/** 시작 카운트다운이 끝나는 시각(서버 월드 시간). 카운트다운 중이 아니면 0. */
	UPROPERTY(Replicated)
	float CountdownEndServerTime = 0.0f;

	TArray<FMobaAnnouncement> Announcements;
};
