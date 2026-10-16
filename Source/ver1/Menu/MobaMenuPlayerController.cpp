#include "Menu/MobaMenuPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Character/MobaChampionData.h"
#include "Game/MobaGameInstance.h"
#include "Game/MobaGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Menu/SMobaMainMenu.h"

AMobaMenuPlayerController::AMobaMenuPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
}

void AMobaMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	// 접속 실패나 끊김으로 돌아온 경우 그 이유를 보여 준다.
	FText InitialStatus;
	if (UMobaGameInstance* MobaGameInstance = GetGameInstance<UMobaGameInstance>())
	{
		InitialStatus = MobaGameInstance->ConsumePendingMessage();
	}

	MenuWidget = SNew(SMobaMainMenu)
		.OwnerController(this)
		.InitialStatus(InitialStatus);
	GEngine->GameViewport->AddViewportWidgetContent(MenuWidget.ToSharedRef());

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MenuWidget);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	// 실행 인자 -MobaHost는 바로 방을 만들고, -MobaJoin=주소는 바로 접속한다(바로가기용).
	// 배포용 빌드는 명령줄의 맵·주소를 무시하므로 이 방법으로 시작한다. 메뉴로 돌아왔을 때 반복하지 않도록 한 번만 한다.
	static bool bAutoActionDone = false;
	if (!bAutoActionDone)
	{
		bAutoActionDone = true;

		FString JoinAddress;
		if (FParse::Param(FCommandLine::Get(), TEXT("MobaHost")))
		{
			HostGame();
		}
		else if (FParse::Value(FCommandLine::Get(), TEXT("MobaJoin="), JoinAddress))
		{
			JoinGame(JoinAddress);
		}
	}
}

void AMobaMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MenuWidget.IsValid())
	{
		if (GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->RemoveViewportWidgetContent(MenuWidget.ToSharedRef());
		}
		MenuWidget.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void AMobaMenuPlayerController::HostGame()
{
	UGameplayStatics::OpenLevel(this, FName(*GameMap), true, FString::Printf(TEXT("listen?Players=%d%s"), GetHostPlayers(), *MakeChampionOption()));
}

void AMobaMenuPlayerController::StartPractice()
{
	UGameplayStatics::OpenLevel(this, FName(*GameMap), true, MakeChampionOption().RightChop(1));
}

bool AMobaMenuPlayerController::JoinGame(const FString& Address)
{
	// 주소 뒤에 옵션이 붙어 들어오지 않도록 주소 부분만 쓴다.
	FString Host = Address.TrimStartAndEnd();
	int32 OptionIndex = INDEX_NONE;
	if (Host.FindChar(TEXT('?'), OptionIndex))
	{
		Host.LeftInline(OptionIndex);
	}

	if (Host.IsEmpty() || Host.Contains(TEXT(" ")) || Host.Contains(TEXT("/")))
	{
		return false;
	}

	ClientTravel(Host + MakeChampionOption(), TRAVEL_Absolute);
	return true;
}

TArray<FMobaChampionChoice> AMobaMenuPlayerController::GetChampionChoices() const
{
	TArray<FMobaChampionChoice> Choices;
	for (const FSoftObjectPath& Path : GetDefault<AMobaGameMode>()->GetChampionRoster())
	{
		FMobaChampionChoice& Choice = Choices.AddDefaulted_GetRef();
		Choice.AssetName = Path.GetAssetName();

		const UMobaChampionData* Data = Cast<UMobaChampionData>(Path.TryLoad());
		Choice.DisplayName = Data && !Data->DisplayName.IsEmpty() ? Data->DisplayName : FText::FromString(Choice.AssetName);
	}
	return Choices;
}

FString AMobaMenuPlayerController::GetSelectedChampion() const
{
	const UMobaGameInstance* MobaGameInstance = GetGameInstance<UMobaGameInstance>();
	return MobaGameInstance ? MobaGameInstance->GetSelectedChampion() : FString();
}

void AMobaMenuPlayerController::SelectChampion(const FString& AssetName)
{
	if (UMobaGameInstance* MobaGameInstance = GetGameInstance<UMobaGameInstance>())
	{
		MobaGameInstance->SetSelectedChampion(AssetName);
	}
}

FString AMobaMenuPlayerController::MakeChampionOption() const
{
	const UMobaGameInstance* MobaGameInstance = GetGameInstance<UMobaGameInstance>();
	return MobaGameInstance ? MobaGameInstance->MakeLoadoutOptions() : FString();
}

EMobaSummonerSpell AMobaMenuPlayerController::GetSelectedSpell(int32 Index) const
{
	const UMobaGameInstance* MobaGameInstance = GetGameInstance<UMobaGameInstance>();
	return MobaGameInstance ? MobaGameInstance->GetSelectedSpell(Index) : (Index == 0 ? EMobaSummonerSpell::Flash : EMobaSummonerSpell::Ignite);
}

int32 AMobaMenuPlayerController::GetHostPlayers() const
{
	const UMobaGameInstance* MobaGameInstance = GetGameInstance<UMobaGameInstance>();
	return MobaGameInstance ? MobaGameInstance->GetHostPlayers() : 2;
}

void AMobaMenuPlayerController::ToggleHostPlayers()
{
	if (UMobaGameInstance* MobaGameInstance = GetGameInstance<UMobaGameInstance>())
	{
		MobaGameInstance->SetHostPlayers(MobaGameInstance->GetHostPlayers() >= 4 ? 2 : 4);
	}
}

void AMobaMenuPlayerController::CycleSpell(int32 Index)
{
	UMobaGameInstance* MobaGameInstance = GetGameInstance<UMobaGameInstance>();
	if (!MobaGameInstance)
	{
		return;
	}

	const int32 Count = static_cast<int32>(EMobaSummonerSpell::Count);
	const EMobaSummonerSpell Other = MobaGameInstance->GetSelectedSpell(1 - Index);
	int32 Next = static_cast<int32>(MobaGameInstance->GetSelectedSpell(Index));
	do
	{
		Next = (Next + 1) % Count;
	}
	while (static_cast<EMobaSummonerSpell>(Next) == Other);

	MobaGameInstance->SetSelectedSpell(Index, static_cast<EMobaSummonerSpell>(Next));
}

void AMobaMenuPlayerController::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
