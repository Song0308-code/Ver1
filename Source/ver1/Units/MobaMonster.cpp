#include "Units/MobaMonster.h"
#include "Game/MobaVisuals.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaCharacter.h"
#include "Combat/MobaCombatComponent.h"
#include "Combat/MobaCombatEvents.h"
#include "Combat/MobaCombatLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Game/MobaGameMode.h"
#include "Game/MobaGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Map/MobaJungleCamp.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float BaseCapsuleRadius = 40.0f;
	constexpr float BaseCapsuleHalfHeight = 55.0f;
	constexpr float AIInterval = 0.25f;

	/** 제자리에서 이 거리 안이면 돌아온 것으로 본다. */
	constexpr float HomeArrivalDistance = 90.0f;
}

AMobaMonster::AMobaMonster()
{
	PrimaryActorTick.bCanEverTick = false;

	// 미니맵과 시야 계산에 필요하므로 거리와 무관하게 복제한다.
	bAlwaysRelevant = true;

	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(BaseCapsuleRadius, BaseCapsuleHalfHeight);

	bUseControllerRotationYaw = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.0, 720.0, 0.0);
	Movement->MaxAcceleration = 4096.0f;
	Movement->BrakingDecelerationWalking = 4096.0f;

	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
	PlaceholderMesh->SetupAttachment(RootComponent);
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		PlaceholderMesh->SetStaticMesh(SphereMesh.Object);
		MobaVisuals::UseColorableMaterial(PlaceholderMesh);
	}

	CombatComponent = CreateDefaultSubobject<UMobaCombatComponent>(TEXT("CombatComponent"));
	CombatComponent->ConfigureProjectileVisual(0.3f, FVector::ZeroVector);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UMobaAttributeSet>(TEXT("AttributeSet"));
}

void AMobaMonster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMobaMonster, VisualScale);
}

void AMobaMonster::InitMonster(AMobaJungleCamp* InCamp, const FMobaMonsterConfig& InConfig, const FVector& InHomeLocation)
{
	Camp = InCamp;
	Config = InConfig;
	HomeLocation = InHomeLocation;
	VisualScale = FMath::Max(InConfig.VisualScale, 0.1f);

	ApplyAppearance();
}

void AMobaMonster::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	ApplyAppearance();

	if (HasAuthority())
	{
		AttributeSet->InitFromBaseStats(Config.Stats);
		AttributeSet->OnOutOfHealth.AddUObject(this, &AMobaMonster::HandleOutOfHealth);
		GetCharacterMovement()->MaxWalkSpeed = Config.Stats.MoveSpeed;
		CombatComponent->ConfigureBasicAttack(Config.bRanged, Config.ProjectileSpeed);

		if (UMobaCombatEvents* Events = UMobaCombatEvents::Get(this))
		{
			DamageDealtHandle = Events->OnDamageDealt.AddUObject(this, &AMobaMonster::HandleDamageDealt);
		}

		GetWorldTimerManager().SetTimer(AITimer, this, &AMobaMonster::UpdateAI, AIInterval, true, FMath::FRandRange(0.05f, AIInterval));
	}
}

void AMobaMonster::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMobaCombatEvents* Events = UMobaCombatEvents::Get(this))
	{
		Events->OnDamageDealt.Remove(DamageDealtHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AMobaMonster::OnRep_VisualScale()
{
	ApplyAppearance();
}

void AMobaMonster::ApplyAppearance()
{
	GetCapsuleComponent()->SetCapsuleSize(BaseCapsuleRadius * VisualScale, BaseCapsuleHalfHeight * VisualScale);

	// 엔진 기본 구의 지름은 100이다.
	PlaceholderMesh->SetRelativeScale3D(FVector(BaseCapsuleRadius * VisualScale / 50.0f));

	const FLinearColor Color = MobaTeam::GetTeamColor(EMobaTeam::Neutral);
	PlaceholderMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(Color.R, Color.G, Color.B));
}

void AMobaMonster::StopActing()
{
	bActing = false;
	GetWorldTimerManager().ClearTimer(AITimer);
	CombatComponent->SetAttackTarget(nullptr);
	if (AController* MonsterController = GetController())
	{
		MonsterController->StopMovement();
	}
}

void AMobaMonster::UpdateAI()
{
	if (!bActing || MobaCombat::IsDead(this))
	{
		return;
	}

	const float HomeDistance = FVector::Dist2D(GetActorLocation(), HomeLocation);

	if (const AActor* Target = CombatComponent->GetAttackTarget())
	{
		const bool bKeepFighting = MobaCombat::IsValidEnemyTarget(this, Target)
			&& HomeDistance <= LeashRange
			&& FVector::Dist2D(Target->GetActorLocation(), HomeLocation) <= LeashRange + CombatComponent->GetAttackRange();
		if (!bKeepFighting)
		{
			ReturnHome();
		}
		return;
	}

	if (HomeDistance > HomeArrivalDistance)
	{
		// 대상이 죽었거나 놓친 경우. 도착할 때까지 계속 걸어간다.
		ReturnHome();
	}
	else
	{
		bReturning = false;
	}

	const float MaxHealth = AbilitySystemComponent->GetNumericAttribute(UMobaAttributeSet::GetMaxHealthAttribute());
	const float Health = AbilitySystemComponent->GetNumericAttributeBase(UMobaAttributeSet::GetHealthAttribute());
	if (Health < MaxHealth)
	{
		AbilitySystemComponent->SetNumericAttributeBase(UMobaAttributeSet::GetHealthAttribute(), FMath::Min(Health + MaxHealth * IdleHealFractionPerSecond * AIInterval, MaxHealth));
	}
}

void AMobaMonster::ReturnHome()
{
	bReturning = true;
	CombatComponent->SetAttackTarget(nullptr);

	if (AAIController* MonsterController = Cast<AAIController>(GetController()))
	{
		MonsterController->MoveToLocation(HomeLocation, 30.0f, true, true, true);
	}
}

void AMobaMonster::HandleDamageDealt(AActor* DamageInstigator, AActor* Victim, float Damage)
{
	if (!bActing || bReturning || Config.bPassive || MobaCombat::IsDead(this) || CombatComponent->GetAttackTarget())
	{
		return;
	}

	// 나 또는 같은 캠프의 몬스터가 맞았을 때만 반응한다.
	const AMobaMonster* VictimMonster = Cast<AMobaMonster>(Victim);
	if (!VictimMonster || VictimMonster->Camp != Camp)
	{
		return;
	}

	if (MobaCombat::IsValidEnemyTarget(this, DamageInstigator))
	{
		CombatComponent->SetAttackTarget(DamageInstigator);
	}
}

void AMobaMonster::HandleOutOfHealth(AActor* DamageInstigator)
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
		GameMode->HandleMonsterKilled(this, DamageInstigator);
	}

	if (Camp)
	{
		Camp->HandleMonsterKilled(this, DamageInstigator);
	}
}
