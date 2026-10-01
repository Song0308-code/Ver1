#include "Combat/MobaCombatComponent.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystem/MobaDamageExecution.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Combat/MobaCombatLibrary.h"
#include "Combat/MobaProjectile.h"
#include "Game/MobaGameplayTags.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** 쫓아가는 동안 이동 명령을 다시 내리는 간격(초). */
	constexpr double ChaseRepathInterval = 0.2;

	/** 사거리 끝에 걸치지 않도록 조금 더 안쪽까지 들어간다. */
	constexpr float ChaseRangeFraction = 0.9f;

	/** 서버에서 이 속도 이하일 때만 공격을 시작한다. */
	constexpr float MaxSpeedToStartAttack = 20.0f;
}

UMobaCombatComponent::UMobaCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	ProjectileClass = AMobaProjectile::StaticClass();
	DamageEffectClass = UMobaDamageEffect::StaticClass();
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

void UMobaCombatComponent::SetAttackTarget(AActor* NewTarget)
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
		UpdateChase();
	}

	if (GetOwner()->HasAuthority())
	{
		UpdateAttack();
	}
}

void UMobaCombatComponent::UpdateChase()
{
	if (!AttackTarget)
	{
		return;
	}

	AController* Controller = GetOwnerController();
	if (!Controller)
	{
		return;
	}

	if (MobaCombat::IsDead(GetOwner()) || !MobaCombat::IsValidEnemyTarget(GetOwner(), AttackTarget))
	{
		if (bChasing)
		{
			Controller->StopMovement();
		}
		AttackTarget = nullptr;
		bChasing = false;
		return;
	}

	const float Distance = MobaCombat::GetDistanceToTargetEdge(GetOwner(), AttackTarget);
	if (Distance > GetAttackRange() * ChaseRangeFraction)
	{
		const double Now = GetWorld()->GetTimeSeconds();
		if (!bChasing || Now - LastChaseTime >= ChaseRepathInterval)
		{
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(Controller, AttackTarget->GetActorLocation());
			LastChaseTime = Now;
			bChasing = true;
		}
	}
	else if (bChasing)
	{
		Controller->StopMovement();
		bChasing = false;
	}
}

void UMobaCombatComponent::UpdateAttack()
{
	if (!AttackTarget)
	{
		bWindingUp = false;
		return;
	}

	if (MobaCombat::IsDead(GetOwner()) || !MobaCombat::IsValidEnemyTarget(GetOwner(), AttackTarget))
	{
		AttackTarget = nullptr;
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
	UAbilitySystemComponent* TargetAbilitySystem = MobaCombat::GetAbilitySystem(Target);
	if (!SourceAbilitySystem || !TargetAbilitySystem || !DamageEffectClass)
	{
		return;
	}

	FGameplayEffectContextHandle Context = SourceAbilitySystem->MakeEffectContext();
	Context.AddInstigator(GetOwner(), GetOwner());

	const FGameplayEffectSpecHandle Spec = SourceAbilitySystem->MakeOutgoingSpec(DamageEffectClass, 1.0f, Context);
	if (!Spec.IsValid())
	{
		return;
	}

	const float AttackDamage = SourceAbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetAttackDamageAttribute());
	Spec.Data->SetSetByCallerMagnitude(MobaTags::Data_Damage_Physical, AttackDamage);

	if (bRangedAttack && ProjectileClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = Cast<APawn>(GetOwner());
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AMobaProjectile* Projectile = GetWorld()->SpawnActor<AMobaProjectile>(ProjectileClass, GetOwner()->GetActorLocation(), GetOwner()->GetActorRotation(), SpawnParams))
		{
			Projectile->InitHoming(Target, Spec, ProjectileSpeed);
		}
	}
	else
	{
		SourceAbilitySystem->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetAbilitySystem);
	}
}
