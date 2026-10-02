#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/GameInstance.h"
#include "MobaGameInstance.generated.h"

class UNetDriver;

/** 접속 실패나 끊김처럼 메뉴로 돌아간 이유를 맵 이동 사이에 들고 있는다. */
UCLASS()
class VER1_API UMobaGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	/** 메뉴에 한 번 보여 줄 안내 문구를 꺼내고 비운다. */
	FText ConsumePendingMessage();

protected:
	void RecordNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void RecordTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

	FText PendingMessage;

	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;
};
