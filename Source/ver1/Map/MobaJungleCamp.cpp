#include "Map/MobaJungleCamp.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Game/MobaGameMode.h"
#include "Game/MobaGameState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

namespace
{
	FMobaMonsterConfig MakeMonster(float Health, float AttackDamage, float AttackSpeed, float AttackRange, bool bRanged, float Gold, float Experience, float Scale, const FVector2D& Offset = FVector2D::ZeroVector)
	{
		FMobaMonsterConfig Config;
		Config.Stats.MaxHealth = Health;
		Config.Stats.MaxMana = 0.0f;
		Config.Stats.AttackDamage = AttackDamage;
		Config.Stats.Armor = 20.0f;
		Config.Stats.MagicResist = 20.0f;
		Config.Stats.AttackSpeed = AttackSpeed;
		Config.Stats.MoveSpeed = 330.0f;
		Config.Stats.AttackRange = AttackRange;
		Config.Stats.HealthRegenPerSecond = 0.0f;
		Config.Stats.ManaRegenPerSecond = 0.0f;
		Config.bRanged = bRanged;
		Config.GoldReward = Gold;
		Config.ExperienceReward = Experience;
		Config.VisualScale = Scale;
		Config.Offset = Offset;
		return Config;
	}
}

FMobaCampDefinition AMobaJungleCamp::GetDefinition(EMobaCampType Type)
{
	// 수치는 실제 게임을 참고해 프로토타입 챔피언 혼자서도 잡을 수 있게 낮춘 값이다.
	FMobaCampDefinition Definition;

	switch (Type)
	{
	case EMobaCampType::BlueBuff:
		Definition.DisplayName = TEXT("푸른 파수꾼");
		Definition.RespawnTime = 300.0f;
		Definition.Monsters.Add(MakeMonster(1800.0f, 45.0f, 0.5f, 170.0f, false, 90.0f, 150.0f, 1.8f));
		break;

	case EMobaCampType::RedBuff:
		Definition.DisplayName = TEXT("붉은 덩굴정령");
		Definition.RespawnTime = 300.0f;
		Definition.Monsters.Add(MakeMonster(1800.0f, 50.0f, 0.5f, 170.0f, false, 90.0f, 150.0f, 1.8f));
		break;

	case EMobaCampType::Gromp:
		Definition.DisplayName = TEXT("심술두꺼비");
		Definition.Monsters.Add(MakeMonster(1400.0f, 45.0f, 0.6f, 250.0f, true, 80.0f, 120.0f, 1.5f));
		break;

	case EMobaCampType::Wolves:
		Definition.DisplayName = TEXT("어스름 늑대");
		Definition.Monsters.Add(MakeMonster(1100.0f, 30.0f, 0.6f, 150.0f, false, 55.0f, 75.0f, 1.3f));
		Definition.Monsters.Add(MakeMonster(350.0f, 10.0f, 0.6f, 130.0f, false, 10.0f, 20.0f, 0.8f, FVector2D(-110.0, 130.0)));
		Definition.Monsters.Add(MakeMonster(350.0f, 10.0f, 0.6f, 130.0f, false, 10.0f, 20.0f, 0.8f, FVector2D(-110.0, -130.0)));
		break;

	case EMobaCampType::Raptors:
		Definition.DisplayName = TEXT("칼날부리");
		Definition.Monsters.Add(MakeMonster(900.0f, 25.0f, 0.65f, 150.0f, false, 35.0f, 60.0f, 1.2f));
		Definition.Monsters.Add(MakeMonster(300.0f, 8.0f, 1.0f, 130.0f, false, 8.0f, 15.0f, 0.7f, FVector2D(120.0, 110.0)));
		Definition.Monsters.Add(MakeMonster(300.0f, 8.0f, 1.0f, 130.0f, false, 8.0f, 15.0f, 0.7f, FVector2D(120.0, -110.0)));
		Definition.Monsters.Add(MakeMonster(300.0f, 8.0f, 1.0f, 130.0f, false, 8.0f, 15.0f, 0.7f, FVector2D(-120.0, 110.0)));
		Definition.Monsters.Add(MakeMonster(300.0f, 8.0f, 1.0f, 130.0f, false, 8.0f, 15.0f, 0.7f, FVector2D(-120.0, -110.0)));
		break;

	case EMobaCampType::Krugs:
		Definition.DisplayName = TEXT("돌거북");
		Definition.InitialSpawnTime = 100.0f;
		Definition.Monsters.Add(MakeMonster(1100.0f, 40.0f, 0.6f, 150.0f, false, 45.0f, 90.0f, 1.5f));
		Definition.Monsters.Add(MakeMonster(500.0f, 15.0f, 0.6f, 130.0f, false, 15.0f, 40.0f, 1.0f, FVector2D(0.0, 170.0)));
		break;

	case EMobaCampType::Scuttle:
		Definition.DisplayName = TEXT("바위게");
		Definition.InitialSpawnTime = 210.0f;
		Definition.RespawnTime = 150.0f;
		Definition.Monsters.Add(MakeMonster(1050.0f, 0.0f, 0.0f, 0.0f, false, 70.0f, 115.0f, 1.2f));
		Definition.Monsters[0].bPassive = true;
		break;

	case EMobaCampType::Dragon:
		Definition.DisplayName = TEXT("드래곤");
		Definition.InitialSpawnTime = 300.0f;
		Definition.RespawnTime = 300.0f;
		Definition.Monsters.Add(MakeMonster(3500.0f, 80.0f, 0.5f, 500.0f, true, 25.0f, 150.0f, 2.5f));
		break;

	case EMobaCampType::Baron:
		Definition.DisplayName = TEXT("내셔 남작");
		Definition.InitialSpawnTime = 1200.0f;
		Definition.RespawnTime = 360.0f;
		Definition.Monsters.Add(MakeMonster(9000.0f, 150.0f, 0.6f, 600.0f, true, 25.0f, 600.0f, 3.5f));
		break;
	}

	return Definition;
}

