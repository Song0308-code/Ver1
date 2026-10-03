#include "Combat/MobaCombatComponent.h"
#include "AbilitySystem/MobaAbilityLibrary.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Combat/MobaCombatLibrary.h"
#include "Combat/MobaProjectile.h"
#include "Game/MobaGameplayTags.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "NavigationSystem.h"
#include "Vision/MobaVisionSubsystem.h"

namespace
{
	/** 쫓아가는 동안 이동 명령을 다시 내리는 간격(초). */
	constexpr double ChaseRepathInterval = 0.2;

	/** 사거리 끝에 걸치지 않도록 조금 더 안쪽까지 들어간다. */
	constexpr float ChaseRangeFraction = 0.97f;

	/** 서버에서 이 속도 이하일 때만 공격을 시작한다. */
	constexpr float MaxSpeedToStartAttack = 20.0f;

	/** 주변의 적을 다시 찾는 간격(초). */
	constexpr double EnemyScanInterval = 0.15;

	/** 명령을 내린 직후에는 이 시간 동안 자동 공격 대상을 찾지 않는다. 막 걷기 시작한 순간을 가만히 선 것으로 오해하지 않기 위해서다. */
	constexpr double IdleDelayAfterOrder = 0.3;

	/** 이 속도 이하면 서 있는 것으로 본다. */
	constexpr float IdleSpeed = 5.0f;

	/** 공격 이동 목적지에 이만큼 가까워지면 도착한 것으로 본다. */
	constexpr float AttackMoveArrivalDistance = 100.0f;

	/** 공격 이동 중 이 시간 동안 움직이지 못하면 더 갈 수 없는 것으로 보고 끝낸다. */
	constexpr double AttackMoveStallTime = 0.75;
}

UMobaCombatComponent::UMobaCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	ProjectileClass = AMobaProjectile::StaticClass();
}

AController* UMobaCombatComponent::GetOwnerController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn ? OwnerPawn->GetController() : nullptr;
}

float UMobaCombatComponent::GetAttackRange() const
{
	const UAbilitySystemComponent* AbilitySystem = MobaCombat::GetAbilitySystem(GetOwner());
	return AbilitySystem ? AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetAttackRangeAttribute()) : 0.0f;
}

void UMobaCombatComponent::ConfigureBasicAttack(bool bInRanged, float InProjectileSpeed)
{
	bRangedAttack = bInRanged;
	ProjectileSpeed = InProjectileSpeed;
}

void UMobaCombatComponent::ConfigureProjectileVisual(float InVisualScale, const FVector& InSpawnOffset)
{
	ProjectileVisualScale = InVisualScale;
	ProjectileSpawnOffset = InSpawnOffset;
}

bool UMobaCombatComponent::CanOwnerAct() const
{
	const UAbilitySystemComponent* AbilitySystem = MobaCombat::GetAbilitySystem(GetOwner());
	return AbilitySystem
		&& !AbilitySystem->HasMatchingGameplayTag(MobaTags::State_Dead)
		&& !AbilitySystem->HasMatchingGameplayTag(MobaTags::State_Stunned)
		&& !MobaCombat::IsMatchOver(this);
}

bool UMobaCombatComponent::CanSeeTarget(const AActor* Target) const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetOwnerController());
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return true;
	}

	const UMobaVisionSubsystem* Vision = UMobaVisionSubsystem::Get(this);
	return !Vision || Vision->IsVisibleToLocalPlayer(Target);
}

// ---------------------------------------------------------------------------------------------------------------------
// 명령

void UMobaCombatComponent::SetAttackTarget(AActor* NewTarget)
{
	bAttackMoving = false;
	bAttackMoveWalking = false;
	bTargetAutoAcquired = false;
	LastOrderTime = GetWorld()->GetTimeSeconds();

	SetAttackTargetInternal(NewTarget);
}

void UMobaCombatComponent::StartAttackMove(const FVector& Destination)
{
	const double Now = GetWorld()->GetTimeSeconds();

	bAttackMoving = true;
	bAttackMoveWalking = false;
	bTargetAutoAcquired = false;
	AttackMoveDestination = Destination;
	LastOrderTime = Now;
	LastMovingTime = Now;
	LastScanTime = -1.0;

	SetAttackTargetInternal(nullptr);
}

