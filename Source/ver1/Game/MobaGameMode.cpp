#include "Game/MobaGameMode.h"
#include "Character/MobaCharacter.h"
#include "EngineUtils.h"
#include "Game/MobaGameState.h"
#include "GameFramework/PlayerStart.h"
#include "Player/MobaPlayerController.h"
#include "Player/MobaPlayerState.h"
#include "TimerManager.h"
#include "UI/MobaHUD.h"

AMobaGameMode::AMobaGameMode()
{
	DefaultPawnClass = AMobaCharacter::StaticClass();
	PlayerControllerClass = AMobaPlayerController::StaticClass();
	PlayerStateClass = AMobaPlayerState::StaticClass();
	GameStateClass = AMobaGameState::StaticClass();
	HUDClass = AMobaHUD::StaticClass();
}

FString AMobaGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	// Super가 시작 지점을 고르기 전에 팀이 정해져 있어야 한다.
	if (AMobaPlayerState* MobaPlayerState = NewPlayerController ? NewPlayerController->GetPlayerState<AMobaPlayerState>() : nullptr)
	{
		MobaPlayerState->SetTeam(PickTeamForNewPlayer());
	}

	return Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
}

AActor* AMobaGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	const AMobaPlayerState* MobaPlayerState = Player ? Player->GetPlayerState<AMobaPlayerState>() : nullptr;
	const FName TeamTag = MobaTeam::GetTeamTag(MobaPlayerState ? MobaPlayerState->GetTeam() : EMobaTeam::None);

	if (!TeamTag.IsNone())
	{
		TArray<APlayerStart*> TeamStarts;
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			if (It->PlayerStartTag == TeamTag)
			{
				TeamStarts.Add(*It);
			}
		}

		if (TeamStarts.Num() > 0)
		{
			return TeamStarts[FMath::RandRange(0, TeamStarts.Num() - 1)];
		}
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

EMobaTeam AMobaGameMode::PickTeamForNewPlayer() const
{
	int32 BlueCount = 0;
	int32 RedCount = 0;

	if (GameState)
	{
		for (const APlayerState* PlayerState : GameState->PlayerArray)
		{
			if (const AMobaPlayerState* MobaPlayerState = Cast<AMobaPlayerState>(PlayerState))
			{
				BlueCount += MobaPlayerState->GetTeam() == EMobaTeam::Blue ? 1 : 0;
				RedCount += MobaPlayerState->GetTeam() == EMobaTeam::Red ? 1 : 0;
			}
		}
	}

	return RedCount < BlueCount ? EMobaTeam::Red : EMobaTeam::Blue;
}

void AMobaGameMode::HandleChampionKilled(AMobaCharacter* Victim, AActor* Killer)
{
	if (!Victim)
	{
		return;
	}

	if (AMobaPlayerState* VictimState = Victim->GetPlayerState<AMobaPlayerState>())
	{
		VictimState->AddDeath();
		VictimState->SetRespawnServerTime(GameState ? GameState->GetServerWorldTimeSeconds() + RespawnDelay : 0.0f);
	}

	const APawn* KillerPawn = Cast<APawn>(Killer);
	if (AMobaPlayerState* KillerState = KillerPawn ? KillerPawn->GetPlayerState<AMobaPlayerState>() : nullptr)
	{
		KillerState->AddKill();
	}

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateWeakLambda(Victim, [this, Victim]()
	{
		RespawnChampion(Victim);
	}), RespawnDelay, false);
}

void AMobaGameMode::RespawnChampion(AMobaCharacter* Champion)
{
	if (!Champion)
	{
		return;
	}

	const AActor* StartSpot = ChoosePlayerStart(Champion->GetController());
	const FVector Location = StartSpot ? StartSpot->GetActorLocation() : Champion->GetActorLocation();
	const FRotator Rotation = StartSpot ? StartSpot->GetActorRotation() : Champion->GetActorRotation();

	if (AMobaPlayerState* PlayerState = Champion->GetPlayerState<AMobaPlayerState>())
	{
		PlayerState->SetRespawnServerTime(0.0f);
	}

	Champion->Respawn(Location, Rotation);
}
