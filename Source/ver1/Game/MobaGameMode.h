#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Game/MobaTypes.h"
#include "GameplayTagContainer.h"
#include "Units/MobaMinion.h"
#include "MobaGameMode.generated.h"

class AMobaCharacter;
class AMobaJungleCamp;
class AMobaLanePath;
class AMobaMonster;
class AMobaPlayerState;
class AMobaStructure;
class UMobaChampionData;

UCLASS(Config = Game)
class VER1_API AMobaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMobaGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void StartPlay() override;

	/** 고를 수 있는 챔피언이 하나도 없을 때 쓰는 챔피언. 지정하지 않았으면 nullptr. */
	UMobaChampionData* GetDefaultChampion() const;

	/** 고를 수 있는 챔피언 목록. DefaultGame.ini의 ChampionRoster로 정한다. 메뉴에서도 읽는다. */
	const TArray<FSoftObjectPath>& GetChampionRoster() const { return ChampionRoster; }

	/** 서버에서 호출하면 모든 PC의 화면에 알림 한 줄이 뜬다. */
	void Announce(const FString& Message, EMobaTeam Team);

	/** 챔피언이 죽었을 때 서버에서 호출된다. 전적을 기록하고 부활을 예약한다. */
	void HandleChampionKilled(AMobaCharacter* Victim, AActor* Killer);

	/** 미니언이 죽었을 때. 막타 골드와 주변 경험치를 나눠 준다. */
	void HandleMinionKilled(AMobaMinion* Minion, AActor* Killer);

	/** 정글 몬스터가 죽었을 때. 막타를 친 챔피언에게 골드와 경험치를 준다. */
	void HandleMonsterKilled(AMobaMonster* Monster, AActor* Killer);

	/** 캠프의 몬스터가 전멸했을 때. 캠프 종류에 따른 보상(버프, 시야, 팀 골드)을 준다. */
	void HandleCampCleared(AMobaJungleCamp* Camp, AActor* Killer);

	/** 건물이 파괴됐을 때. 골드를 주고, 넥서스였다면 경기를 끝낸다. */
	void HandleStructureDestroyed(AMobaStructure* Structure, AActor* Killer);

	/** 경기를 끝내고 모든 유닛을 멈춘다. bForfeit는 상대가 나가서 끝난 경우. */
	void EndMatch(EMobaTeam WinningTeam, bool bForfeit = false);

