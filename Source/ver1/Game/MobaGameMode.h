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
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;

	/**
	 * 같은 팀에서 몇 번째 플레이어인지에 따라 시작 지점에서 비켜 설 거리. 팀마다 시작 지점이 하나뿐이어도
	 * 2대2 이상에서 챔피언이 겹쳐 생성·부활하지 않게 한다. 첫 번째 플레이어는 0.
	 */
	static FVector GetTeamSlotOffset(int32 SlotIndex);
	virtual void StartPlay() override;

	/** 고를 수 있는 챔피언이 하나도 없을 때 쓰는 챔피언. 지정하지 않았으면 nullptr. */
	UMobaChampionData* GetDefaultChampion() const;

	/** 고를 수 있는 챔피언 목록. DefaultGame.ini의 ChampionRoster로 정한다. 메뉴에서도 읽는다. */
	const TArray<FSoftObjectPath>& GetChampionRoster() const { return ChampionRoster; }

	/** 서버에서 호출하면 모든 PC의 화면에 알림 한 줄이 뜬다. */
	void Announce(const FString& Message, EMobaTeam Team);

	/** 서버 전용. 로비에서 챔피언을 바꾼다. 경기가 시작된 뒤에는 무시한다. */
	void RequestChampionChange(APlayerController* Player, const FString& AssetName);

	/**
	 * 접속 옵션의 "Spells" 값("Flash,Smite"처럼 D, F 순서)을 해석한다. 이름은 대소문자를 가리지 않는다.
	 * 둘 다 알맞고 서로 다를 때만 true.
	 */
	static bool ParseSpellsOption(const FString& Value, EMobaSummonerSpell& OutD, EMobaSummonerSpell& OutF);

	/** 접속 옵션에 넣을 "Flash,Smite" 형태의 문자열. */
	static FString MakeSpellsOption(EMobaSummonerSpell SpellD, EMobaSummonerSpell SpellF);

	/** 서버 전용. 로비에서 소환사 주문을 바꾼다. Index 0이 D, 1이 F. */
	void RequestSummonerSpell(APlayerController* Player, int32 Index, EMobaSummonerSpell Spell);

	/** 서버 전용. 포탑 방패가 하나 깨졌다. 깬 챔피언(미니언이면 근처의 상대 챔피언들)에게 골드를 준다. */
	void HandleTowerPlateDestroyed(class AMobaTower* Tower, AActor* DamageInstigator);

	/** 서버 전용. 로비에서 준비 상태를 바꾼다. */
	void RequestReady(APlayerController* Player, bool bReady);

	/** 미니언 종류별 수치와 외형. 클라이언트는 외형을 고를 때 클래스 기본값에서 읽는다. */
	const FMobaMinionConfig& GetMinionConfig(EMobaMinionType Type) const;

	/** 서버 전용. Summoner가 전령의 눈을 써서 가장 가까운 라인에 아군 전령을 소환한다. 성공하면 true. */
	bool SummonHerald(class AMobaCharacter* Summoner);

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

	/** 경기를 끝내고 모든 유닛을 멈춘다. bForfeit는 상대가 나가서, bSurrender는 진 팀이 항복해서 끝난 경우. */
	void EndMatch(EMobaTeam WinningTeam, bool bForfeit = false, bool bSurrender = false);

	/** 서버 전용. 항복에 동의한다. 팀 전원이 동의하면 그 팀의 패배로 끝난다. 경기 시작 후 SurrenderMinTime이 지나야 한다. */
	void RequestSurrender(APlayerController* Player);

	/** 항복할 수 있게 되는 경기 시간(초). */
	float GetSurrenderMinTime() const { return SurrenderMinTime; }

	/** 해당 레벨 챔피언의 부활 대기 시간(초). */
	float GetRespawnTime(int32 ChampionLevel) const;

	/** 처치당한 챔피언의 연속 처치·연속 죽음 기록으로 정하는 처치 골드. */
	float CalculateKillBounty(int32 VictimKillStreak, int32 VictimDeathStreak) const;

	/** 해당 경기 시간에 새로 나오는 미니언이 받은 강화 횟수. */
	int32 GetMinionUpgradeCount(float MatchTimeSeconds) const;

