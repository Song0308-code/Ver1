#include "Game/MobaGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"

void UMobaGameInstance::Init()
{
	Super::Init();

	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &UMobaGameInstance::RecordNetworkFailure);
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &UMobaGameInstance::RecordTravelFailure);
	}
}

void UMobaGameInstance::Shutdown()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
		GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	}

	Super::Shutdown();
}

FText UMobaGameInstance::ConsumePendingMessage()
{
	const FText Message = PendingMessage;
	PendingMessage = FText::GetEmpty();
	return Message;
}

void UMobaGameInstance::RecordNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	// 에디터에서는 창마다 게임 인스턴스가 따로 있으므로 내 월드의 일만 본다.
	if (World && World->GetGameInstance() != this)
	{
		return;
	}

	// 호스트 쪽에서 상대 연결이 끊긴 것은 게임 모드가 처리한다. 메뉴로 돌아가는 것은 접속한 쪽뿐이다.
	if (!NetDriver || !NetDriver->ServerConnection)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("네트워크 오류 (%s): %s"), ENetworkFailure::ToString(FailureType), *ErrorString);

	// 아직 접속을 시도하는 중이었는지, 경기에 들어간 뒤였는지에 따라 안내가 달라진다.
	const bool bWasConnecting = NetDriver->NetDriverName == NAME_PendingNetDriver;

	switch (FailureType)
	{
	case ENetworkFailure::ConnectionLost:
	case ENetworkFailure::ConnectionTimeout:
		PendingMessage = FText::FromString(bWasConnecting
			? TEXT("접속하지 못했습니다. 주소와 호스트의 방화벽(UDP 7777)을 확인하세요.")
			: TEXT("호스트와의 연결이 끊어졌습니다."));
		break;

	case ENetworkFailure::FailureReceived:
	case ENetworkFailure::PendingConnectionFailure:
		// 방이 가득 찼을 때처럼 호스트가 거절 사유를 보내 준 경우.
		PendingMessage = FText::FromString(ErrorString.IsEmpty() ? TEXT("호스트가 접속을 거절했습니다.") : ErrorString);
		break;

	default:
		PendingMessage = FText::FromString(FString::Printf(TEXT("네트워크 오류: %s"), ENetworkFailure::ToString(FailureType)));
		break;
	}
}

void UMobaGameInstance::RecordTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	if (World && World->GetGameInstance() != this)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("맵 이동 실패 (%s): %s"), ETravelFailure::ToString(FailureType), *ErrorString);

	// 네트워크 오류가 먼저 남긴 문구가 더 구체적이므로 덮어쓰지 않는다.
	if (PendingMessage.IsEmpty())
	{
		PendingMessage = FText::FromString(TEXT("접속하지 못했습니다. 주소와 호스트의 방화벽(UDP 7777)을 확인하세요."));
	}
}
