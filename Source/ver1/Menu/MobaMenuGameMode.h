#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MobaMenuGameMode.generated.h"

/** 메인 메뉴 맵에서 쓰는 게임 모드. 폰 없이 메뉴 컨트롤러만 띄운다. */
UCLASS()
class VER1_API AMobaMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMobaMenuGameMode();
};
