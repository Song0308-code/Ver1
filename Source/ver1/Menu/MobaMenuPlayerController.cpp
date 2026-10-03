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
	UGameplayStatics::OpenLevel(this, FName(*GameMap), true, FString::Printf(TEXT("listen?Players=%d%s"), HostRequiredPlayers, *MakeChampionOption()));
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
	const FString Selected = GetSelectedChampion();
	return Selected.IsEmpty() ? FString() : FString::Printf(TEXT("?Champion=%s"), *Selected);
}

void AMobaMenuPlayerController::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
