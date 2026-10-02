#include "Menu/MobaMenuGameMode.h"
#include "Menu/MobaMenuPlayerController.h"

AMobaMenuGameMode::AMobaMenuGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
	PlayerControllerClass = AMobaMenuPlayerController::StaticClass();
}
