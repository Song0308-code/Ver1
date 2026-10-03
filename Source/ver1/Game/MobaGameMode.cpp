#include "Game/MobaGameMode.h"
#include "AbilitySystem/MobaAbilityLibrary.h"
#include "AbilitySystemComponent.h"
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
		Minion->InitMinion(Team, Type, LanePath, GetMinionConfig(Type));
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

		UE_LOG(LogTemp, Log, TEXT("플레이어 입장: %s 팀, 챔피언 %s"), *MobaTeam::GetTeamTag(MobaPlayerState->GetTeam()).ToString(), *GetNameSafe(MobaPlayerState->GetChampion()));
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

	Announce(FString::Printf(TEXT("%s → %s 처치"), *DescribeUnit(Killer), *DescribeUnit(Victim)), MobaCombat::GetTeam(Killer));

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
