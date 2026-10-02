#include "Game/MobaGameMode.h"
#include "AbilitySystem/MobaAbilityLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaChampionData.h"
#include "Character/MobaCharacter.h"
#include "Combat/MobaCombatLibrary.h"
#include "Components/SplineComponent.h"
#include "EngineUtils.h"
#include "Game/MobaGameState.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Map/MobaLanePath.h"
#include "Player/MobaPlayerController.h"
#include "Player/MobaPlayerState.h"
#include "Structures/MobaStructure.h"
#include "TimerManager.h"
#include "UI/MobaHUD.h"

AMobaGameMode::AMobaGameMode()
{
	DefaultPawnClass = AMobaCharacter::StaticClass();
	PlayerControllerClass = AMobaPlayerController::StaticClass();
	PlayerStateClass = AMobaPlayerState::StaticClass();
	GameStateClass = AMobaGameState::StaticClass();
	HUDClass = AMobaHUD::StaticClass();

	MinionClass = AMobaMinion::StaticClass();
	ActiveLanes.Add(EMobaLane::Mid);

	MeleeMinion.Stats.MaxHealth = 477.0f;
	MeleeMinion.Stats.MaxMana = 0.0f;
	MeleeMinion.Stats.AttackDamage = 12.0f;
	MeleeMinion.Stats.Armor = 0.0f;
	MeleeMinion.Stats.MagicResist = 0.0f;
	MeleeMinion.Stats.AttackSpeed = 1.25f;
	MeleeMinion.Stats.MoveSpeed = 325.0f;
	MeleeMinion.Stats.AttackRange = 110.0f;
	MeleeMinion.bRanged = false;
	MeleeMinion.GoldReward = 21.0f;
	MeleeMinion.ExperienceReward = 60.0f;

	CasterMinion.Stats.MaxHealth = 296.0f;
	CasterMinion.Stats.MaxMana = 0.0f;
	CasterMinion.Stats.AttackDamage = 23.0f;
	CasterMinion.Stats.Armor = 0.0f;
	CasterMinion.Stats.MagicResist = 0.0f;
	CasterMinion.Stats.AttackSpeed = 0.67f;
	CasterMinion.Stats.MoveSpeed = 325.0f;
	CasterMinion.Stats.AttackRange = 550.0f;
	CasterMinion.bRanged = true;
	CasterMinion.ProjectileSpeed = 1000.0f;
	CasterMinion.GoldReward = 14.0f;
	CasterMinion.ExperienceReward = 30.0f;
}

void AMobaGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	RequiredPlayers = FMath::Clamp(UGameplayStatics::GetIntOption(Options, TEXT("Players"), RequiredPlayers), 1, MaxPlayers);
}

void AMobaGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty())
	{
		return;
	}

	// 여기서 채운 문구는 접속하려던 쪽의 메뉴에 그대로 표시된다.
	if (MobaCombat::IsMatchOver(this))
	{
		ErrorMessage = TEXT("이미 끝난 경기입니다.");
	}
	else if (GetNumPlayers() >= MaxPlayers)
	{
		ErrorMessage = TEXT("방이 가득 찼습니다.");
	}
}

void AMobaGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	TryStartMatch();
}

