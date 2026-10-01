#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Game/MobaTypes.h"
#include "MobaGameMode.generated.h"

class AMobaCharacter;
class UMobaChampionData;

UCLASS(Config = Game)
class VER1_API AMobaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMobaGameMode();

	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	/** 챔피언 선택이 생기기 전까지 모든 플레이어가 쓰는 챔피언. 지정하지 않았으면 nullptr. */
	UMobaChampionData* GetDefaultChampion() const;

	/** 챔피언이 죽었을 때 서버에서 호출된다. 전적을 기록하고 부활을 예약한다. */
	void HandleChampionKilled(AMobaCharacter* Victim, AActor* Killer);

protected:
	/** 인원이 더 적은 팀을 고른다. 같으면 블루. */
	EMobaTeam PickTeamForNewPlayer() const;

	void RespawnChampion(AMobaCharacter* Champion);

	/** DefaultGame.ini의 [/Script/ver1.MobaGameMode] DefaultChampion으로 지정한다. */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Champion")
	TSoftObjectPtr<UMobaChampionData> DefaultChampion;

	/** 사망 후 부활까지 걸리는 시간(초). */
	UPROPERTY(EditAnywhere, Category = "Respawn")
	float RespawnDelay = 5.0f;
};
