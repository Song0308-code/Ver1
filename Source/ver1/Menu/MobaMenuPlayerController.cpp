#include "Menu/MobaMenuPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Game/MobaGameInstance.h"
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
	UGameplayStatics::OpenLevel(this, FName(*GameMap), true, FString::Printf(TEXT("listen?Players=%d"), HostRequiredPlayers));
}

void AMobaMenuPlayerController::StartPractice()
{
	UGameplayStatics::OpenLevel(this, FName(*GameMap), true);
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

	ClientTravel(Host, TRAVEL_Absolute);
	return true;
}

void AMobaMenuPlayerController::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