void UMobaCombatComponent::SetAttackTargetInternal(AActor* NewTarget)
{
	if (NewTarget && !MobaCombat::IsValidEnemyTarget(GetOwner(), NewTarget))
	{
		NewTarget = nullptr;
	}

	if (AttackTarget == NewTarget)
	{
		return;
	}

	AttackTarget = NewTarget;
	bChasing = false;
	LastChaseTime = -1.0;
	bStopPending = NewTarget != nullptr;

	if (GetOwner()->HasAuthority())
	{
		bWindingUp = false;
	}
	else
	{
		ServerSetAttackTarget(NewTarget);
	}
}

void UMobaCombatComponent::ServerSetAttackTarget_Implementation(AActor* NewTarget)
{
	AttackTarget = MobaCombat::IsValidEnemyTarget(GetOwner(), NewTarget) ? NewTarget : nullptr;
	bWindingUp = false;
}

void UMobaCombatComponent::MulticastAttackStarted_Implementation(AActor* Target)
{
	OnAttackStarted.Broadcast(Target);
}

void UMobaCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn && OwnerPawn->IsLocallyControlled())
	{
		UpdateOrders();
	}

	if (GetOwner()->HasAuthority())
	{
		UpdateAttack();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// 조종하는 쪽: 대상 찾기와 추격

void UMobaCombatComponent::UpdateOrders()
{
	AController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	if (GetOwner()->GetVelocity().Size2D() > IdleSpeed)
	{
		LastMovingTime = GetWorld()->GetTimeSeconds();
	}

	if (AttackTarget)
	{
		UpdateChase(Controller);
	}
	else if (!CanOwnerAct())
	{
		bAttackMoving = false;
		bAttackMoveWalking = false;
	}
	else if (bAttackMoving)
	{
		UpdateAttackMove(Controller);
	}
	else if (bAutoAcquireWhenIdle)
	{
		UpdateIdleAutoAcquire();
	}
}

AActor* UMobaCombatComponent::FindNearestEnemy(float Range, bool bIncludeStructures, bool bIncludeNeutral) const
{
	// 가장자리까지의 거리로 판정하므로, 큰 대상도 놓치지 않게 검색 반경은 넉넉히 잡는다.
	TArray<AActor*> Enemies;
	MobaAbility::FindEnemiesInRadius(GetOwner(), GetOwner()->GetActorLocation(), Range + 400.0f, Enemies, bIncludeStructures);

	AActor* Nearest = nullptr;
	float NearestDistance = Range;
	for (AActor* Enemy : Enemies)
	{
		if (!bIncludeNeutral && MobaCombat::GetTeam(Enemy) == EMobaTeam::Neutral)
		{
			continue;
		}

		const float Distance = MobaCombat::GetDistanceToTargetEdge(GetOwner(), Enemy);
		if (Distance <= NearestDistance && CanSeeTarget(Enemy))
		{
			Nearest = Enemy;
			NearestDistance = Distance;
		}
	}
	return Nearest;
}

void UMobaCombatComponent::UpdateIdleAutoAcquire()
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastOrderTime < IdleDelayAfterOrder || Now - LastMovingTime < IdleDelayAfterOrder || Now - LastScanTime < EnemyScanInterval)
	{
		return;
	}
	LastScanTime = Now;

	// 귀환을 준비하는 동안에는 알아서 공격하지 않는다(공격하면 귀환이 취소된다).
	const UAbilitySystemComponent* OwnerAbilitySystem = MobaCombat::GetAbilitySystem(GetOwner());
	if (OwnerAbilitySystem && OwnerAbilitySystem->HasMatchingGameplayTag(MobaTags::State_Recalling))
	{
		return;
	}

	// 가만히 서 있을 때는 건물과 정글 몬스터를 알아서 치지 않는다.
	if (AActor* Enemy = FindNearestEnemy(GetAttackRange(), false, false))
	{
		bTargetAutoAcquired = true;
		SetAttackTargetInternal(Enemy);
	}
}

