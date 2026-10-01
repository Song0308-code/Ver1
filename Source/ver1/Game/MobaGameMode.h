#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Game/MobaTypes.h"
#include "Units/MobaMinion.h"
#include "MobaGameMode.generated.h"

class AMobaCharacter;
class AMobaLanePath;
class AMobaStructure;
class UMobaChampionData;

UCLASS(Config = Game)
class VER1_API AMobaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMobaGameMode();

	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void StartPlay() override;

	/** 챔피언 선택이 생기기 전까지 모든 플레이어가 쓰는 챔피언. 지정하지 않았으면 nullptr. */
	UMobaChampionData* GetDefaultChampion() const;

	/** 챔피언이 죽었을 때 서버에서 호출된다. 전적을 기록하고 부활을 예약한다. */
	void HandleChampionKilled(AMobaCharacter* Victim, AActor* Killer);

	/** 미니언이 죽었을 때. 막타 골드와 주변 경험치를 나눠 준다. */
	void HandleMinionKilled(AMobaMinion* Minion, AActor* Killer);

	/** 건물이 파괴됐을 때. 골드를 주고, 넥서스였다면 경기를 끝낸다. */
	void HandleStructureDestroyed(AMobaStructure* Structure, AActor* Killer);

	/** 경기를 끝내고 모든 유닛을 멈춘다. */
	void EndMatch(EMobaTeam WinningTeam);

protected:
	/** 인원이 더 적은 팀을 고른다. 같으면 블루. */
	EMobaTeam PickTeamForNewPlayer() const;

	void RespawnChampion(AMobaCharacter* Champion);

	/** 활성화된 라인마다 양 팀의 미니언 한 무리를 순서대로 내보낸다. */
	void SpawnWave();

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

	FTimerHandle WaveTimer;
	FTimerHandle PassiveGoldTimer;

	/** DefaultGame.ini의 [/Script/ver1.MobaGameMode] DefaultChampion으로 지정한다. */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Champion")
	TSoftObjectPtr<UMobaChampionData> DefaultChampion;

	/** 사망 후 부활까지 걸리는 시간(초). */
	UPROPERTY(EditAnywhere, Category = "Respawn")
	float RespawnDelay = 5.0f;
};