void AMobaGameMode::Logout(AController* Exiting)
{
	const AMobaPlayerState* LeavingState = Exiting ? Exiting->GetPlayerState<AMobaPlayerState>() : nullptr;
	const EMobaTeam LeavingTeam = LeavingState ? LeavingState->GetTeam() : EMobaTeam::None;

	Super::Logout(Exiting);

	const AMobaGameState* MobaGameState = GetGameState<AMobaGameState>();
	if (!MobaGameState || !MobaGameState->HasMatchStarted() || MobaGameState->IsMatchOver() || LeavingTeam == EMobaTeam::None || GetWorld()->bIsTearingDown)
	{
		return;
	}

	// 나간 플레이어의 팀에 아무도 남지 않았으면 상대 팀의 승리로 끝낸다.
	for (const APlayerState* PlayerState : MobaGameState->PlayerArray)
	{
		const AMobaPlayerState* MobaPlayerState = Cast<AMobaPlayerState>(PlayerState);
		if (MobaPlayerState && MobaPlayerState != LeavingState && MobaPlayerState->GetTeam() == LeavingTeam)
		{
			return;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("플레이어가 나가서 경기를 끝냅니다."));
	EndMatch(LeavingTeam == EMobaTeam::Blue ? EMobaTeam::Red : EMobaTeam::Blue, true);
}

void AMobaGameMode::StartPlay()
{
	Super::StartPlay();

	TryStartMatch();
}

void AMobaGameMode::TryStartMatch()
{
	AMobaGameState* MobaGameState = GetGameState<AMobaGameState>();
	if (!MobaGameState || MobaGameState->HasMatchStarted() || !GetWorld()->HasBegunPlay() || GetNumPlayers() < RequiredPlayers)
	{
		return;
	}

	MobaGameState->StartMatch();

	GetWorldTimerManager().SetTimer(WaveTimer, this, &AMobaGameMode::SpawnWave, WaveInterval, true, FirstWaveDelay);
	GetWorldTimerManager().SetTimer(PassiveGoldTimer, this, &AMobaGameMode::GrantPassiveGold, 1.0f, true);

	UE_LOG(LogTemp, Log, TEXT("경기 시작 (플레이어 %d명)."), GetNumPlayers());
}

void AMobaGameMode::SpawnWave()
{
	if (MobaCombat::IsMatchOver(this))
	{
		return;
	}

	for (TActorIterator<AMobaLanePath> It(GetWorld()); It; ++It)
	{
		AMobaLanePath* LanePath = *It;
		if (!ActiveLanes.Contains(LanePath->GetLane()))
		{
			continue;
		}

		// 근접이 앞장서고 원거리가 뒤따르도록 한 마리씩 간격을 두고 내보낸다.
		for (int32 Index = 0; Index < MinionsPerType * 2; ++Index)
		{
			const EMobaMinionType Type = Index < MinionsPerType ? EMobaMinionType::Melee : EMobaMinionType::Caster;
			for (const EMobaTeam Team : { EMobaTeam::Blue, EMobaTeam::Red })
			{
				FTimerHandle Handle;
				GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(LanePath, [this, LanePath, Team, Type]()
				{
					SpawnMinion(LanePath, Team, Type);
				}), FMath::Max(SpawnStagger * Index, 0.01f), false);
			}
		}
	}
}

void AMobaGameMode::SpawnMinion(AMobaLanePath* LanePath, EMobaTeam Team, EMobaMinionType Type)
{
	if (!MinionClass || !LanePath || MobaCombat::IsMatchOver(this))
	{
		return;
	}

	// 스플라인은 블루 넥서스에서 시작해 레드 넥서스에서 끝난다.
	const USplineComponent* Spline = LanePath->GetSpline();
	const float Distance = Team == EMobaTeam::Blue ? SpawnDistanceFromNexus : Spline->GetSplineLength() - SpawnDistanceFromNexus;
	const FVector Location = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World) + FVector(0.0, 0.0, 60.0);
	const FVector Forward = Spline->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World) * (Team == EMobaTeam::Blue ? 1.0 : -1.0);

	const FTransform SpawnTransform(Forward.Rotation(), Location);
	if (AMobaMinion* Minion = GetWorld()->SpawnActorDeferred<AMobaMinion>(MinionClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
	{
		Minion->InitMinion(Team, Type, LanePath, Type == EMobaMinionType::Melee ? MeleeMinion : CasterMinion);
		Minion->FinishSpawning(SpawnTransform);
	}
}

void AMobaGameMode::GrantPassiveGold()
{
	if (MobaCombat::IsMatchOver(this) || !GameState)
	{
		return;
	}

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (AMobaPlayerState* MobaPlayerState = Cast<AMobaPlayerState>(PlayerState))
		{
			MobaPlayerState->AddGold(PassiveGoldPerSecond);
		}
	}
}

void AMobaGameMode::GrantGoldToTeam(EMobaTeam Team, float Amount)
{
	if (!GameState || Amount <= 0.0f)
	{
		return;
	}

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		AMobaPlayerState* MobaPlayerState = Cast<AMobaPlayerState>(PlayerState);
		if (MobaPlayerState && MobaPlayerState->GetTeam() == Team)
		{
			MobaPlayerState->AddGold(Amount);
		}
	}
}

