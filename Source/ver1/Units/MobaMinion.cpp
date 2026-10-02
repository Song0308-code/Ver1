#include "Units/MobaMinion.h"
#include "AIController.h"
#include "AbilitySystem/MobaAbilityLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaCharacter.h"
#include "Combat/MobaCombatComponent.h"
#include "Combat/MobaCombatEvents.h"
#include "Combat/MobaCombatLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Game/MobaGameMode.h"
#include "Game/MobaGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Map/MobaLanePath.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AMobaMinion::AMobaMinion()
{
	PrimaryActorTick.bCanEverTick = false;

	// 미니맵과 시야 계산에 필요하므로 거리와 무관하게 복제한다.
	bAlwaysRelevant = true;

	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(30.0f, 50.0f);

	bUseControllerRotationYaw = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.0, 720.0, 0.0);
	Movement->MaxAcceleration = 4096.0f;
	Movement->BrakingDecelerationWalking = 4096.0f;

	// 서로 비켜 가게 해서 앞 미니언이 싸우는 동안 뒤 미니언이 막히지 않도록 한다.
	Movement->bUseRVOAvoidance = true;
	Movement->AvoidanceConsiderationRadius = 200.0f;

	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
	PlaceholderMesh->SetupAttachment(RootComponent);
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		PlaceholderMesh->SetStaticMesh(CubeMesh.Object);
	}

	CombatComponent = CreateDefaultSubobject<UMobaCombatComponent>(TEXT("CombatComponent"));
	CombatComponent->ConfigureProjectileVisual(0.22f, FVector::ZeroVector);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UMobaAttributeSet>(TEXT("AttributeSet"));
}

void AMobaMinion::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMobaMinion, Team);
	DOREPLIFETIME(AMobaMinion, MinionType);
}

void AMobaMinion::InitMinion(EMobaTeam InTeam, EMobaMinionType InType, AMobaLanePath* InLanePath, const FMobaMinionConfig& InConfig)
{
	Team = InTeam;
	MinionType = InType;
	LanePath = InLanePath;
	Config = InConfig;
}

void AMobaMinion::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	ApplyAppearance();

	if (HasAuthority())
	{
		AttributeSet->InitFromBaseStats(Config.Stats);
		AttributeSet->OnOutOfHealth.AddUObject(this, &AMobaMinion::HandleOutOfHealth);
		GetCharacterMovement()->MaxWalkSpeed = Config.Stats.MoveSpeed;
		CombatComponent->ConfigureBasicAttack(Config.bRanged, Config.ProjectileSpeed);

		if (UMobaCombatEvents* Events = UMobaCombatEvents::Get(this))
		{
			DamageDealtHandle = Events->OnDamageDealt.AddUObject(this, &AMobaMinion::HandleDamageDealt);
		}

		// 같은 틱에 모든 미니언이 한꺼번에 판단하지 않도록 시작 시점을 조금씩 어긋나게 한다.
		GetWorldTimerManager().SetTimer(AITimer, this, &AMobaMinion::UpdateAI, 0.25f, true, FMath::FRandRange(0.05f, 0.25f));
	}
}

