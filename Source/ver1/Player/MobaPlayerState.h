#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Game/MobaTypes.h"
#include "MobaPlayerState.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMobaTeamChanged, EMobaTeam);

UCLASS()
class VER1_API AMobaPlayerState : public APlayerState, public IMobaTeamInterface
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual EMobaTeam GetTeam() const override { return Team; }

	/** 서버 전용. */
	void SetTeam(EMobaTeam NewTeam);

	/** 서버와 클라이언트 양쪽에서 팀이 정해지거나 바뀔 때 호출된다. */
	FOnMobaTeamChanged OnTeamChanged;

protected:
	UFUNCTION()
	void OnRep_Team();

	UPROPERTY(ReplicatedUsing = OnRep_Team, VisibleAnywhere, BlueprintReadOnly, Category = "Team")
	EMobaTeam Team = EMobaTeam::None;
};
