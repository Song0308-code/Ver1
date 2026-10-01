#include "Structures/MobaStructure.h"
#include "AbilitySystem/MobaAbilityLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaCharacter.h"
#include "Combat/MobaCombatComponent.h"
#include "Combat/MobaCombatEvents.h"
#include "Combat/MobaCombatLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Game/MobaGameMode.h"
#include "Game/MobaGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Units/MobaMinion.h"

namespace
{
	/** 엔진 기본 원기둥(지름 100, 높이 100)을 바닥에 세워 놓는다. */
	void SetupCylinder(UStaticMeshComponent* Mesh, float Radius, float Height)
	{
		Mesh->SetRelativeScale3D(FVector(Radius / 50.0f, Radius / 50.0f, Height / 100.0f));
		Mesh->SetRelativeLocation(FVector(0.0, 0.0, Height * 0.5f));
	}
}

AMobaStructure::AMobaStructure()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Static);
	RootComponent = SceneRoot;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetMobility(EComponentMobility::Static);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));

	// 범위 검색에서 지형과 구분되도록 동적 오브젝트로 분류한다.
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CylinderMesh.Object);
	}

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UMobaAttributeSet>(TEXT("AttributeSet"));

	Stats.MaxMana = 0.0f;
	Stats.AttackDamage = 0.0f;
	Stats.AttackSpeed = 0.0f;
	Stats.MoveSpeed = 0.0f;
	Stats.AttackRange = 0.0f;
	Stats.HealthRegenPerSecond = 0.0f;
	Stats.ManaRegenPerSecond = 0.0f;
}

void AMobaStructure::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMobaStructure, Team);
}

void AMobaStructure::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyTeamColor();
}

void AMobaStructure::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	ApplyTeamColor();

	if (HasAuthority())
	{
		AttributeSet->InitFromBaseStats(Stats);
		AttributeSet->OnOutOfHealth.AddUObject(this, &AMobaStructure::HandleOutOfHealth);
		UpdateProtection();
	}
}

void AMobaStructure::OnRep_Team()
{
	ApplyTeamColor();
}

void AMobaStructure::ApplyTeamColor()
{
	const FLinearColor TeamColor = MobaTeam::GetTeamColor(Team);
	Mesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(TeamColor.R, TeamColor.G, TeamColor.B));
}

bool AMobaStructure::IsDestroyed() const
{
	return AbilitySystemComponent->HasMatchingGameplayTag(MobaTags::State_Dead);
}

bool AMobaStructure::IsInvulnerable() const
{
	return AbilitySystemComponent->HasMatchingGameplayTag(MobaTags::State_Invulnerable);
}

void AMobaStructure::UpdateProtection()
{
	if (!HasAuthority() || IsDestroyed())
	{
		return;
	}

	bool bProtected = false;
	for (const AMobaStructure* Structure : RequiresAllDestroyed)
	{
		if (Structure && !Structure->IsDestroyed())
		{
			bProtected = true;
			break;
		}
	}

	if (!bProtected && RequiresAnyDestroyed.Num() > 0)
	{
		bProtected = true;
		for (const AMobaStructure* Structure : RequiresAnyDestroyed)
		{
			if (!Structure || Structure->IsDestroyed())
			{
				bProtected = false;
				break;
			}
		}
	}

	AbilitySystemComponent->SetLooseGameplayTagCount(MobaTags::State_Invulnerable, bProtected ? 1 : 0, EGameplayTagReplicationState::TagOnly);
}

void AMobaStructure::HandleOutOfHealth(AActor* DamageInstigator)
{
	if (IsDestroyed())
	{
		return;
	}

	AbilitySystemComponent->AddLooseGameplayTag(MobaTags::State_Dead, 1, EGameplayTagReplicationState::TagOnly);
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);

	// 이 건물이 지켜 주던 다음 건물들을 공격할 수 있게 한다.
	for (TActorIterator<AMobaStructure> It(GetWorld()); It; ++It)
	{
		It->UpdateProtection();
	}

	if (AMobaGameMode* GameMode = GetWorld()->GetAuthGameMode<AMobaGameMode>())
	{
		GameMode->HandleStructureDestroyed(this, DamageInstigator);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Tower

AMobaTower::AMobaTower()
{
	StructureType = EMobaStructureType::Tower;
	SetupCylinder(Mesh, 100.0f, 500.0f);
	HealthBarHeight = 560.0f;

	Stats.MaxHealth = 3500.0f;
	Stats.AttackDamage = 170.0f;
	Stats.Armor = 40.0f;
	Stats.MagicResist = 40.0f;
	Stats.AttackSpeed = 0.83f;
	Stats.AttackRange = 750.0f;
	GoldReward = 150.0f;

	CombatComponent = CreateDefaultSubobject<UMobaCombatComponent>(TEXT("CombatComponent"));

	// 타워 꼭대기에서 큼직한 탄을 쏜다.
	CombatComponent->ConfigureProjectileVisual(0.6f, FVector(0.0, 0.0, 480.0));
}

void AMobaTower::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		CombatComponent->ConfigureBasicAttack(true, ProjectileSpeed);
		GetWorldTimerManager().SetTimer(TargetingTimer, this, &AMobaTower::UpdateTargeting, 0.2f, true);

		if (UMobaCombatEvents* Events = UMobaCombatEvents::Get(this))
		{
			DamageDealtHandle = Events->OnDamageDealt.AddUObject(this, &AMobaTower::HandleDamageDealt);
		}
	}
}