protected:
	/** 필요한 인원이 모였으면 경기를 시작한다. 미니언 웨이브와 골드 지급이 이때부터 돈다. */
	void TryStartMatch();

	/** 인원이 더 적은 팀을 고른다. 같으면 블루. */
	EMobaTeam PickTeamForNewPlayer() const;

	/**
	 * 접속 옵션의 Champion(에셋 이름)에 해당하는 챔피언을 고른다.
	 * 옵션이 없으면 들어온 순서대로 목록을 돌아가며 배정한다(에디터에서 메뉴 없이 실행할 때).
	 */
	UMobaChampionData* PickChampionForNewPlayer(const FString& Options, const AMobaPlayerState* NewPlayerState) const;

	/** 알림에 쓰는 유닛 이름 ("블루 레인저", "레드 타워" 등). */
	FString DescribeUnit(const AActor* Unit) const;

	void RespawnChampion(AMobaCharacter* Champion);

	/** 활성화된 라인마다 양 팀의 미니언 한 무리를 순서대로 내보낸다. */
	void SpawnWave();

	/** 해당 팀이 이 라인에 내보낼 슈퍼 미니언 수. 그 라인의 적 억제기가 파괴돼 있으면 1, 적 억제기가 전부 파괴돼 있으면 2. */
	int32 GetSuperMinionCount(EMobaTeam Team, EMobaLane Lane) const;

	const FMobaMinionConfig& GetMinionConfig(EMobaMinionType Type) const;

	/** 해당 팀의 살아 있는 챔피언 전원에게 버프를 건다. */
	void ApplyBuffToTeam(EMobaTeam Team, const FGameplayTag& BuffTag, float Duration, float BonusAttackDamage, float BonusAbilityPower);

	void ApplyBuff(AMobaCharacter* Champion, const FGameplayTag& BuffTag, float Duration, float BonusAttackDamage, float BonusAbilityPower);

	void SpawnMinion(AMobaLanePath* LanePath, EMobaTeam Team, EMobaMinionType Type);

	void GrantPassiveGold();

	/** 해당 팀의 챔피언 전원에게 골드를 준다. */
	void GrantGoldToTeam(EMobaTeam Team, float Amount);

	/** 미니언이 나오는 라인. 나머지 라인은 길만 있고 비어 있다. */
	UPROPERTY(EditDefaultsOnly, Category = "Minions")
	TArray<EMobaLane> ActiveLanes;

	UPROPERTY(EditDefaultsOnly, Category = "Minions")
	TSubclassOf<AMobaMinion> MinionClass;

	UPROPERTY(EditDefaultsOnly, Category = "Minions")
	FMobaMinionConfig MeleeMinion;

	UPROPERTY(EditDefaultsOnly, Category = "Minions")
	FMobaMinionConfig CasterMinion;

	UPROPERTY(EditDefaultsOnly, Category = "Minions")
	FMobaMinionConfig SuperMinion;

	/** 경기 시작 후 첫 웨이브까지의 시간(초). */
	UPROPERTY(EditAnywhere, Category = "Minions")
	float FirstWaveDelay = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Minions")
	float WaveInterval = 30.0f;

	/** 한 웨이브에서 종류별로 나오는 수. */
	UPROPERTY(EditDefaultsOnly, Category = "Minions")
	int32 MinionsPerType = 3;

	/** 한 마리씩 나오는 간격(초). */
	UPROPERTY(EditDefaultsOnly, Category = "Minions")
	float SpawnStagger = 0.8f;

	/** 넥서스 중심에서 라인 쪽으로 이만큼 떨어진 곳에서 나온다. */
	UPROPERTY(EditDefaultsOnly, Category = "Minions")
	float SpawnDistanceFromNexus = 500.0f;

	/** 죽은 미니언에게서 이 거리 안에 있는 적 챔피언이 경험치를 나눠 받는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Rewards")
	float ExperienceShareRange = 1400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Rewards")
	float ChampionKillGold = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Rewards")
	float ChampionKillExperience = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Rewards")
	float PassiveGoldPerSecond = 2.0f;

	/**
	 * 경기를 시작하는 데 필요한 인원. 접속 주소의 Players 옵션으로 덮어쓴다(예: ?listen?Players=2).
	 * 기본값이 1이라 에디터에서 혼자 실행해도 바로 시작한다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 RequiredPlayers = 1;

	/** 이 인원이 차면 더 받지 않는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 MaxPlayers = 2;

	/** 블루·레드 버프의 지속 시간(초). */
	UPROPERTY(EditDefaultsOnly, Category = "Jungle")
	float CampBuffDuration = 120.0f;

	/** 드래곤을 잡을 때마다 팀 전원이 경기 끝까지 얻는 공격력·주문력. */
	UPROPERTY(EditDefaultsOnly, Category = "Jungle")
	float DragonBonusAttackDamage = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Jungle")
	float DragonBonusAbilityPower = 15.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Jungle")
	float BaronBuffDuration = 180.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Jungle")
	float BaronBonusAttackDamage = 40.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Jungle")
	float BaronBonusAbilityPower = 40.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Jungle")
	float BaronTeamGold = 300.0f;

	/** 바위게를 잡으면 그 자리에 생기는 시야의 지속 시간(초). */
	UPROPERTY(EditDefaultsOnly, Category = "Jungle")
	float ScuttleVisionDuration = 90.0f;

	FTimerHandle WaveTimer;
	FTimerHandle PassiveGoldTimer;

	UPROPERTY(Config, EditDefaultsOnly, Category = "Champion")
	TArray<FSoftObjectPath> ChampionRoster;

	/** DefaultGame.ini의 [/Script/ver1.MobaGameMode] DefaultChampion으로 지정한다. */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Champion")
	TSoftObjectPtr<UMobaChampionData> DefaultChampion;

	/** 사망 후 부활까지 걸리는 시간(초). */
	UPROPERTY(EditAnywhere, Category = "Respawn")
	float RespawnDelay = 5.0f;
};
