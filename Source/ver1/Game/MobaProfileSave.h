#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Game/MobaTypes.h"
#include "MobaProfileSave.generated.h"

/** 메뉴·로비에서 마지막으로 고른 챔피언과 소환사 주문. 다음에 실행해도 그대로 쓴다. */
UCLASS()
class VER1_API UMobaProfileSave : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr const TCHAR* SlotName = TEXT("MobaProfile");

	UPROPERTY()
	FString ChampionAssetName;

	UPROPERTY()
	EMobaSummonerSpell SpellD = EMobaSummonerSpell::Flash;

	UPROPERTY()
	EMobaSummonerSpell SpellF = EMobaSummonerSpell::Ignite;

	/** 방을 만들 때의 인원. 2(1대1) 또는 4(2대2). */
	UPROPERTY()
	int32 HostPlayers = 2;
};
