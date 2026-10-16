#include "Game/MobaGameMode.h"
#include "AbilitySystem/MobaAbilityLibrary.h"
#include "AbilitySystemComponent.h"
#include "Combat/MobaCombatEvents.h"
#include "Character/MobaChampionData.h"
#include "Character/MobaCharacter.h"
#include "Combat/MobaCombatLibrary.h"
#include "Components/SplineComponent.h"
#include "EngineUtils.h"
#include "Game/MobaGameState.h"
#include "Game/MobaGameplayTags.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Map/MobaJungleCamp.h"
#include "Map/MobaLanePath.h"
#include "Player/MobaPlayerController.h"
#include "Player/MobaPlayerState.h"
#include "Structures/MobaStructure.h"
#include "TimerManager.h"
#include "UI/MobaHUD.h"
#include "Units/MobaMonster.h"
#include "Vision/MobaWard.h"

AMobaGameMode::AMobaGameMode()
{
	DefaultPawnClass = AMobaCharacter::StaticClass();
	PlayerControllerClass = AMobaPlayerController::StaticClass();
	PlayerStateClass = AMobaPlayerState::StaticClass();
	GameStateClass = AMobaGameState::StaticClass();
	HUDClass = AMobaHUD::StaticClass();

	MinionClass = AMobaMinion::StaticClass();
	ActiveLanes.Add(EMobaLane::Top);
	ActiveLanes.Add(EMobaLane::Mid);
	ActiveLanes.Add(EMobaLane::Bot);

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

	SuperMinion.Stats.MaxHealth = 1500.0f;
	SuperMinion.Stats.MaxMana = 0.0f;
	SuperMinion.Stats.AttackDamage = 150.0f;
	SuperMinion.Stats.Armor = 30.0f;
	SuperMinion.Stats.MagicResist = 0.0f;
	SuperMinion.Stats.AttackSpeed = 0.85f;
	SuperMinion.Stats.MoveSpeed = 325.0f;
	SuperMinion.Stats.AttackRange = 170.0f;
	SuperMinion.bRanged = false;
	SuperMinion.GoldReward = 60.0f;
	SuperMinion.ExperienceReward = 95.0f;

	// 임시 아트: Epic 마네킹을 작게 줄여 쓴다. 근접은 Manny, 원거리는 Quinn, 슈퍼 미니언은 큰 Manny.
	const TCHAR* AnimRoot = TEXT("/Game/Characters/Mannequins/Anims/");
	const auto AnimPath = [AnimRoot](const TCHAR* Relative, const TCHAR* Name)
	{
		return FSoftObjectPath(FString::Printf(TEXT("%s%s/%s.%s"), AnimRoot, Relative, Name, Name));
	};

	FMobaAnimationSet MinionAnimations;
	MinionAnimations.Locomotion = TSoftObjectPtr<UAnimationAsset>(AnimPath(TEXT("Unarmed"), TEXT("BS_Idle_Walk_Run")));
	MinionAnimations.AttackAnimations.Add(TSoftObjectPtr<UAnimSequenceBase>(AnimPath(TEXT("Unarmed/Attack"), TEXT("MM_Attack_01"))));
	MinionAnimations.AttackAnimations.Add(TSoftObjectPtr<UAnimSequenceBase>(AnimPath(TEXT("Unarmed/Attack"), TEXT("MM_Attack_02"))));
	MinionAnimations.DeathAnimation = TSoftObjectPtr<UAnimSequenceBase>(AnimPath(TEXT("Death"), TEXT("MM_Death_Back_01")));

	// 작게 줄인 만큼 보폭도 짧아지므로 블렌드 스페이스에는 더 빠른 속도로 넣는다.
	MinionAnimations.LocomotionSpeedScale = 1.4f;

	const FSoftObjectPath Manny(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	const FSoftObjectPath Quinn(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));

	MeleeMinion.Mesh = TSoftObjectPtr<USkeletalMesh>(Manny);
	MeleeMinion.MeshScale = 0.58f;
	MeleeMinion.Animations = MinionAnimations;

	CasterMinion.Mesh = TSoftObjectPtr<USkeletalMesh>(Quinn);
	CasterMinion.MeshScale = 0.55f;
	CasterMinion.Animations = MinionAnimations;

	SuperMinion.Mesh = TSoftObjectPtr<USkeletalMesh>(Manny);
	SuperMinion.MeshScale = 0.85f;
	SuperMinion.Animations = MinionAnimations;
	SuperMinion.Animations.LocomotionSpeedScale = 1.1f;
}

void AMobaGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	RequiredPlayers = FMath::Clamp(UGameplayStatics::GetIntOption(Options, TEXT("Players"), RequiredPlayers), 1, MaxPlayers);

	// 메뉴에서 방을 만들면 상대를 기다리며 챔피언을 고르는 로비를 거친다. 에디터에서 바로 실행하거나 혼자 연습하면 거치지 않는다.
	bUseLobby = UGameplayStatics::HasOption(Options, TEXT("Players")) && RequiredPlayers >= 2;

	// 실행 인자 -MobaForceLobby를 주면 에디터에서도 2인 로비를 거친다(로비 시험용). -MobaLobbyPlayers=4처럼 인원을 정할 수 있다.
