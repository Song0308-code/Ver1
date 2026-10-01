#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Game/MobaTypes.h"
#include "MobaGameState.generated.h"

UCLASS()
class VER1_API AMobaGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool IsMatchOver() const { return bMatchOver; }

	/** 경기가 끝났을 때 이긴 팀. 진행 중이면 None. */
	EMobaTeam GetWinningTeam() const { return WinningTeam; }

	/** 서버 전용. */
	void SetMatchResult(EMobaTeam InWinningTeam);

protected:
	UPROPERTY(Replicated)
	bool bMatchOver = false;

	UPROPERTY(Replicated)
	EMobaTeam WinningTeam = EMobaTeam::None;
};
