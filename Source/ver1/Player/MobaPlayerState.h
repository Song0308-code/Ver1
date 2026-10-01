#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "Game/MobaTypes.h"
#include "MobaPlayerState.generated.h"

class UAbilitySystemComponent;
class UMobaAttributeSet;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMobaTeamChanged, EMobaTeam);

UCLASS()
class VER1_API AMobaPlayerState : public APlayerState, public IMobaTeamInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMobaPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual EMobaTeam GetTeam() const override { return Team; }
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UMobaAttributeSet* GetAttributeSet() const { return AttributeSet; }

	/** 서버 전용. */
	void SetTeam(EMobaTeam NewTeam);

	/** 서버 전용. */
	void AddKill() { ++Kills; }
	void AddDeath() { ++Deaths; }
	void SetRespawnServerTime(float NewTime) { RespawnServerTime = NewTime; }

	int32 GetKills() const { return Kills; }
	int32 GetDeaths() const { return Deaths; }

	/** 부활 예정 시각(서버 월드 시간). 살아 있으면 0. */
	float GetRespawnServerTime() const { return RespawnServerTime; }

	/** 서버와 클라이언트 양쪽에서 팀이 정해지거나 바뀔 때 호출된다. */
	FOnMobaTeamChanged OnTeamChanged;

	/** 서버에서 기본 스탯을 한 번만 채우기 위한 표시. */
	bool bStatsInitialized = false;

protected:
	UFUNCTION()
	void OnRep_Team();

	/** 챔피언이 죽었다 살아나도 유지되도록 능력 시스템은 PlayerState가 가진다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UMobaAttributeSet> AttributeSet;

	UPROPERTY(ReplicatedUsing = OnRep_Team, VisibleAnywhere, BlueprintReadOnly, Category = "Team")
	EMobaTeam Team = EMobaTeam::None;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Score")
	int32 Kills = 0;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Score")
	int32 Deaths = 0;

	UPROPERTY(Replicated)
	float RespawnServerTime = 0.0f;
};