#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("MobaForceLobby")))
	{
		int32 LobbyPlayers = 2;
		FParse::Value(FCommandLine::Get(), TEXT("MobaLobbyPlayers="), LobbyPlayers);
		RequiredPlayers = FMath::Clamp(LobbyPlayers, 2, MaxPlayers);
		bUseLobby = true;
	}
#endif

	UE_LOG(LogTemp, Log, TEXT("방 설정: 필요 인원 %d, 로비 %s"), RequiredPlayers, bUseLobby ? TEXT("사용") : TEXT("사용 안 함"));
}

bool AMobaGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	// 로비 동안에는 챔피언을 내보내지 않는다. 경기가 시작될 때 한꺼번에 내보낸다.
	const AMobaGameState* MobaGameState = GetGameState<AMobaGameState>();
	if (bUseLobby && MobaGameState && !MobaGameState->HasMatchStarted())
	{
		return false;
	}
	return Super::PlayerCanRestart_Implementation(Player);
}

void AMobaGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty())
	{
		return;
	}

	// 여기서 채운 문구는 접속하려던 쪽의 메뉴에 그대로 표시된다.
	const AMobaGameState* MobaGameState = GetGameState<AMobaGameState>();
	if (MobaCombat::IsMatchOver(this))
	{
		ErrorMessage = TEXT("이미 끝난 경기입니다.");
	}
	else if (bUseLobby && MobaGameState && MobaGameState->HasMatchStarted())
	{
		ErrorMessage = TEXT("이미 시작된 경기입니다.");
	}
	else if (GetNumPlayers() >= (bUseLobby ? RequiredPlayers : MaxPlayers))
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

	// 로비 카운트다운 중에 누가 나가면 멈춘다.
	if (bUseLobby && !GetWorld()->bIsTearingDown)
	{
		TryStartMatch();
	}

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

	if (AMobaGameState* MobaGameState = GetGameState<AMobaGameState>())
	{
		MobaGameState->SetLobbyInfo(bUseLobby, RequiredPlayers);
	}

	if (UMobaCombatEvents* Events = UMobaCombatEvents::Get(this))
	{
		Events->OnDamageDealt.AddUObject(this, &AMobaGameMode::HandleDamageDealt);
	}

	TryStartMatch();
}

void AMobaGameMode::HandleDamageDealt(AActor* DamageInstigator, AActor* Victim, float Damage)
{
	const AMobaCharacter* Attacker = Cast<AMobaCharacter>(DamageInstigator);
	const AMobaCharacter* VictimChampion = Cast<AMobaCharacter>(Victim);
	if (Attacker && VictimChampion && Attacker != VictimChampion)
	{
		AMobaPlayerState* AttackerState = Attacker->GetPlayerState<AMobaPlayerState>();
		AMobaPlayerState* VictimState = VictimChampion->GetPlayerState<AMobaPlayerState>();
		if (AttackerState)
		{
			AttackerState->AddDamageToChampions(Damage);
		}
		if (AttackerState && VictimState)
		{
			RecentChampionDamage.FindOrAdd(VictimState).Add(AttackerState, GetWorld()->GetTimeSeconds());
		}
	}
}

void AMobaGameMode::RequestChampionChange(APlayerController* Player, const FString& AssetName)
{
	const AMobaGameState* MobaGameState = GetGameState<AMobaGameState>();
	AMobaPlayerState* MobaPlayerState = Player ? Player->GetPlayerState<AMobaPlayerState>() : nullptr;
	if (!MobaGameState || MobaGameState->HasMatchStarted() || !MobaPlayerState || MobaPlayerState->IsReady())
	{
		return;
	}

	for (const FSoftObjectPath& Path : ChampionRoster)
	{
		if (Path.GetAssetName().Equals(AssetName, ESearchCase::IgnoreCase))
		{
			if (UMobaChampionData* Champion = Cast<UMobaChampionData>(Path.TryLoad()))
			{
				MobaPlayerState->SetChampion(Champion);
			}
			return;
		}
	}
}

