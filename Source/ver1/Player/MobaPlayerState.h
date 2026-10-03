#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "Game/MobaTypes.h"
#include "MobaPlayerState.generated.h"

class UAbilitySystemComponent;
class UMobaAttributeSet;
class UMobaChampionData;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMobaTeamChanged, EMobaTeam);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMobaLevelUp, int32 /*NewLevel*/);

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

	/** 서버 전용. */
	void AddGold(float Amount) { Gold += Amount; }

	/** 서버 전용. 골드가 충분하면 쓰고 true를 돌려준다. */
	bool SpendGold(float Amount);

	/** 서버 전용. 이 플레이어가 쓸 챔피언. 스폰 전에 정한다. */
	void SetChampion(UMobaChampionData* InChampion) { Champion = InChampion; }
	UMobaChampionData* GetChampion() const { return Champion; }

	/** 화면에 보여 줄 챔피언 이름. */
	FString GetChampionDisplayName() const;

	/** 가방에 든 아이템 번호들(MobaItems::GetItems의 인덱스). */
	const TArray<uint8>& GetItems() const { return Items; }

	/** 서버 전용. */
	void AddItem(uint8 ItemIndex) { Items.Add(ItemIndex); }
	void RemoveItemAt(int32 Slot) { Items.RemoveAt(Slot); }

	/** 서버 전용. 미니언·몬스터 처치 수. */
	void AddMinionKill() { ++MinionKills; }
	int32 GetMinionKills() const { return MinionKills; }

	/** 서버 전용. 와드를 다시 놓을 수 있는 시각(서버 월드 시간). */
	void SetNextWardServerTime(float NewTime) { NextWardServerTime = NewTime; }
	float GetNextWardServerTime() const { return NextWardServerTime; }

	/** 서버 전용. 경험치를 더하고 필요한 만큼 레벨을 올린다. */
	void AddExperience(float Amount);

	/** 서버 전용. 스킬 포인트가 있으면 하나 쓰고 true를 돌려준다. */
	bool ConsumeSkillPoint();

	int32 GetChampionLevel() const { return ChampionLevel; }
	float GetExperience() const { return Experience; }
	float GetGold() const { return Gold; }
	int32 GetSkillPoints() const { return SkillPoints; }

	/** 해당 레벨에서 다음 레벨까지 필요한 경험치. */
	static float GetExperienceForNextLevel(int32 Level);

	static constexpr int32 MaxChampionLevel = 18;

	/** 서버에서 레벨이 오를 때마다 호출된다. */
	FOnMobaLevelUp OnLevelUp;

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

	UPROPERTY(Replicated)
	float NextWardServerTime = 0.0f;

	UPROPERTY(Replicated)
	TObjectPtr<UMobaChampionData> Champion;

	UPROPERTY(Replicated)
	TArray<uint8> Items;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Score")
	int32 MinionKills = 0;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	int32 ChampionLevel = 1;

	/** 현재 레벨에서 쌓은 경험치. 레벨이 오르면 필요량만큼 빠진다. */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	float Experience = 0.0f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	float Gold = 500.0f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	int32 SkillPoints = 1;
};