protected:
	/** 필요한 인원이 모였으면(로비를 쓰면 모두 준비되면) 경기를 시작한다. 미니언 웨이브와 골드 지급이 이때부터 돈다. */
	void TryStartMatch();

	/** 카운트다운이 끝났을 때. 모두 그대로 준비돼 있으면 챔피언을 내보내고 경기를 시작한다. */
	void FinishCountdown();

	void StartMatchNow();

	/** 로비에서 모두 준비됐는지. */
	bool AreAllPlayersReady() const;

	/** 챔피언끼리 준 피해를 결과 화면용으로 모은다. 어시스트 판정용으로 누가 언제 때렸는지도 적어 둔다. */
	void HandleDamageDealt(AActor* DamageInstigator, AActor* Victim, float Damage);

	/** 이 챔피언을 처치했을 때 주는 골드. 연속 처치 중이면 늘고, 계속 죽고 있으면 준다. */
	float GetKillBounty(const AMobaPlayerState* VictimState) const;

	/** 챔피언이 최근에 받은 피해 한 건(사망 요약용). */
	struct FRecentHit
	{
		double Time = 0.0;
		FString Source;
		float Damage = 0.0f;
	};

	/** 챔피언마다 최근 DeathRecapWindow초 동안 받은 피해. */
	TMap<TWeakObjectPtr<AMobaPlayerState>, TArray<FRecentHit>> RecentHitsTaken;

	/** 사망 요약에 넣는 기간(초). */
	static constexpr double DeathRecapWindow = 10.0;

	/** 처치당한 플레이어의 PC에 사망 요약을 보내고 기록을 비운다. */
	void SendDeathRecap(AMobaCharacter* Victim, AActor* Killer);

	/** 챔피언마다 최근에 피해를 준 상대 챔피언과 그 시각. */
	TMap<TWeakObjectPtr<AMobaPlayerState>, TMap<TWeakObjectPtr<AMobaPlayerState>, double>> RecentChampionDamage;

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

	/** 전령의 눈으로 소환하는 협곡의 전령. */
	UPROPERTY(EditDefaultsOnly, Category = "Minions")
	FMobaMinionConfig HeraldMinion;

	/** 전령의 눈을 들고 있을 수 있는 시간(초). */
	UPROPERTY(EditDefaultsOnly, Category = "Jungle")
	float HeraldEyeDuration = 240.0f;

	/** 소환된 전령이 살아 있는 시간(초). */
	UPROPERTY(EditDefaultsOnly, Category = "Jungle")
	float SummonedHeraldLifetime = 90.0f;

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

	/** 경기 전에 챔피언을 고르는 로비를 거칠지. 접속 주소에 Players(2 이상)가 있을 때 켜진다. */
	bool bUseLobby = false;


	/** 모두 준비된 뒤 시작까지의 시간(초). */
	UPROPERTY(EditDefaultsOnly, Category = "Match")
	float StartCountdown = 3.0f;

	FTimerHandle CountdownTimer;

	/** 로비 없이(에디터·연습) 받을 수 있는 최대 인원. 로비가 있으면 방을 만들 때 정한 인원(RequiredPlayers)까지만 받는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Match")
	int32 MaxPlayers = 4;

	/** 같은 팀에서 Player가 몇 번째인지(들어온 순서). */
	int32 GetTeamSlotIndex(const AController* Player) const;

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

	/** 1레벨에서 사망 후 부활까지 걸리는 시간(초). */
	UPROPERTY(EditAnywhere, Category = "Respawn")
	float RespawnDelay = 5.0f;

	/** 레벨이 하나 오를 때마다 늘어나는 부활 시간(초). */
	UPROPERTY(EditAnywhere, Category = "Respawn")
	float RespawnDelayPerLevel = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Respawn")
	float MaxRespawnDelay = 50.0f;

	/** 연속 처치 한 번마다 현상금에 더해지는 골드. 2연속부터 붙는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Rewards")
	float KillStreakBountyStep = 150.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Rewards")
	float MaxKillBounty = 1000.0f;

	/** 연속으로 죽을 때마다 현상금에서 빠지는 골드. 2연속부터 줄어든다. */
	UPROPERTY(EditDefaultsOnly, Category = "Rewards")
	float DeathStreakBountyStep = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Rewards")
	float MinKillBounty = 100.0f;

	/** 처치 직전 이 시간(초) 안에 피해를 준 아군은 어시스트를 받는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Rewards")
	float AssistWindow = 10.0f;

	/** 어시스트한 아군들이 나눠 받는 골드(현상금 대비 비율). */
	UPROPERTY(EditDefaultsOnly, Category = "Rewards")
	float AssistGoldFraction = 0.5f;

	/** 경기의 첫 챔피언 처치에 더 주는 골드. */
	UPROPERTY(EditDefaultsOnly, Category = "Rewards")
	float FirstBloodBonusGold = 100.0f;

	/** 포탑 방패 하나를 깼을 때의 골드. */
	UPROPERTY(EditDefaultsOnly, Category = "Rewards")
	float TowerPlateGold = 160.0f;

	/** 미니언이 방패를 깼을 때 골드를 나눠 받는 근처 상대 챔피언의 거리. */
	UPROPERTY(EditDefaultsOnly, Category = "Rewards")
	float TowerPlateShareRadius = 1200.0f;

	/** 경기 시작 후 이 시간(초)이 지나면 포탑 방패가 모두 사라진다. */
	UPROPERTY(EditDefaultsOnly, Category = "Rewards")
	float TowerPlatingDuration = 840.0f;

	bool bFirstBloodTaken = false;
	FTimerHandle PlatingTimer;

	/** 포탑 방패 시간이 끝났을 때. */
	void EndTowerPlating();

	/** 이 간격(초)마다 새로 나오는 미니언이 강해진다. */
	UPROPERTY(EditDefaultsOnly, Category = "Minions")
	float MinionUpgradeInterval = 90.0f;

	/** 강화 한 번마다 체력·공격력·보상 골드가 늘어나는 비율. */
	UPROPERTY(EditDefaultsOnly, Category = "Minions")
	float MinionHealthPerUpgrade = 0.05f;

	UPROPERTY(EditDefaultsOnly, Category = "Minions")
	float MinionDamagePerUpgrade = 0.04f;

	UPROPERTY(EditDefaultsOnly, Category = "Minions")
	float MinionGoldPerUpgrade = 0.02f;

	UPROPERTY(EditDefaultsOnly, Category = "Minions")
	int32 MaxMinionUpgrades = 20;

	/** 항복할 수 있게 되는 경기 시간(초). */
	UPROPERTY(EditAnywhere, Category = "Match")
	float SurrenderMinTime = 180.0f;
};