bool AMobaGameMode::ParseSpellsOption(const FString& Value, EMobaSummonerSpell& OutD, EMobaSummonerSpell& OutF)
{
	TArray<FString> Names;
	Value.ParseIntoArray(Names, TEXT(","));
	if (Names.Num() != 2)
	{
		return false;
	}

	const UEnum* SpellEnum = StaticEnum<EMobaSummonerSpell>();
	int64 Values[2];
	for (int32 Index = 0; Index < 2; ++Index)
	{
		Values[Index] = INDEX_NONE;
		const FString Name = Names[Index].TrimStartAndEnd();
		for (int32 EnumIndex = 0; EnumIndex < static_cast<int32>(EMobaSummonerSpell::Count); ++EnumIndex)
		{
			if (SpellEnum->GetNameStringByValue(EnumIndex).Equals(Name, ESearchCase::IgnoreCase))
			{
				Values[Index] = EnumIndex;
			}
		}
		if (Values[Index] == INDEX_NONE)
		{
			return false;
		}
	}

	if (Values[0] == Values[1])
	{
		return false;
	}

	OutD = static_cast<EMobaSummonerSpell>(Values[0]);
	OutF = static_cast<EMobaSummonerSpell>(Values[1]);
	return true;
}

FString AMobaGameMode::MakeSpellsOption(EMobaSummonerSpell SpellD, EMobaSummonerSpell SpellF)
{
	const UEnum* SpellEnum = StaticEnum<EMobaSummonerSpell>();
	return FString::Printf(TEXT("%s,%s"), *SpellEnum->GetNameStringByValue(static_cast<int64>(SpellD)), *SpellEnum->GetNameStringByValue(static_cast<int64>(SpellF)));
}

void AMobaGameMode::RequestSummonerSpell(APlayerController* Player, int32 Index, EMobaSummonerSpell Spell)
{
	const AMobaGameState* MobaGameState = GetGameState<AMobaGameState>();
	AMobaPlayerState* MobaPlayerState = Player ? Player->GetPlayerState<AMobaPlayerState>() : nullptr;
	if (!MobaGameState || MobaGameState->HasMatchStarted() || !MobaPlayerState || MobaPlayerState->IsReady() || (Index != 0 && Index != 1))
	{
		return;
	}

	MobaPlayerState->SetSummonerSpell(Index, Spell);
}

void AMobaGameMode::RequestReady(APlayerController* Player, bool bReady)
{
	const AMobaGameState* MobaGameState = GetGameState<AMobaGameState>();
	AMobaPlayerState* MobaPlayerState = Player ? Player->GetPlayerState<AMobaPlayerState>() : nullptr;
	if (!MobaGameState || MobaGameState->HasMatchStarted() || !MobaPlayerState)
	{
		return;
	}

	MobaPlayerState->SetReady(bReady);
	TryStartMatch();
}

bool AMobaGameMode::AreAllPlayersReady() const
{
	if (!GameState)
	{
		return false;
	}

	for (const APlayerState* PlayerState : GameState->PlayerArray)
	{
		const AMobaPlayerState* MobaPlayerState = Cast<AMobaPlayerState>(PlayerState);
		if (MobaPlayerState && !MobaPlayerState->IsReady())
		{
			return false;
		}
	}
	return true;
}

void AMobaGameMode::TryStartMatch()
{
	AMobaGameState* MobaGameState = GetGameState<AMobaGameState>();
	if (!MobaGameState || MobaGameState->HasMatchStarted() || !GetWorld()->HasBegunPlay())
	{
		return;
	}

	const bool bEnoughPlayers = GetNumPlayers() >= RequiredPlayers;
	if (!bUseLobby)
	{
		if (bEnoughPlayers)
		{
			StartMatchNow();
		}
		return;
	}

	// 로비: 인원이 차고 모두 준비되면 카운트다운을 시작하고, 그 사이 누가 준비를 풀거나 나가면 멈춘다.
	const bool bCanStart = bEnoughPlayers && AreAllPlayersReady();
	const bool bCountingDown = GetWorldTimerManager().IsTimerActive(CountdownTimer);
	if (bCanStart && !bCountingDown)
	{
		GetWorldTimerManager().SetTimer(CountdownTimer, this, &AMobaGameMode::FinishCountdown, StartCountdown, false);
		MobaGameState->SetCountdownEndServerTime(static_cast<float>(MobaGameState->GetServerWorldTimeSeconds()) + StartCountdown);
	}
	else if (!bCanStart && bCountingDown)
	{
		GetWorldTimerManager().ClearTimer(CountdownTimer);
		MobaGameState->SetCountdownEndServerTime(0.0f);
	}
}

void AMobaGameMode::FinishCountdown()
{
	AMobaGameState* MobaGameState = GetGameState<AMobaGameState>();
	if (!MobaGameState || MobaGameState->HasMatchStarted())
	{
		return;
	}

	MobaGameState->SetCountdownEndServerTime(0.0f);
	if (GetNumPlayers() >= RequiredPlayers && AreAllPlayersReady())
	{
		StartMatchNow();
	}
}