void AMobaTower::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMobaCombatEvents* Events = UMobaCombatEvents::Get(this))
	{
		Events->OnDamageDealt.Remove(DamageDealtHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AMobaTower::UpdateTargeting()
{
	if (IsDestroyed() || MobaCombat::IsMatchOver(this))
	{
		CombatComponent->SetAttackTarget(nullptr);
		return;
	}

	const float Range = CombatComponent->GetAttackRange();

	// 한번 노린 대상은 죽거나 사거리를 벗어날 때까지 유지한다.
	AActor* CurrentTarget = CombatComponent->GetAttackTarget();
	if (CurrentTarget && MobaCombat::IsValidEnemyTarget(this, CurrentTarget) && MobaCombat::GetDistanceToTargetEdge(this, CurrentTarget) <= Range)
	{
		return;
	}

	CombatComponent->SetAttackTarget(FindBestTarget(Range));
}

AActor* AMobaTower::FindBestTarget(float Range) const
{
	TArray<AActor*> Enemies;
	MobaAbility::FindEnemiesInRadius(this, GetActorLocation(), Range + 100.0f, Enemies, false);

	// 미니언을 먼저 노리고, 없으면 챔피언을 노린다.
	AActor* Best = nullptr;
	int32 BestPriority = MAX_int32;
	float BestDistance = TNumericLimits<float>::Max();

	for (AActor* Enemy : Enemies)
	{
		const float Distance = MobaCombat::GetDistanceToTargetEdge(this, Enemy);
		if (Distance > Range)
		{
			continue;
		}

		const int32 Priority = Enemy->IsA<AMobaMinion>() ? 0 : 1;
		if (Priority < BestPriority || (Priority == BestPriority && Distance < BestDistance))
		{
			Best = Enemy;
			BestPriority = Priority;
			BestDistance = Distance;
		}
	}

	return Best;
}

void AMobaTower::HandleDamageDealt(AActor* DamageInstigator, AActor* Victim, float Damage)
{
	// 사거리 안의 적 챔피언이 아군 챔피언을 공격하면 그 적으로 대상을 바꾼다.
	const AMobaCharacter* VictimChampion = Cast<AMobaCharacter>(Victim);
	AMobaCharacter* AttackerChampion = Cast<AMobaCharacter>(DamageInstigator);
	if (IsDestroyed() || !VictimChampion || !AttackerChampion || VictimChampion->GetTeam() != Team)
	{
		return;
	}

	if (Cast<AMobaCharacter>(CombatComponent->GetAttackTarget()))
	{
		return;
	}

	if (MobaCombat::IsValidEnemyTarget(this, AttackerChampion) && MobaCombat::GetDistanceToTargetEdge(this, AttackerChampion) <= CombatComponent->GetAttackRange())
	{
		CombatComponent->SetAttackTarget(AttackerChampion);
	}
}

void AMobaTower::HandleOutOfHealth(AActor* DamageInstigator)
{
	GetWorldTimerManager().ClearTimer(TargetingTimer);
	CombatComponent->SetAttackTarget(nullptr);

	Super::HandleOutOfHealth(DamageInstigator);
}

// ---------------------------------------------------------------------------------------------------------------------
// Inhibitor / Nexus

AMobaInhibitor::AMobaInhibitor()
{
	StructureType = EMobaStructureType::Inhibitor;
	SetupCylinder(Mesh, 200.0f, 150.0f);
	HealthBarHeight = 230.0f;

	Stats.MaxHealth = 4000.0f;
	Stats.Armor = 20.0f;
	Stats.MagicResist = 20.0f;
	GoldReward = 50.0f;
}

AMobaNexus::AMobaNexus()
{
	StructureType = EMobaStructureType::Nexus;
	SetupCylinder(Mesh, 300.0f, 300.0f);
	HealthBarHeight = 380.0f;

	Stats.MaxHealth = 5500.0f;
	Stats.Armor = 20.0f;
	Stats.MagicResist = 20.0f;
}