AMobaJungleCamp::AMobaJungleCamp()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	MonsterClass = AMobaMonster::StaticClass();
}

void AMobaJungleCamp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMobaJungleCamp, CampType);
	DOREPLIFETIME(AMobaJungleCamp, NextSpawnMatchTime);
	DOREPLIFETIME(AMobaJungleCamp, AliveCount);
}

void AMobaJungleCamp::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		NextSpawnMatchTime = InitialSpawnTimeOverride >= 0.0f ? InitialSpawnTimeOverride : GetDefinition(CampType).InitialSpawnTime;
		GetWorldTimerManager().SetTimer(SpawnTimer, this, &AMobaJungleCamp::UpdateSpawn, 0.5f, true);
	}
}

float AMobaJungleCamp::GetSecondsUntilSpawn() const
{
	const AMobaGameState* GameState = GetWorld()->GetGameState<AMobaGameState>();
	if (!GameState || !GameState->HasMatchStarted() || IsAlive())
	{
		return 0.0f;
	}

	return FMath::Max(NextSpawnMatchTime - GameState->GetMatchTimeSeconds(), 0.0f);
}

void AMobaJungleCamp::StopSpawning()
{
	GetWorldTimerManager().ClearTimer(SpawnTimer);
}

void AMobaJungleCamp::UpdateSpawn()
{
	bool bForceSpawn = false;

#if WITH_EDITORONLY_DATA
	bForceSpawn = bDebugSpawnNow;
	bDebugSpawnNow = false;
#endif

	const AMobaGameState* GameState = GetWorld()->GetGameState<AMobaGameState>();
	if (!GameState || !GameState->HasMatchStarted() || GameState->IsMatchOver() || IsAlive())
	{
		return;
	}

	const bool bSpawnNow = bForceSpawn || GameState->GetMatchTimeSeconds() >= NextSpawnMatchTime;

	if (bSpawnNow)
	{
		SpawnMonsters();
	}
}

void AMobaJungleCamp::SpawnMonsters()
{
	if (!MonsterClass)
	{
		return;
	}

	const FMobaCampDefinition Definition = GetDefinition(CampType);
	for (const FMobaMonsterConfig& Config : Definition.Monsters)
	{
		const FVector Home = GetActorLocation() + GetActorRotation().RotateVector(FVector(Config.Offset.X, Config.Offset.Y, 0.0));

		// 캡슐이 바닥에 묻히지 않도록 크기에 맞춰 띄워서 내보낸다.
		const FTransform SpawnTransform(GetActorRotation(), Home + FVector(0.0, 0.0, 60.0 * FMath::Max(Config.VisualScale, 1.0f) + 20.0));
		if (AMobaMonster* Monster = GetWorld()->SpawnActorDeferred<AMobaMonster>(MonsterClass, SpawnTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
		{
			Monster->InitMonster(this, Config, Home);
			Monster->FinishSpawning(SpawnTransform);
			++AliveCount;
		}
	}
}

void AMobaJungleCamp::HandleMonsterKilled(AMobaMonster* Monster, AActor* Killer)
{
	if (AliveCount <= 0)
	{
		return;
	}

	--AliveCount;
	if (AliveCount > 0)
	{
		return;
	}

	const AMobaGameState* GameState = GetWorld()->GetGameState<AMobaGameState>();
	const float RespawnTime = RespawnTimeOverride >= 0.0f ? RespawnTimeOverride : GetDefinition(CampType).RespawnTime;
	NextSpawnMatchTime = (GameState ? GameState->GetMatchTimeSeconds() : 0.0f) + RespawnTime;

	if (AMobaGameMode* GameMode = GetWorld()->GetAuthGameMode<AMobaGameMode>())
	{
		GameMode->HandleCampCleared(this, Killer);
	}
}