void UMobaCombatComponent::UpdateAttackMove(AController* Controller)
{
	const double Now = GetWorld()->GetTimeSeconds();

	if (Now - LastScanTime >= EnemyScanInterval)
	{
		LastScanTime = Now;
		if (AActor* Enemy = FindNearestEnemy(GetAttackRange() + AttackMoveAcquisitionBonus, true, true))
		{
			SetAttackTargetInternal(Enemy);
			return;
		}
	}

	if (FVector::Dist2D(GetOwner()->GetActorLocation(), AttackMoveDestination) <= AttackMoveArrivalDistance)
	{
		bAttackMoving = false;
		bAttackMoveWalking = false;
		return;
	}

	if (!bAttackMoveWalking)
	{
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(Controller, AttackMoveDestination);
		bAttackMoveWalking = true;
		LastMovingTime = Now;
	}
	else if (Now - LastMovingTime > AttackMoveStallTime)
	{
		// 더 갈 수 없는 곳이다.
		bAttackMoving = false;
		bAttackMoveWalking = false;
	}
}

void UMobaCombatComponent::UpdateChase(AController* Controller)
{
	if (MobaCombat::IsDead(GetOwner()) || !MobaCombat::IsValidEnemyTarget(GetOwner(), AttackTarget))
	{
		if (bChasing)
		{
			Controller->StopMovement();
		}

		// 서버도 같은 판단으로 대상을 비우므로 따로 알리지 않는다. 공격 이동 중이었다면 다음 틱에 다시 걷기 시작한다.
		AttackTarget = nullptr;
		bChasing = false;
		bAttackMoveWalking = false;
		bTargetAutoAcquired = false;
		return;
	}

	// 기절 중에는 쫓아가지 않는다. 풀리면 이어서 쫓아간다.
	const UAbilitySystemComponent* OwnerAbilitySystem = MobaCombat::GetAbilitySystem(GetOwner());
	if (OwnerAbilitySystem && OwnerAbilitySystem->HasMatchingGameplayTag(MobaTags::State_Stunned))
	{
		bChasing = false;
		return;
	}

	const float Range = GetAttackRange();
	const float Distance = MobaCombat::GetDistanceToTargetEdge(GetOwner(), AttackTarget);

	// 자동으로 잡은 대상은 사거리를 벗어나면 쫓아가지 않고 놓는다. 부쉬나 벽 뒤로 사라진 대상도 놓는다.
	if ((bTargetAutoAcquired && Distance > Range) || !CanSeeTarget(AttackTarget))
	{
		if (bChasing)
		{
			Controller->StopMovement();
		}

		bTargetAutoAcquired = false;
		bAttackMoveWalking = false;
		SetAttackTargetInternal(nullptr);
		return;
	}

	if (Distance > Range * ChaseRangeFraction)
	{
		const double Now = GetWorld()->GetTimeSeconds();
		if (!bChasing || Now - LastChaseTime >= ChaseRepathInterval)
		{
			// 건물처럼 걸을 수 없는 자리를 차지한 대상도 쫓아갈 수 있도록, 대상의 가장자리에서 내 쪽으로 조금 나온 지점을 목표로 삼는다.
			const FVector OwnerLocation = GetOwner()->GetActorLocation();
			FVector ToOwner = OwnerLocation - AttackTarget->GetActorLocation();
			ToOwner.Z = 0.0;
			FVector Goal = AttackTarget->GetActorLocation() + ToOwner.GetSafeNormal() * (AttackTarget->GetSimpleCollisionRadius() + 60.0f);
			Goal.Z = OwnerLocation.Z;

			const UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
			FNavLocation NavLocation;
			if (NavSystem && NavSystem->ProjectPointToNavigation(Goal, NavLocation, FVector(200.0, 200.0, 300.0)))
			{
				Goal = NavLocation.Location;
			}

			UAIBlueprintHelperLibrary::SimpleMoveToLocation(Controller, Goal);
			LastChaseTime = Now;
			bChasing = true;
		}
	}
	else if (bChasing || bAttackMoveWalking || bStopPending)
	{
		// 사거리 안이다. 걷던 중이었다면 감속 없이 그 자리에 바로 멈춰야 서버가 곧바로 공격을 시작한다.
		Controller->StopMovement();
		if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
		{
			if (UPawnMovementComponent* Movement = OwnerPawn->GetMovementComponent())
			{
				Movement->StopMovementImmediately();
			}
		}

		bChasing = false;
		bAttackMoveWalking = false;
		bStopPending = false;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// 서버: 공격 주기

void UMobaCombatComponent::UpdateAttack()
{
	if (!AttackTarget)
	{
		bWindingUp = false;
		return;
	}

	if (MobaCombat::IsDead(GetOwner()) || MobaCombat::IsMatchOver(this) || !MobaCombat::IsValidEnemyTarget(GetOwner(), AttackTarget))
	{
		AttackTarget = nullptr;
		bWindingUp = false;
		return;
	}

	// 기절 중에는 선딜레이가 끊기고 새 공격도 시작하지 못한다. 대상은 유지한다.
	const UAbilitySystemComponent* OwnerAbilitySystem = MobaCombat::GetAbilitySystem(GetOwner());
	if (OwnerAbilitySystem && OwnerAbilitySystem->HasMatchingGameplayTag(MobaTags::State_Stunned))
	{
		bWindingUp = false;
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();

	if (bWindingUp)
	{
		if (Now >= WindupEndTime)
		{
			bWindingUp = false;
			FireAttack(AttackTarget);
		}
		return;
	}

	if (Now < NextAttackTime)
	{
		return;
	}

	if (MobaCombat::GetDistanceToTargetEdge(GetOwner(), AttackTarget) > GetAttackRange() + ServerRangeLeeway)
	{
		return;
	}

	if (GetOwner()->GetVelocity().Size2D() > MaxSpeedToStartAttack)
	{
		return;
	}

	const UAbilitySystemComponent* AbilitySystem = MobaCombat::GetAbilitySystem(GetOwner());
	const float AttackSpeed = AbilitySystem ? AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetAttackSpeedAttribute()) : 0.0f;
	if (AttackSpeed <= 0.0f)
	{
		return;
	}

	const double AttackPeriod = 1.0 / AttackSpeed;
	bWindingUp = true;
	WindupEndTime = Now + AttackPeriod * WindupFraction;
	NextAttackTime = Now + AttackPeriod;

	MulticastAttackStarted(AttackTarget);
}

void UMobaCombatComponent::FireAttack(AActor* Target)
{
	UAbilitySystemComponent* SourceAbilitySystem = MobaCombat::GetAbilitySystem(GetOwner());
	if (!SourceAbilitySystem || !MobaCombat::GetAbilitySystem(Target) || !ProjectileClass)
	{
		return;
	}

	// 레드 버프는 기본 공격에 고정 피해와 둔화를 더한다. 건물에는 적용하지 않는다.
	const bool bRedBuff = SourceAbilitySystem->HasMatchingGameplayTag(MobaTags::Buff_Red) && Target->IsA<APawn>();

	const float AttackDamage = SourceAbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetAttackDamageAttribute());
	const FGameplayEffectSpecHandle Spec = MobaAbility::MakeDamageSpec(SourceAbilitySystem, GetOwner(), AttackDamage, 0.0f, bRedBuff ? RedBuffTrueDamage : 0.0f);
	if (!Spec.IsValid())
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Cast<APawn>(GetOwner());
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector SpawnLocation = GetOwner()->GetActorLocation() + ProjectileSpawnOffset;
	if (AMobaProjectile* Projectile = GetWorld()->SpawnActor<AMobaProjectile>(ProjectileClass, SpawnLocation, GetOwner()->GetActorRotation(), SpawnParams))
	{
		Projectile->InitHoming(Target, Spec, bRangedAttack ? ProjectileSpeed : MeleeProjectileSpeed, ProjectileVisualScale, MobaCombat::GetTeam(GetOwner()));
		if (bRedBuff)
		{
			Projectile->AddOnHitEffect(MobaAbility::MakeStatModSpec(SourceAbilitySystem, GetOwner(), RedBuffSlowDuration, RedBuffSlowMultiplier, 1.0f, MobaTags::State_Slowed));
		}
	}
}