void AMobaGameMode::HandleMinionKilled(AMobaMinion* Minion, AActor* Killer)
{
	if (!Minion || !GameState)
	{
		return;
	}

	// 골드는 막타를 친 챔피언만 받는다.
	const AMobaCharacter* KillerChampion = Cast<AMobaCharacter>(Killer);
	if (AMobaPlayerState* KillerState = KillerChampion ? KillerChampion->GetPlayerState<AMobaPlayerState>() : nullptr)
	{
		KillerState->AddGold(Minion->GetConfig().GoldReward);
	}

	// 경험치는 근처에 살아 있는 적 챔피언들이 똑같이 나눠 받는다.
	TArray<AMobaPlayerState*> Receivers;
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		AMobaPlayerState* MobaPlayerState = Cast<AMobaPlayerState>(PlayerState);
		const AMobaCharacter* Champion = MobaPlayerState ? Cast<AMobaCharacter>(MobaPlayerState->GetPawn()) : nullptr;
		if (Champion && !Champion->IsDead() && Champion->GetTeam() != Minion->GetTeam()
			&& FVector::Dist2D(Champion->GetActorLocation(), Minion->GetActorLocation()) <= ExperienceShareRange)
		{
			Receivers.Add(MobaPlayerState);
		}
	}

	for (AMobaPlayerState* Receiver : Receivers)
	{
		Receiver->AddExperience(Minion->GetConfig().ExperienceReward / Receivers.Num());
	}
}

void AMobaGameMode::HandleStructureDestroyed(AMobaStructure* Structure, AActor* Killer)
{
	if (!Structure)
	{
		return;
	}

	const EMobaTeam WinningSide = Structure->GetTeam() == EMobaTeam::Blue ? EMobaTeam::Red : EMobaTeam::Blue;
	GrantGoldToTeam(WinningSide, Structure->GetGoldReward());

	if (Structure->GetStructureType() == EMobaStructureType::Nexus)
	{
		EndMatch(WinningSide);
	}
}

void AMobaGameMode::EndMatch(EMobaTeam WinningTeam, bool bForfeit)
{
	AMobaGameState* MobaGameState = GetGameState<AMobaGameState>();
	if (!MobaGameState || MobaGameState->IsMatchOver())
	{
		return;
	}

	MobaGameState->SetMatchResult(WinningTeam, bForfeit);

	UE_LOG(LogTemp, Log, TEXT("경기 종료. 승리 팀: %s%s"), *MobaTeam::GetTeamTag(WinningTeam).ToString(), bForfeit ? TEXT(" (상대 이탈)") : TEXT(""));

	GetWorldTimerManager().ClearTimer(WaveTimer);
	GetWorldTimerManager().ClearTimer(PassiveGoldTimer);

	for (TActorIterator<AMobaMinion> It(GetWorld()); It; ++It)
	{
		It->StopActing();
	}

	// 챔피언은 긴 기절을 걸어 이동·공격·스킬을 모두 막는다.
	for (TActorIterator<AMobaCharacter> It(GetWorld()); It; ++It)
	{
		// 나가는 중인 플레이어의 챔피언은 이미 어빌리티 시스템과 끊겨 있을 수 있다.
		if (UAbilitySystemComponent* AbilitySystem = It->GetAbilitySystemComponent())
		{
			MobaAbility::ApplySpecToTarget(MobaAbility::MakeStunSpec(AbilitySystem, *It, 100000.0f), *It);
		}
	}
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

UMobaChampionData* AMobaGameMode::GetDefaultChampion() const
{
	return DefaultChampion.LoadSynchronous();
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
		KillerState->AddGold(ChampionKillGold);
		KillerState->AddExperience(ChampionKillExperience);
	}

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateWeakLambda(Victim, [this, Victim]()
	{
		RespawnChampion(Victim);
	}), RespawnDelay, false);
}

void AMobaGameMode::RespawnChampion(AMobaCharacter* Champion)
{
	if (!Champion || MobaCombat::IsMatchOver(this))
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