void AMobaMinion::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMobaCombatEvents* Events = UMobaCombatEvents::Get(this))
	{
		Events->OnDamageDealt.Remove(DamageDealtHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AMobaMinion::OnRep_Appearance()
{
	ApplyAppearance();
}

void AMobaMinion::ApplyAppearance()
{
	// 근접은 넓적한 상자, 원거리는 가늘고 긴 상자, 슈퍼 미니언은 큰 상자로 구분한다.
	FVector Scale(0.55f, 0.55f, 0.8f);
	if (MinionType == EMobaMinionType::Caster)
	{
		Scale = FVector(0.35f, 0.35f, 1.0f);
	}
	else if (MinionType == EMobaMinionType::Super)
	{
		Scale = FVector(0.9f, 0.9f, 1.2f);
	}
	PlaceholderMesh->SetRelativeScale3D(Scale);

	const FLinearColor TeamColor = MobaTeam::GetTeamColor(Team);
	PlaceholderMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(TeamColor.R, TeamColor.G, TeamColor.B));
}

void AMobaMinion::StopActing()
{
	bActing = false;
	GetWorldTimerManager().ClearTimer(AITimer);
	CombatComponent->SetAttackTarget(nullptr);
	if (AController* MinionController = GetController())
	{
		MinionController->StopMovement();
	}
}

void AMobaMinion::UpdateAI()
{
	if (!bActing || MobaCombat::IsDead(this))
	{
		return;
	}

	AActor* CurrentTarget = CombatComponent->GetAttackTarget();
	if (CurrentTarget)
	{
		const bool bStillValid = MobaCombat::IsValidEnemyTarget(this, CurrentTarget) && MobaCombat::GetDistanceToTargetEdge(this, CurrentTarget) <= LeashRange;
		if (bStillValid)
		{
			return;
		}
		CombatComponent->SetAttackTarget(nullptr);
	}

	if (AActor* NewTarget = FindBestTarget())
	{
		CombatComponent->SetAttackTarget(NewTarget);
	}
	else
	{
		AdvanceAlongLane();
	}
}

AActor* AMobaMinion::FindBestTarget() const
{
	TArray<AActor*> Enemies;
	MobaAbility::FindEnemiesInRadius(this, GetActorLocation(), AcquisitionRange, Enemies, true);

	// 미니언, 챔피언, 건물 순으로 노리고 같은 종류 안에서는 가장 가까운 것을 고른다.
	AActor* Best = nullptr;
	int32 BestPriority = MAX_int32;
	float BestDistance = TNumericLimits<float>::Max();

	for (AActor* Enemy : Enemies)
	{
		const int32 Priority = Enemy->IsA<AMobaMinion>() ? 0 : (Enemy->IsA<AMobaCharacter>() ? 1 : 2);
		const float Distance = MobaCombat::GetDistanceToTargetEdge(this, Enemy);
		if (Priority < BestPriority || (Priority == BestPriority && Distance < BestDistance))
		{
			Best = Enemy;
			BestPriority = Priority;
			BestDistance = Distance;
		}
	}

	return Best;
}

void AMobaMinion::AdvanceAlongLane()
{
	AAIController* MinionController = Cast<AAIController>(GetController());
	if (!MinionController || !LanePath)
	{
		return;
	}

	// 블루는 스플라인의 시작에서 끝으로, 레드는 반대로 걷는다.
	const USplineComponent* Spline = LanePath->GetSpline();
	const float Current = Spline->GetDistanceAlongSplineAtSplineInputKey(Spline->FindInputKeyClosestToWorldLocation(GetActorLocation()));
	const float Target = Team == EMobaTeam::Blue
		? FMath::Min(Current + LaneLookAhead, Spline->GetSplineLength())
		: FMath::Max(Current - LaneLookAhead, 0.0f);

	const FVector Goal = Spline->GetLocationAtDistanceAlongSpline(Target, ESplineCoordinateSpace::World);
	MinionController->MoveToLocation(Goal, 60.0f, true, true, true);
}

void AMobaMinion::HandleDamageDealt(AActor* DamageInstigator, AActor* Victim, float Damage)
{
	// 적 챔피언이 근처의 아군 챔피언을 공격하면 그 적을 노린다.
	const AMobaCharacter* VictimChampion = Cast<AMobaCharacter>(Victim);
	AMobaCharacter* AttackerChampion = Cast<AMobaCharacter>(DamageInstigator);
	if (!bActing || !VictimChampion || !AttackerChampion || VictimChampion->GetTeam() != Team)
	{
		return;
	}

	if (Cast<AMobaCharacter>(CombatComponent->GetAttackTarget()))
	{
		return;
	}

	if (MobaCombat::IsValidEnemyTarget(this, AttackerChampion) && MobaCombat::GetDistanceToTargetEdge(this, AttackerChampion) <= AcquisitionRange)
	{
		CombatComponent->SetAttackTarget(AttackerChampion);
	}
}

void AMobaMinion::HandleOutOfHealth(AActor* DamageInstigator)
{
	if (MobaCombat::IsDead(this))
	{
		return;
	}

	AbilitySystemComponent->AddLooseGameplayTag(MobaTags::State_Dead, 1, EGameplayTagReplicationState::TagOnly);
	StopActing();

	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	SetLifeSpan(0.5f);

	if (AMobaGameMode* GameMode = GetWorld()->GetAuthGameMode<AMobaGameMode>())
	{
		GameMode->HandleMinionKilled(this, DamageInstigator);
	}
}
