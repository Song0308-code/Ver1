#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MobaMenuPlayerController.generated.h"

class SMobaMainMenu;

/** 메인 메뉴 화면을 띄우고, 방 만들기·접속 같은 메뉴 동작을 실행한다. */
UCLASS()
class VER1_API AMobaMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMobaMenuPlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 방을 열고 상대가 들어올 때까지 기다린다. */
	void HostGame();

	/** 혼자서 바로 시작한다. */
	void StartPractice();

	/** 주소로 접속을 시도한다. 주소가 쓸 수 없는 형태면 false를 돌려준다. */
	bool JoinGame(const FString& Address);

	void QuitGame();

protected:
	/** 경기를 진행하는 맵. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	FString GameMap = TEXT("/Game/Maps/L_SummonersRift");

	/** 방을 만들었을 때 경기를 시작하는 데 필요한 인원. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	int32 HostRequiredPlayers = 2;

	TSharedPtr<SMobaMainMenu> MenuWidget;
};