void AMobaGameMode::StartMatchNow()
{
	AMobaGameState* MobaGameState = GetGameState<AMobaGameState>();
	if (!MobaGameState || MobaGameState->HasMatchStarted())
	{
		return;
	}

	MobaGameState->StartMatch();

	// 로비 동안 내보내지 않았던 챔피언을 지금 내보낸다.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (PlayerController && !PlayerController->GetPawn())
		{
			RestartPlayer(PlayerController);
		}
	}

	GetWorldTimerManager().SetTimer(WaveTimer, this, &AMobaGameMode::SpawnWave, WaveInterval, true, FirstWaveDelay);
	GetWorldTimerManager().SetTimer(PassiveGoldTimer, this, &AMobaGameMode::GrantPassiveGold, 1.0f, true);
	GetWorldTimerManager().SetTimer(PlatingTimer, this, &AMobaGameMode::EndTowerPlating, TowerPlatingDuration, false);

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

		for (const EMobaTeam Team : { EMobaTeam::Blue, EMobaTeam::Red })
		{
			// 근접이 앞장서고, 슈퍼 미니언이 있으면 그 뒤에, 원거리가 맨 뒤에 서도록 한 마리씩 간격을 두고 내보낸다.
			TArray<EMobaMinionType> Order;
			for (int32 Index = 0; Index < MinionsPerType; ++Index)
			{
				Order.Add(EMobaMinionType::Melee);
			}
			for (int32 Index = 0; Index < GetSuperMinionCount(Team, LanePath->GetLane()); ++Index)
			{
				Order.Add(EMobaMinionType::Super);
			}
			for (int32 Index = 0; Index < MinionsPerType; ++Index)
			{
				Order.Add(EMobaMinionType::Caster);
			}

			for (int32 Index = 0; Index < Order.Num(); ++Index)
			{
				const EMobaMinionType Type = Order[Index];
				FTimerHandle Handle;
				GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(LanePath, [this, LanePath, Team, Type]()
				{
					SpawnMinion(LanePath, Team, Type);
				}), FMath::Max(SpawnStagger * Index, 0.01f), false);
			}
		}
	}
}

int32 AMobaGameMode::GetSuperMinionCount(EMobaTeam Team, EMobaLane Lane) const
{
	bool bLaneInhibitorDown = false;
	bool bAllInhibitorsDown = true;
	int32 EnemyInhibitors = 0;

	for (TActorIterator<AMobaInhibitor> It(GetWorld()); It; ++It)
	{
		if (It->GetTeam() == Team || It->GetTeam() == EMobaTeam::None)
		{
			continue;
		}

		++EnemyInhibitors;
		if (!It->IsDestroyed())
		{
			bAllInhibitorsDown = false;
		}
		else if (It->GetLane() == Lane)
		{
			bLaneInhibitorDown = true;
		}
	}

	if (EnemyInhibitors == 0)
	{
		return 0;
	}
	return bAllInhibitorsDown ? 2 : (bLaneInhibitorDown ? 1 : 0);
}

const FMobaMinionConfig& AMobaGameMode::GetMinionConfig(EMobaMinionType Type) const
{
	switch (Type)
	{
	case EMobaMinionType::Caster: return CasterMinion;
	case EMobaMinionType::Super: return SuperMinion;
	default: return MeleeMinion;
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
		// 시간이 지날수록 새로 나오는 미니언이 강해지고 보상도 늘어난다.
		FMobaMinionConfig Config = GetMinionConfig(Type);
		const AMobaGameState* MobaGameState = GetGameState<AMobaGameState>();
		const int32 Upgrades = MobaGameState ? GetMinionUpgradeCount(MobaGameState->GetMatchTimeSeconds()) : 0;
		Config.Stats.MaxHealth *= 1.0f + MinionHealthPerUpgrade * Upgrades;
		Config.Stats.AttackDamage *= 1.0f + MinionDamagePerUpgrade * Upgrades;
		Config.GoldReward *= 1.0f + MinionGoldPerUpgrade * Upgrades;

		Minion->InitMinion(Team, Type, LanePath, Config);
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
		KillerState->AddMinionKill();
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

void AMobaGameMode::HandleMonsterKilled(AMobaMonster* Monster, AActor* Killer)
{
	const AMobaCharacter* KillerChampion = Cast<AMobaCharacter>(Killer);
	AMobaPlayerState* KillerState = KillerChampion ? KillerChampion->GetPlayerState<AMobaPlayerState>() : nullptr;
	if (Monster && KillerState)
	{
		KillerState->AddGold(Monster->GetConfig().GoldReward);
		KillerState->AddExperience(Monster->GetConfig().ExperienceReward);
		KillerState->AddMinionKill();
	}
}

void AMobaGameMode::HandleCampCleared(AMobaJungleCamp* Camp, AActor* Killer)
{
	AMobaCharacter* KillerChampion = Cast<AMobaCharacter>(Killer);
	if (!Camp || !KillerChampion)
	{
		return;
	}

	const EMobaTeam Team = KillerChampion->GetTeam();

	// 다시 잡기 전에는 끝나지 않을 만큼 긴 시간. 드래곤 보상은 경기 끝까지 간다.
	const float PermanentDuration = 1000000.0f;

	switch (Camp->GetCampType())
	{
	case EMobaCampType::BlueBuff:
		ApplyBuff(KillerChampion, MobaTags::Buff_Blue, CampBuffDuration, 0.0f, 0.0f);
		break;

	case EMobaCampType::RedBuff:
		ApplyBuff(KillerChampion, MobaTags::Buff_Red, CampBuffDuration, 0.0f, 0.0f);
		break;

	case EMobaCampType::Scuttle:
		AMobaWard::SpawnWard(GetWorld(), Team, Camp->GetActorLocation(), ScuttleVisionDuration);
		break;

	case EMobaCampType::Dragon:
		ApplyBuffToTeam(Team, MobaTags::Buff_Dragon, PermanentDuration, DragonBonusAttackDamage, DragonBonusAbilityPower);
		break;

	case EMobaCampType::Baron:
		ApplyBuffToTeam(Team, MobaTags::Buff_Baron, BaronBuffDuration, BaronBonusAttackDamage, BaronBonusAbilityPower);
		GrantGoldToTeam(Team, BaronTeamGold);
		break;

	default:
		break;
	}

	if (Camp->GetCampType() == EMobaCampType::Dragon || Camp->GetCampType() == EMobaCampType::Baron)
	{
		Announce(FString::Printf(TEXT("%s 팀이 %s 처치"), *MobaTeam::GetTeamDisplayName(Team), *AMobaJungleCamp::GetDefinition(Camp->GetCampType()).DisplayName), Team);
	}

	UE_LOG(LogTemp, Log, TEXT("캠프 정리: %s (%s 팀)"), *AMobaJungleCamp::GetDefinition(Camp->GetCampType()).DisplayName, *MobaTeam::GetTeamTag(Team).ToString());
}

void AMobaGameMode::ApplyBuff(AMobaCharacter* Champion, const FGameplayTag& BuffTag, float Duration, float BonusAttackDamage, float BonusAbilityPower)
{
	UAbilitySystemComponent* AbilitySystem = Champion ? Champion->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystem)
	{
		return;
	}

	// 블루·레드·바론은 다시 받으면 겹치지 않고 시간만 새로 시작한다. 드래곤은 겹친다.
	if (BuffTag != MobaTags::Buff_Dragon)
	{
		AbilitySystem->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(BuffTag));
	}

	MobaAbility::ApplySpecToTarget(MobaAbility::MakeBuffSpec(AbilitySystem, Champion, Duration, BonusAttackDamage, BonusAbilityPower, BuffTag), Champion);
}

