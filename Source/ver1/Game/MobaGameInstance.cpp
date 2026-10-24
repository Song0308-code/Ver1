#include "Game/MobaGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "Game/MobaGameMode.h"
#include "Game/MobaProfileSave.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void UMobaGameInstance::Init()
{
	Super::Init();

	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &UMobaGameInstance::RecordNetworkFailure);
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &UMobaGameInstance::RecordTravelFailure);
	}

	LoadProfile();

#if !UE_BUILD_SHIPPING
	// 검증용: -MobaChampion=에셋이름, -MobaSpells=Flash,Smite 로 메뉴에서 고른 것처럼 정하고 저장한다.
	FString Value;
	if (FParse::Value(FCommandLine::Get(), TEXT("MobaChampion="), Value))
	{
		SetSelectedChampion(Value);
	}
	EMobaSummonerSpell SpellD;
	EMobaSummonerSpell SpellF;
	if (FParse::Value(FCommandLine::Get(), TEXT("MobaSpells="), Value, false) && AMobaGameMode::ParseSpellsOption(Value, SpellD, SpellF))
	{
		SetSelectedSpell(0, SpellD);
		SetSelectedSpell(1, SpellF);
	}
#endif
}

void UMobaGameInstance::SetSelectedChampion(const FString& InName)
{
	if (SelectedChampion != InName)
	{
		SelectedChampion = InName;
		SaveProfile();
	}
}

void UMobaGameInstance::SetSelectedSpell(int32 Index, EMobaSummonerSpell Spell)
{
	if (Spell >= EMobaSummonerSpell::Count || (Index != 0 && Index != 1))
	{
		return;
	}

	EMobaSummonerSpell& Mine = Index == 0 ? SelectedSpellD : SelectedSpellF;
	EMobaSummonerSpell& Other = Index == 0 ? SelectedSpellF : SelectedSpellD;
	if (Mine == Spell)
	{
		return;
	}
	if (Other == Spell)
	{
		Other = Mine;
	}
	Mine = Spell;
	SaveProfile();
}

void UMobaGameInstance::SetHostPlayers(int32 Players)
{
	Players = Players >= 4 ? 4 : 2;
	if (HostPlayers != Players)
	{
		HostPlayers = Players;
		SaveProfile();
	}
}

FString UMobaGameInstance::MakeLoadoutOptions() const
{
	FString Options;
	if (!SelectedChampion.IsEmpty())
	{
		Options += FString::Printf(TEXT("?Champion=%s"), *SelectedChampion);
	}
	Options += FString::Printf(TEXT("?Spells=%s"), *AMobaGameMode::MakeSpellsOption(SelectedSpellD, SelectedSpellF));
	return Options;
}

void UMobaGameInstance::SetSmartCastEnabled(bool bEnabled)
{
	bSmartCast = bEnabled;
	SaveProfile();
}

void UMobaGameInstance::SetCameraLockDefault(bool bLocked)
{
	bCameraLockDefault = bLocked;
	SaveProfile();
}

void UMobaGameInstance::SetCameraSpeedIndex(int32 Index)
{
	CameraSpeedIndex = FMath::Clamp(Index, 0, NumCameraSpeeds - 1);
	SaveProfile();
}

float UMobaGameInstance::GetCameraPanSpeed(int32 Index)
{
	static const float Speeds[NumCameraSpeeds] = { 1800.0f, 3000.0f, 4500.0f };
	return Speeds[FMath::Clamp(Index, 0, NumCameraSpeeds - 1)];
}

FString UMobaGameInstance::GetCameraSpeedName(int32 Index)
{
	static const TCHAR* Names[NumCameraSpeeds] = { TEXT("느림"), TEXT("보통"), TEXT("빠름") };
	return Names[FMath::Clamp(Index, 0, NumCameraSpeeds - 1)];
}

void UMobaGameInstance::LoadProfile()
{
	const UMobaProfileSave* Profile = Cast<UMobaProfileSave>(UGameplayStatics::LoadGameFromSlot(UMobaProfileSave::SlotName, 0));
	if (!Profile)
	{
		return;
	}

	SelectedChampion = Profile->ChampionAssetName;
	HostPlayers = Profile->HostPlayers >= 4 ? 4 : 2;
	bSmartCast = Profile->bSmartCast;
	bCameraLockDefault = Profile->bCameraLockDefault;
	CameraSpeedIndex = FMath::Clamp(Profile->CameraSpeedIndex, 0, NumCameraSpeeds - 1);
	if (Profile->SpellD < EMobaSummonerSpell::Count && Profile->SpellF < EMobaSummonerSpell::Count && Profile->SpellD != Profile->SpellF)
	{
		SelectedSpellD = Profile->SpellD;
		SelectedSpellF = Profile->SpellF;
	}
	UE_LOG(LogTemp, Log, TEXT("저장된 구성 불러옴: 챔피언 %s, 주문 %s"), *SelectedChampion, *AMobaGameMode::MakeSpellsOption(SelectedSpellD, SelectedSpellF));
}

void UMobaGameInstance::SaveProfile() const
{
	UMobaProfileSave* Profile = Cast<UMobaProfileSave>(UGameplayStatics::CreateSaveGameObject(UMobaProfileSave::StaticClass()));
	if (!Profile)
	{
		return;
	}

	Profile->ChampionAssetName = SelectedChampion;
	Profile->SpellD = SelectedSpellD;
	Profile->SpellF = SelectedSpellF;
	Profile->HostPlayers = HostPlayers;
	Profile->bSmartCast = bSmartCast;
	Profile->bCameraLockDefault = bCameraLockDefault;
	Profile->CameraSpeedIndex = CameraSpeedIndex;
	UGameplayStatics::SaveGameToSlot(Profile, UMobaProfileSave::SlotName, 0);
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
