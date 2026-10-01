#include "AbilitySystem/Abilities/MobaAbilityArchetypes.h"
#include "AbilitySystem/MobaAbilityLibrary.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaCharacter.h"
#include "Combat/MobaAreaIndicator.h"
#include "Combat/MobaCombatLibrary.h"
#include "Combat/MobaSkillshotProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Game/MobaGameplayTags.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

namespace
{
	float GetAbilityPower(const UAbilitySystemComponent* AbilitySystem)
	{
		return AbilitySystem ? AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetAbilityPowerAttribute()) : 0.0f;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Skillshot

UMobaAbility_Skillshot::UMobaAbility_Skillshot()
{
	DisplayName = NSLOCTEXT("Moba", "SkillshotName", "직선 투사체");
	TargetingMode = EMobaTargetingMode::Direction;
	CastRange = 1100.0f;
	ManaCost = 50.0f;
	CooldownDuration = 6.0f;

	ProjectileClass = AMobaSkillshotProjectile::StaticClass();
}

void UMobaAbility_Skillshot::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();

	FVector Direction = TargetLocation - Caster->GetActorLocation();
	Direction.Z = 0.0;
	if (!Direction.Normalize())
	{
		Direction = Caster->GetActorForwardVector();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Caster;
	SpawnParams.Instigator = Caster;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (AMobaSkillshotProjectile* Projectile = GetWorld()->SpawnActor<AMobaSkillshotProjectile>(ProjectileClass, Caster->GetActorLocation(), Direction.Rotation(), SpawnParams))
	{
		TArray<FGameplayEffectSpecHandle> HitEffects;

		const float Damage = ScaleByLevel(BaseDamage, DamagePerLevel) + GetAbilityPower(AbilitySystem) * AbilityPowerRatio;
		HitEffects.Add(MobaAbility::MakeDamageSpec(AbilitySystem, Caster, 0.0f, Damage, 0.0f));

		if (SlowMoveSpeedMultiplier < 1.0f && SlowDuration > 0.0f)
		{
			HitEffects.Add(MobaAbility::MakeStatModSpec(AbilitySystem, Caster, SlowDuration, SlowMoveSpeedMultiplier, 1.0f, MobaTags::State_Slowed));
		}

		Projectile->Launch(Caster, Direction, ProjectileSpeed, CastRange, ProjectileRadius, HitEffects);
	}

	FinishCast();
}

// ---------------------------------------------------------------------------------------------------------------------
// Self buff

UMobaAbility_SelfBuff::UMobaAbility_SelfBuff()
{
	DisplayName = NSLOCTEXT("Moba", "SelfBuffName", "가속");
	TargetingMode = EMobaTargetingMode::None;
	ManaCost = 40.0f;
	CooldownDuration = 12.0f;
}

void UMobaAbility_SelfBuff::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	MobaAbility::ApplySpecToTarget(MobaAbility::MakeStatModSpec(AbilitySystem, Caster, BuffDuration, MoveSpeedMultiplier, AttackSpeedMultiplier, MobaTags::State_Hasted), Caster);

	FinishCast();
}

// ---------------------------------------------------------------------------------------------------------------------
// Blink

UMobaAbility_Blink::UMobaAbility_Blink()
{
	DisplayName = NSLOCTEXT("Moba", "BlinkName", "점멸");
	TargetingMode = EMobaTargetingMode::Location;
	CastRange = 450.0f;
	ManaCost = 60.0f;
	CooldownDuration = 10.0f;
}

bool UMobaAbility_Blink::ResolveTarget(const AMobaCharacter* Caster, FVector& InOutTargetLocation) const
{
	Super::ResolveTarget(Caster, InOutTargetLocation);

	const UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Caster->GetWorld());
	if (!NavSystem)
	{
		return false;
	}

	// 도착 지점이 벽 속이면 조금씩 당겨 가며 걸을 수 있는 곳을 찾는다.
	const FVector CasterLocation = Caster->GetActorLocation();
	const FVector Offset = InOutTargetLocation - CasterLocation;
	for (const float Fraction : { 1.0f, 0.8f, 0.6f, 0.4f })
	{
		FNavLocation NavLocation;
		if (NavSystem->ProjectPointToNavigation(CasterLocation + Offset * Fraction, NavLocation, NavProjectExtent))
		{
			InOutTargetLocation = NavLocation.Location + FVector(0.0, 0.0, Caster->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
			return true;
		}
	}

	return false;
}

void UMobaAbility_Blink::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	FVector Facing = TargetLocation - Caster->GetActorLocation();
	Facing.Z = 0.0;
	const FRotator Rotation = Facing.IsNearlyZero() ? Caster->GetActorRotation() : Facing.Rotation();

	Caster->TeleportChampion(TargetLocation, Rotation);

	FinishCast();
}

// ---------------------------------------------------------------------------------------------------------------------
// Ground AoE

UMobaAbility_GroundAoE::UMobaAbility_GroundAoE()
{
	DisplayName = NSLOCTEXT("Moba", "GroundAoEName", "범위 폭발");
	TargetingMode = EMobaTargetingMode::Location;
	CastRange = 800.0f;
	ManaCost = 100.0f;
	CooldownDuration = 40.0f;
	bUltimate = true;

	IndicatorClass = AMobaAreaIndicator::StaticClass();
}

void UMobaAbility_GroundAoE::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	// 지면 높이에 맞춘다.
	FVector GroundLocation = TargetLocation;
	GroundLocation.Z = Caster->GetActorLocation().Z - Caster->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	if (IndicatorClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Caster;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AMobaAreaIndicator* Indicator = GetWorld()->SpawnActor<AMobaAreaIndicator>(IndicatorClass, GroundLocation, FRotator::ZeroRotator, SpawnParams))
		{
			Indicator->Show(Radius, Delay);
		}
	}

	if (Delay > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(DetonateTimer, FTimerDelegate::CreateUObject(this, &UMobaAbility_GroundAoE::Detonate, GroundLocation), Delay, false);
	}
	else
	{
		Detonate(GroundLocation);
	}
}

void UMobaAbility_GroundAoE::Detonate(FVector Location)
{
	AMobaCharacter* Caster = Cast<AMobaCharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();

	if (Caster && AbilitySystem)
	{
		TArray<AActor*> Enemies;
		MobaAbility::FindEnemiesInRadius(Caster, Location, Radius, Enemies);

		const float Damage = ScaleByLevel(BaseDamage, DamagePerLevel) + GetAbilityPower(AbilitySystem) * AbilityPowerRatio;
		for (AActor* Enemy : Enemies)
		{
			MobaAbility::ApplySpecToTarget(MobaAbility::MakeDamageSpec(AbilitySystem, Caster, 0.0f, Damage, 0.0f), Enemy);

			// 이 피해로 죽은 대상에게는 기절을 남기지 않는다.
			if (StunDuration > 0.0f && !MobaCombat::IsDead(Enemy))
			{
				MobaAbility::ApplySpecToTarget(MobaAbility::MakeStunSpec(AbilitySystem, Caster, StunDuration), Enemy);
			}
		}
	}

	FinishCast();
}