void AMobaGameMode::ApplyBuffToTeam(EMobaTeam Team, const FGameplayTag& BuffTag, float Duration, float BonusAttackDamage, float BonusAbilityPower)
{
	for (TActorIterator<AMobaCharacter> It(GetWorld()); It; ++It)
	{
		// 드래곤 보상은 죽어 있는 팀원도 받는다. 나머지는 살아 있는 팀원만 받는다.
		if (It->GetTeam() == Team && (BuffTag == MobaTags::Buff_Dragon || !It->IsDead()))
		{
			ApplyBuff(*It, BuffTag, Duration, BonusAttackDamage, BonusAbilityPower);
		}
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

	Announce(FString::Printf(TEXT("%s 파괴"), *DescribeUnit(Structure)), WinningSide);

	if (Structure->GetStructureType() == EMobaStructureType::Nexus)
	{
		EndMatch(WinningSide);
	}
}

void AMobaGameMode::EndMatch(EMobaTeam WinningTeam, bool bForfeit, bool bSurrender)
{
	AMobaGameState* MobaGameState = GetGameState<AMobaGameState>();
	if (!MobaGameState || MobaGameState->IsMatchOver())
	{
		return;
	}

	MobaGameState->SetMatchResult(WinningTeam, bForfeit, bSurrender);

	UE_LOG(LogTemp, Log, TEXT("경기 종료. 승리 팀: %s%s"), *MobaTeam::GetTeamTag(WinningTeam).ToString(), bForfeit ? TEXT(" (상대 이탈)") : TEXT(""));

	GetWorldTimerManager().ClearTimer(WaveTimer);
	GetWorldTimerManager().ClearTimer(PassiveGoldTimer);

	for (TActorIterator<AMobaMinion> It(GetWorld()); It; ++It)
	{
		It->StopActing();
	}

	for (TActorIterator<AMobaMonster> It(GetWorld()); It; ++It)
	{
		It->StopActing();
	}

	for (TActorIterator<AMobaJungleCamp> It(GetWorld()); It; ++It)
	{
		It->StopSpawning();
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
	// Super가 시작 지점을 고르고 챔피언을 스폰하기 전에 팀과 챔피언이 정해져 있어야 한다.
	if (AMobaPlayerState* MobaPlayerState = NewPlayerController ? NewPlayerController->GetPlayerState<AMobaPlayerState>() : nullptr)
	{
		MobaPlayerState->SetTeam(PickTeamForNewPlayer());
		MobaPlayerState->SetChampion(PickChampionForNewPlayer(Options, MobaPlayerState));

		// 메뉴에서 고른 소환사 주문(?Spells=D,F). 없거나 잘못되면 기본값(점멸·점화)을 둔다.
		EMobaSummonerSpell SpellD;
		EMobaSummonerSpell SpellF;
		if (ParseSpellsOption(UGameplayStatics::ParseOption(Options, TEXT("Spells")), SpellD, SpellF))
		{
			MobaPlayerState->SetSummonerSpell(0, SpellD);
			MobaPlayerState->SetSummonerSpell(1, SpellF);
		}

		UE_LOG(LogTemp, Log, TEXT("플레이어 입장: %s 팀, 챔피언 %s, 주문 %s"), *MobaTeam::GetTeamTag(MobaPlayerState->GetTeam()).ToString(), *GetNameSafe(MobaPlayerState->GetChampion()),
			*MakeSpellsOption(MobaPlayerState->GetSummonerSpell(0), MobaPlayerState->GetSummonerSpell(1)));
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

APawn* AMobaGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	FTransform Transform = SpawnTransform;
	Transform.AddToTranslation(GetTeamSlotOffset(GetTeamSlotIndex(NewPlayer)));
	return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, Transform);
}

FVector AMobaGameMode::GetTeamSlotOffset(int32 SlotIndex)
{
	if (SlotIndex <= 0)
	{
		return FVector::ZeroVector;
	}

	// 시작 지점을 둘러싸는 원 위에 차례로 놓는다.
	const float Angle = FMath::DegreesToRadians(45.0f + 90.0f * (SlotIndex - 1));
	return FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * 180.0f;
}

int32 AMobaGameMode::GetTeamSlotIndex(const AController* Player) const
{
	const AMobaPlayerState* Mine = Player ? Player->GetPlayerState<AMobaPlayerState>() : nullptr;
	if (!Mine || !GameState)
	{
		return 0;
	}

	int32 Index = 0;
	for (const APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (PlayerState == Mine)
		{
			return Index;
		}
		const AMobaPlayerState* Other = Cast<AMobaPlayerState>(PlayerState);
		Index += Other && Other->GetTeam() == Mine->GetTeam() ? 1 : 0;
	}
	return 0;
}

UMobaChampionData* AMobaGameMode::PickChampionForNewPlayer(const FString& Options, const AMobaPlayerState* NewPlayerState) const
{
	if (ChampionRoster.Num() == 0)
	{
		return GetDefaultChampion();
	}

	const FString Wanted = UGameplayStatics::ParseOption(Options, TEXT("Champion"));
	if (!Wanted.IsEmpty())
	{
		for (const FSoftObjectPath& Path : ChampionRoster)
		{
			if (Path.GetAssetName().Equals(Wanted, ESearchCase::IgnoreCase))
			{
				return Cast<UMobaChampionData>(Path.TryLoad());
			}
		}
	}

	// 먼저 들어와 있는 플레이어 수를 순번으로 쓴다.
	int32 PlayerIndex = 0;
	if (GameState)
	{
		for (const APlayerState* PlayerState : GameState->PlayerArray)
		{
			const AMobaPlayerState* Other = Cast<AMobaPlayerState>(PlayerState);
			if (Other && Other != NewPlayerState && Other->GetChampion())
			{
				++PlayerIndex;
			}
		}
	}

	UMobaChampionData* Champion = Cast<UMobaChampionData>(ChampionRoster[PlayerIndex % ChampionRoster.Num()].TryLoad());
	return Champion ? Champion : GetDefaultChampion();
}

void AMobaGameMode::Announce(const FString& Message, EMobaTeam Team)
{
	if (AMobaGameState* MobaGameState = GetGameState<AMobaGameState>())
	{
		MobaGameState->MulticastAnnounce(Message, Team);
	}
}

FString AMobaGameMode::DescribeUnit(const AActor* Unit) const
{
	const FString TeamName = MobaTeam::GetTeamDisplayName(MobaCombat::GetTeam(Unit));

	if (const AMobaCharacter* Champion = Cast<AMobaCharacter>(Unit))
	{
		const AMobaPlayerState* PlayerState = Champion->GetPlayerState<AMobaPlayerState>();
		return FString::Printf(TEXT("%s %s"), *TeamName, PlayerState ? *PlayerState->GetChampionDisplayName() : TEXT("챔피언"));
	}

	if (const AMobaStructure* Structure = Cast<AMobaStructure>(Unit))
	{
		switch (Structure->GetStructureType())
		{
		case EMobaStructureType::Inhibitor: return TeamName + TEXT(" 억제기");
		case EMobaStructureType::Nexus: return TeamName + TEXT(" 넥서스");
		default: return TeamName + TEXT(" 타워");
		}
	}

	if (Unit && Unit->IsA<AMobaMinion>())
	{
		return TeamName + TEXT(" 미니언");
	}

	if (const AMobaMonster* Monster = Cast<AMobaMonster>(Unit))
	{
		return Monster->GetCamp() ? AMobaJungleCamp::GetDefinition(Monster->GetCamp()->GetCampType()).DisplayName : FString(TEXT("몬스터"));
	}

	return TEXT("알 수 없는 것");
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

	AMobaPlayerState* VictimState = Victim->GetPlayerState<AMobaPlayerState>();
	const APawn* KillerPawn = Cast<APawn>(Killer);
	AMobaPlayerState* KillerState = KillerPawn ? KillerPawn->GetPlayerState<AMobaPlayerState>() : nullptr;

	// 현상금은 처치당한 쪽의 연속 기록으로 정하므로 기록을 고치기 전에 계산한다.
	const float Bounty = GetKillBounty(VictimState);
	const float RespawnTime = GetRespawnTime(VictimState ? VictimState->GetChampionLevel() : 1);

	if (VictimState)
	{
		VictimState->AddDeath();
		VictimState->SetRespawnServerTime(GameState ? GameState->GetServerWorldTimeSeconds() + RespawnTime : 0.0f);
	}

	// 경기의 첫 챔피언 처치는 선취점 보너스를 준다. 포탑·미니언에게 죽은 것은 치지 않는다.
	const bool bFirstBlood = KillerState && !bFirstBloodTaken;
	if (KillerState)
	{
		KillerState->AddKill();
		KillerState->AddGold(Bounty + (bFirstBlood ? FirstBloodBonusGold : 0.0f));
		KillerState->AddExperience(ChampionKillExperience);
		bFirstBloodTaken = true;
	}

	// 최근에 함께 피해를 준 아군에게 어시스트를 준다. 1대1에서는 생기지 않지만 인원을 늘리면 쓰인다.
	if (VictimState)
	{
		TArray<AMobaPlayerState*> Assisters;
		if (const TMap<TWeakObjectPtr<AMobaPlayerState>, double>* Attackers = RecentChampionDamage.Find(VictimState))
		{
			const double Now = GetWorld()->GetTimeSeconds();
			for (const TPair<TWeakObjectPtr<AMobaPlayerState>, double>& Entry : *Attackers)
			{
				AMobaPlayerState* Attacker = Entry.Key.Get();
				if (Attacker && Attacker != KillerState && Now - Entry.Value <= AssistWindow && Attacker->GetTeam() != VictimState->GetTeam())
				{
					Assisters.Add(Attacker);
				}
			}
		}

		for (AMobaPlayerState* Assister : Assisters)
		{
			Assister->AddAssist();
			Assister->AddGold(Bounty * AssistGoldFraction / Assisters.Num());
		}
		RecentChampionDamage.Remove(VictimState);
	}

	const bool bSpecialBounty = !FMath::IsNearlyEqual(Bounty, ChampionKillGold);
	Announce(FString::Printf(TEXT("%s → %s 처치%s"), *DescribeUnit(Killer), *DescribeUnit(Victim),
		bSpecialBounty ? *FString::Printf(TEXT(" (현상금 %dG)"), FMath::RoundToInt(Bounty)) : TEXT("")), MobaCombat::GetTeam(Killer));

	if (bFirstBlood)
	{
		Announce(FString::Printf(TEXT("선취점! %s (+%dG)"), *DescribeUnit(KillerPawn), FMath::RoundToInt(FirstBloodBonusGold)), KillerState->GetTeam());
	}

	if (KillerState && KillerState->GetKillStreak() >= 3)
	{
		Announce(FString::Printf(TEXT("%s %d연속 처치!"), *DescribeUnit(KillerPawn), KillerState->GetKillStreak()), KillerState->GetTeam());
	}

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateWeakLambda(Victim, [this, Victim]()
	{
		RespawnChampion(Victim);
	}), RespawnTime, false);
}

void AMobaGameMode::HandleTowerPlateDestroyed(AMobaTower* Tower, AActor* DamageInstigator)
{
	if (!Tower)
	{
		return;
	}

	// 챔피언이 깼으면 그 챔피언이, 미니언 등이 깼으면 근처의 상대 챔피언들이 나눠 받는다.
	TArray<AMobaPlayerState*> Receivers;
	const APawn* InstigatorPawn = Cast<APawn>(DamageInstigator);
	if (AMobaPlayerState* InstigatorState = InstigatorPawn ? InstigatorPawn->GetPlayerState<AMobaPlayerState>() : nullptr)
	{
		Receivers.Add(InstigatorState);
	}
	else
	{
		for (TActorIterator<AMobaCharacter> It(GetWorld()); It; ++It)
		{
			AMobaPlayerState* State = It->GetPlayerState<AMobaPlayerState>();
			if (State && State->GetTeam() != Tower->GetTeam() && !It->IsDead()
				&& FVector::Dist2D(It->GetActorLocation(), Tower->GetActorLocation()) <= TowerPlateShareRadius)
			{
				Receivers.Add(State);
			}
		}
	}

	for (AMobaPlayerState* Receiver : Receivers)
	{
		Receiver->AddGold(TowerPlateGold / Receivers.Num());
	}

	const EMobaTeam Breaker = Tower->GetTeam() == EMobaTeam::Blue ? EMobaTeam::Red : EMobaTeam::Blue;
	Announce(FString::Printf(TEXT("%s 방패 파괴 (남은 방패 %d)%s"), *DescribeUnit(Tower), Tower->GetPlatesRemaining(),
		Receivers.Num() > 0 ? *FString::Printf(TEXT(" +%dG"), FMath::RoundToInt(TowerPlateGold / Receivers.Num())) : TEXT("")), Breaker);
}

void AMobaGameMode::EndTowerPlating()
{
	bool bAnyRemoved = false;
	for (TActorIterator<AMobaTower> It(GetWorld()); It; ++It)
	{
		bAnyRemoved |= It->GetPlatesRemaining() > 0;
		It->RemovePlates();
	}

	if (bAnyRemoved)
	{
		Announce(TEXT("포탑 방패가 사라졌습니다"), EMobaTeam::None);
	}
}

float AMobaGameMode::GetRespawnTime(int32 ChampionLevel) const
{
	return FMath::Min(RespawnDelay + RespawnDelayPerLevel * FMath::Max(ChampionLevel - 1, 0), MaxRespawnDelay);
}

float AMobaGameMode::GetKillBounty(const AMobaPlayerState* VictimState) const
{
	return VictimState ? CalculateKillBounty(VictimState->GetKillStreak(), VictimState->GetDeathStreak()) : ChampionKillGold;
}

float AMobaGameMode::CalculateKillBounty(int32 VictimKillStreak, int32 VictimDeathStreak) const
{
	if (VictimKillStreak >= 2)
	{
		return FMath::Min(ChampionKillGold + KillStreakBountyStep * (VictimKillStreak - 1), MaxKillBounty);
	}

	if (VictimDeathStreak >= 2)
	{
		return FMath::Max(ChampionKillGold - DeathStreakBountyStep * (VictimDeathStreak - 1), MinKillBounty);
	}

	return ChampionKillGold;
}

int32 AMobaGameMode::GetMinionUpgradeCount(float MatchTimeSeconds) const
{
	if (MinionUpgradeInterval <= 0.0f || MatchTimeSeconds <= 0.0f)
	{
		return 0;
	}
	return FMath::Min(FMath::FloorToInt(MatchTimeSeconds / MinionUpgradeInterval), MaxMinionUpgrades);
}

void AMobaGameMode::RequestSurrender(APlayerController* Player)
{
	const AMobaGameState* MobaGameState = GetGameState<AMobaGameState>();
	AMobaPlayerState* VoterState = Player ? Player->GetPlayerState<AMobaPlayerState>() : nullptr;
	if (!MobaGameState || !VoterState || !MobaGameState->HasMatchStarted() || MobaGameState->IsMatchOver()
		|| MobaGameState->GetMatchTimeSeconds() < SurrenderMinTime || VoterState->HasVotedSurrender())
	{
		return;
	}

	VoterState->SetVotedSurrender(true);

	int32 TeamSize = 0;
	int32 Votes = 0;
	for (const APlayerState* PlayerState : MobaGameState->PlayerArray)
	{
		const AMobaPlayerState* Member = Cast<AMobaPlayerState>(PlayerState);
		if (Member && Member->GetTeam() == VoterState->GetTeam())
		{
			++TeamSize;
			Votes += Member->HasVotedSurrender() ? 1 : 0;
		}
	}

	const FString TeamName = MobaTeam::GetTeamDisplayName(VoterState->GetTeam());
	if (Votes >= TeamSize)
	{
		Announce(FString::Printf(TEXT("%s 팀이 항복했습니다"), *TeamName), VoterState->GetTeam());
		EndMatch(VoterState->GetTeam() == EMobaTeam::Blue ? EMobaTeam::Red : EMobaTeam::Blue, false, true);
	}
	else
	{
		Announce(FString::Printf(TEXT("%s 팀 항복 투표 %d / %d"), *TeamName, Votes, TeamSize), VoterState->GetTeam());
	}
}

void AMobaGameMode::RespawnChampion(AMobaCharacter* Champion)
{
	if (!Champion || MobaCombat::IsMatchOver(this))
	{
		return;
	}

	const AActor* StartSpot = ChoosePlayerStart(Champion->GetController());
	const FVector Location = StartSpot ? StartSpot->GetActorLocation() + GetTeamSlotOffset(GetTeamSlotIndex(Champion->GetController())) : Champion->GetActorLocation();
	const FRotator Rotation = StartSpot ? StartSpot->GetActorRotation() : Champion->GetActorRotation();

	if (AMobaPlayerState* PlayerState = Champion->GetPlayerState<AMobaPlayerState>())
	{
		PlayerState->SetRespawnServerTime(0.0f);
	}

	Champion->Respawn(Location, Rotation);
}
