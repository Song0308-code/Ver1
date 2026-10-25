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
#include "Game/MobaEffects.h"
#include "Game/MobaGameplayTags.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

namespace
{
	float GetAbilityPower(const UAbilitySystemComponent* AbilitySystem)
	{
		return AbilitySystem ? AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetAbilityPowerAttribute()) : 0.0f;
	}

	float GetAttackDamage(const UAbilitySystemComponent* AbilitySystem)
	{
		return AbilitySystem ? AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetAttackDamageAttribute()) : 0.0f;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Skillshot

UMobaAbility_Skillshot::UMobaAbility_Skillshot()
{
	DisplayName = NSLOCTEXT("Moba", "SkillshotName", "직선 투사체");
	Description = NSLOCTEXT("Moba", "SkillshotDesc", "커서 방향으로 투사체를 쏩니다. 처음 맞은 적에게 마법 피해를 주고 2초 동안 30% 둔화시킵니다.");
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
	Description = NSLOCTEXT("Moba", "SelfBuffDesc", "4초 동안 이동 속도가 30%, 공격 속도가 40% 오릅니다.");
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
	Description = NSLOCTEXT("Moba", "BlinkDesc", "커서 쪽으로 짧게 순간이동합니다. 벽을 넘을 수 있습니다.");
	TargetingMode = EMobaTargetingMode::Location;
	CastRange = 450.0f;
	ManaCost = 60.0f;
	CooldownDuration = 10.0f;
	bMovementAbility = true;
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
	Description = NSLOCTEXT("Moba", "GroundAoEDesc", "지정한 곳이 잠시 뒤 폭발해 범위 안의 적에게 마법 피해를 주고 기절시킵니다.");
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

		const float Damage = ScaleByLevel(BaseDamage, DamagePerLevel) + GetAbilityPower(AbilitySystem) * AbilityPowerRatio + GetAttackDamage(AbilitySystem) * AttackDamageRatio;
		for (AActor* Enemy : Enemies)
		{
			MobaAbility::ApplySpecToTarget(MobaAbility::MakeDamageSpec(AbilitySystem, Caster, bPhysicalDamage ? Damage : 0.0f, bPhysicalDamage ? 0.0f : Damage, 0.0f), Enemy);

			if (SlowMoveSpeedMultiplier < 1.0f && SlowDuration > 0.0f && !MobaCombat::IsDead(Enemy))
			{
				MobaAbility::ApplySpecToTarget(MobaAbility::MakeStatModSpec(AbilitySystem, Caster, SlowDuration, SlowMoveSpeedMultiplier, 1.0f, MobaTags::State_Slowed), Enemy);
			}

			// 이 피해로 죽은 대상에게는 기절 등을 남기지 않는다.
			if (MobaCombat::IsDead(Enemy))
			{
				continue;
			}
			if (StunDuration > 0.0f)
			{
				MobaAbility::ApplySpecToTarget(MobaAbility::MakeStunSpec(AbilitySystem, Caster, StunDuration), Enemy);
			}
			if (AirborneDuration > 0.0f)
			{
				MobaAbility::ApplySpecToTarget(MobaAbility::MakeCrowdControlSpec(AbilitySystem, Caster, AirborneDuration, MobaTags::State_Airborne), Enemy);
			}
			if (RootDuration > 0.0f)
			{
				MobaAbility::ApplySpecToTarget(MobaAbility::MakeCrowdControlSpec(AbilitySystem, Caster, RootDuration, MobaTags::State_Rooted), Enemy);
			}
		}

		OnDetonated(Caster, Location);
	}

	FinishCast();
}

// ---------------------------------------------------------------------------------------------------------------------
// Dash strike

UMobaAbility_DashStrike::UMobaAbility_DashStrike()
{
	DisplayName = NSLOCTEXT("Moba", "DashStrikeName", "돌진 베기");
	Description = NSLOCTEXT("Moba", "DashStrikeDesc", "커서 쪽으로 돌진해 도착 지점 주변의 적에게 물리 피해를 주고 0.75초 동안 침묵시킵니다.");
	TargetingMode = EMobaTargetingMode::Location;
	CastRange = 450.0f;
	ManaCost = 45.0f;
	CooldownDuration = 7.0f;
	SilenceDuration = 0.75f;
}

void UMobaAbility_DashStrike::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	FVector Facing = TargetLocation - Caster->GetActorLocation();
	Facing.Z = 0.0;
	const FRotator Rotation = Facing.IsNearlyZero() ? Caster->GetActorRotation() : Facing.Rotation();

	Caster->TeleportChampion(TargetLocation, Rotation);

	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();

	TArray<AActor*> Enemies;
	MobaAbility::FindEnemiesInRadius(Caster, TargetLocation, Radius, Enemies);

	const float Damage = ScaleByLevel(BaseDamage, DamagePerLevel) + GetAttackDamage(AbilitySystem) * AttackDamageRatio;
	for (AActor* Enemy : Enemies)
	{
		MobaAbility::ApplySpecToTarget(MobaAbility::MakeDamageSpec(AbilitySystem, Caster, Damage, 0.0f, 0.0f), Enemy);
		if (SilenceDuration > 0.0f && !MobaCombat::IsDead(Enemy))
		{
			MobaAbility::ApplySpecToTarget(MobaAbility::MakeCrowdControlSpec(AbilitySystem, Caster, SilenceDuration, MobaTags::State_Silenced), Enemy);
		}
	}

	FinishCast();
}

// ---------------------------------------------------------------------------------------------------------------------
// Self heal

UMobaAbility_SelfHeal::UMobaAbility_SelfHeal()
{
	DisplayName = NSLOCTEXT("Moba", "SelfHealName", "재정비");
	Description = NSLOCTEXT("Moba", "SelfHealDesc", "체력을 회복합니다. 잃은 체력이 많을수록 더 많이 회복합니다.");
	TargetingMode = EMobaTargetingMode::None;
	ManaCost = 60.0f;
	CooldownDuration = 14.0f;
}

void UMobaAbility_SelfHeal::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	if (UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo())
	{
		const float Health = AbilitySystem->GetNumericAttributeBase(UMobaAttributeSet::GetHealthAttribute());
		const float MaxHealth = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxHealthAttribute());
		const float Heal = ScaleByLevel(BaseHeal, HealPerLevel) + (MaxHealth - Health) * MissingHealthFraction;

		AbilitySystem->SetNumericAttributeBase(UMobaAttributeSet::GetHealthAttribute(), FMath::Min(Health + Heal, MaxHealth));
	}

	FinishCast();
}

// ---------------------------------------------------------------------------------------------------------------------
// Ground slam / Frenzy

UMobaAbility_GroundSlam::UMobaAbility_GroundSlam()
{
	DisplayName = NSLOCTEXT("Moba", "GroundSlamName", "지면 강타");
	Description = NSLOCTEXT("Moba", "GroundSlamDesc", "주변의 적에게 물리 피해를 주고 0.75초 동안 공중에 띄웁니다.");
	TargetingMode = EMobaTargetingMode::None;
	CastRange = 0.0f;
	ManaCost = 50.0f;
	CooldownDuration = 9.0f;
	bUltimate = false;

	Radius = 350.0f;
	Delay = 0.0f;
	BaseDamage = 60.0f;
	DamagePerLevel = 35.0f;
	AbilityPowerRatio = 0.0f;
	AttackDamageRatio = 0.5f;
	bPhysicalDamage = true;
	StunDuration = 0.0f;
	AirborneDuration = 0.75f;
	SlowMoveSpeedMultiplier = 1.0f;
	SlowDuration = 0.0f;
}

UMobaAbility_Frenzy::UMobaAbility_Frenzy()
{
	DisplayName = NSLOCTEXT("Moba", "FrenzyName", "전투 광분");
	Description = NSLOCTEXT("Moba", "FrenzyDesc", "8초 동안 이동 속도가 35%, 공격 속도가 80% 오릅니다.");
	ManaCost = 100.0f;
	CooldownDuration = 70.0f;
	bUltimate = true;

	BuffDuration = 8.0f;
	MoveSpeedMultiplier = 1.35f;
	AttackSpeedMultiplier = 1.8f;
}

// ---------------------------------------------------------------------------------------------------------------------
// 미스틱

UMobaAbility_ArcaneBolt::UMobaAbility_ArcaneBolt()
{
	DisplayName = NSLOCTEXT("Moba", "ArcaneBoltName", "마력 화살");
	Description = NSLOCTEXT("Moba", "ArcaneBoltDesc", "커서 방향으로 빠른 마력 화살을 쏩니다. 처음 맞은 적에게 큰 마법 피해를 줍니다.");
	CastRange = 1150.0f;
	ManaCost = 55.0f;
	CooldownDuration = 5.0f;

	ProjectileSpeed = 2000.0f;
	ProjectileRadius = 50.0f;
	BaseDamage = 90.0f;
	DamagePerLevel = 50.0f;
	AbilityPowerRatio = 0.75f;
	SlowMoveSpeedMultiplier = 1.0f;
}

UMobaAbility_GravityField::UMobaAbility_GravityField()
{
	DisplayName = NSLOCTEXT("Moba", "GravityFieldName", "중력장");
	Description = NSLOCTEXT("Moba", "GravityFieldDesc", "지정한 곳에 잠시 뒤 중력장이 생겨 범위 안의 적에게 마법 피해를 주고 1.25초 동안 속박합니다.");
	CastRange = 850.0f;
	ManaCost = 70.0f;
	CooldownDuration = 11.0f;
	bUltimate = false;

	Radius = 250.0f;
	Delay = 0.75f;
	BaseDamage = 70.0f;
	DamagePerLevel = 40.0f;
	AbilityPowerRatio = 0.5f;
	StunDuration = 0.0f;
	RootDuration = 1.25f;
	SlowMoveSpeedMultiplier = 1.0f;
	SlowDuration = 0.0f;
}

UMobaAbility_PhaseShift::UMobaAbility_PhaseShift()
{
	DisplayName = NSLOCTEXT("Moba", "PhaseShiftName", "위상 이동");
	Description = NSLOCTEXT("Moba", "PhaseShiftDesc", "커서 쪽으로 짧게 순간이동합니다. 벽을 넘을 수 있습니다.");
	CastRange = 400.0f;
	ManaCost = 50.0f;
	CooldownDuration = 14.0f;
}

UMobaAbility_Meteor::UMobaAbility_Meteor()
{
	DisplayName = NSLOCTEXT("Moba", "MeteorName", "운석 낙하");
	Description = NSLOCTEXT("Moba", "MeteorDesc", "넓은 범위에 1초 뒤 운석이 떨어져 큰 마법 피해를 주고 1초 동안 기절시킵니다.");
	CastRange = 1200.0f;
	ManaCost = 120.0f;
	CooldownDuration = 70.0f;
	bUltimate = true;

	Radius = 400.0f;
	Delay = 1.0f;
	BaseDamage = 250.0f;
	DamagePerLevel = 125.0f;
	AbilityPowerRatio = 0.9f;
	StunDuration = 1.0f;
}

// ---------------------------------------------------------------------------------------------------------------------
// 섀도우

UMobaAbility_ShadowStep::UMobaAbility_ShadowStep()
{
	DisplayName = NSLOCTEXT("Moba", "ShadowStepName", "그림자 찌르기");
	Description = NSLOCTEXT("Moba", "ShadowStepDesc", "커서 근처의 적 등 뒤로 순간이동해 물리 피해를 줍니다.");
	TargetingMode = EMobaTargetingMode::Location;
	CastRange = 600.0f;
	ManaCost = 40.0f;
	CooldownDuration = 8.0f;
	bMovementAbility = true;
}

bool UMobaAbility_ShadowStep::ResolveTarget(const AMobaCharacter* Caster, FVector& InOutTargetLocation) const
{
	PendingTarget = MobaAbility::PickEnemyNearCursor(Caster, InOutTargetLocation, TargetPickRadius, CastRange, [](const AActor*) { return true; });
	if (!PendingTarget.IsValid())
	{
		return false;
	}
	InOutTargetLocation = PendingTarget->GetActorLocation();
	return true;
}

void UMobaAbility_ShadowStep::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	AActor* Target = PendingTarget.Get();
	PendingTarget.Reset();
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	if (Target && AbilitySystem)
	{
		// 시전자에서 대상 쪽으로 대상을 지나 조금 더 간 곳(등 뒤)에 내린다. 걸을 수 없는 곳이면 대상 위치 근처로.
		FVector Direction = Target->GetActorLocation() - Caster->GetActorLocation();
		Direction.Z = 0.0;
		Direction = Direction.GetSafeNormal();
		FVector Destination = Target->GetActorLocation() + Direction * 90.0f;
		Destination.Z = Caster->GetActorLocation().Z;

		if (const UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Caster->GetWorld()))
		{
			FNavLocation NavLocation;
			if (NavSystem->ProjectPointToNavigation(Destination, NavLocation, FVector(150.0, 150.0, 300.0)))
			{
				Destination = NavLocation.Location + FVector(0.0, 0.0, Caster->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
			}
		}

		Caster->TeleportChampion(Destination, (-Direction).Rotation());

		const float Damage = ScaleByLevel(BaseDamage, DamagePerLevel) + GetAttackDamage(AbilitySystem) * AttackDamageRatio;
		MobaAbility::ApplySpecToTarget(MobaAbility::MakeDamageSpec(AbilitySystem, Caster, Damage, 0.0f, 0.0f), Target);
	}

	FinishCast();
}

UMobaAbility_ShadowCloak::UMobaAbility_ShadowCloak()
{
	DisplayName = NSLOCTEXT("Moba", "ShadowCloakName", "그림자 숨기");
	Description = NSLOCTEXT("Moba", "ShadowCloakDesc", "3초 동안 은신하고 이동 속도가 20% 오릅니다. 공격하거나 스킬을 쓰면 풀립니다. 적 제어 와드 근처에서는 드러납니다.");
	TargetingMode = EMobaTargetingMode::None;
	ManaCost = 60.0f;
	CooldownDuration = 18.0f;
}

void UMobaAbility_ShadowCloak::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	MobaAbility::ApplySpecToTarget(MobaAbility::MakeStatModSpec(AbilitySystem, Caster, StealthDuration, MoveSpeedMultiplier, 1.0f, MobaTags::State_Stealthed), Caster);
	FinishCast();
}

UMobaAbility_SmokeBomb::UMobaAbility_SmokeBomb()
{
	DisplayName = NSLOCTEXT("Moba", "SmokeBombName", "연막탄");
	Description = NSLOCTEXT("Moba", "SmokeBombDesc", "지정한 곳에 연막을 터뜨려 범위 안의 적에게 물리 피해를 주고 1.5초 동안 40% 둔화시킵니다.");
	CastRange = 700.0f;
	ManaCost = 50.0f;
	CooldownDuration = 10.0f;
	bUltimate = false;

	Radius = 250.0f;
	Delay = 0.0f;
	BaseDamage = 50.0f;
	DamagePerLevel = 30.0f;
	AbilityPowerRatio = 0.0f;
	AttackDamageRatio = 0.4f;
	bPhysicalDamage = true;
	StunDuration = 0.0f;
	SlowMoveSpeedMultiplier = 0.6f;
	SlowDuration = 1.5f;
}

UMobaAbility_Execute::UMobaAbility_Execute()
{
	DisplayName = NSLOCTEXT("Moba", "ExecuteName", "처형");
	Description = NSLOCTEXT("Moba", "ExecuteDesc", "커서 근처의 적 챔피언에게 큰 물리 피해를 주고, 대상이 잃은 체력의 30%만큼 추가 물리 피해를 줍니다.");
	TargetingMode = EMobaTargetingMode::Location;
	CastRange = 400.0f;
	ManaCost = 100.0f;
	CooldownDuration = 60.0f;
	bUltimate = true;
}

bool UMobaAbility_Execute::ResolveTarget(const AMobaCharacter* Caster, FVector& InOutTargetLocation) const
{
	PendingTarget = MobaAbility::PickEnemyNearCursor(Caster, InOutTargetLocation, TargetPickRadius, CastRange, [](const AActor* Candidate) { return Candidate && Candidate->IsA<AMobaCharacter>(); });
	if (!PendingTarget.IsValid())
	{
		return false;
	}
	InOutTargetLocation = PendingTarget->GetActorLocation();
	return true;
}

void UMobaAbility_Execute::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	AActor* Target = PendingTarget.Get();
	PendingTarget.Reset();
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	const UAbilitySystemComponent* TargetAbilitySystem = MobaCombat::GetAbilitySystem(Target);
	if (Target && AbilitySystem && TargetAbilitySystem)
	{
		const float MissingHealth = TargetAbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxHealthAttribute()) - TargetAbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetHealthAttribute());
		const float Damage = ScaleByLevel(BaseDamage, DamagePerLevel) + GetAttackDamage(AbilitySystem) * AttackDamageRatio + FMath::Max(MissingHealth, 0.0f) * MissingHealthRatio;
		MobaAbility::ApplySpecToTarget(MobaAbility::MakeDamageSpec(AbilitySystem, Caster, Damage, 0.0f, 0.0f), Target);
		MobaEffects::Play(Caster, EMobaEffect::Hit, MobaCombat::GetAimLocation(Target), 2.0f, Target);
	}

	FinishCast();
}

// ---------------------------------------------------------------------------------------------------------------------
// 세이지

UMobaAbility_HealingWave::UMobaAbility_HealingWave()
{
	DisplayName = NSLOCTEXT("Moba", "HealingWaveName", "치유의 물결");
	Description = NSLOCTEXT("Moba", "HealingWaveDesc", "지정한 곳에 물결을 일으켜 범위 안의 적에게 마법 피해를 주고, 범위 안의 아군 챔피언(자신 포함)의 체력을 회복합니다.");
	CastRange = 800.0f;
	ManaCost = 70.0f;
	CooldownDuration = 9.0f;
	bUltimate = false;

	Radius = 280.0f;
	Delay = 0.0f;
	BaseDamage = 60.0f;
	DamagePerLevel = 35.0f;
	AbilityPowerRatio = 0.4f;
	StunDuration = 0.0f;
}

void UMobaAbility_HealingWave::OnDetonated(AMobaCharacter* Caster, const FVector& Location)
{
	const float Heal = ScaleByLevel(BaseHeal, HealPerLevel) + GetAbilityPower(GetAbilitySystemComponentFromActorInfo()) * HealAbilityPowerRatio;

	TArray<AMobaCharacter*> Allies;
	MobaAbility::FindAlliedChampionsInRadius(Caster, Location, Radius, Allies);
	for (AMobaCharacter* Ally : Allies)
	{
		MobaAbility::HealTarget(Ally, Heal);
	}
}

UMobaAbility_GuardianLight::UMobaAbility_GuardianLight()
{
	DisplayName = NSLOCTEXT("Moba", "GuardianLightName", "수호의 빛");
	Description = NSLOCTEXT("Moba", "GuardianLightDesc", "커서 근처의 아군 챔피언과 자신에게 2.5초 동안 피해를 흡수하는 보호막을 줍니다. 커서 근처에 아군이 없으면 자신에게만 줍니다.");
	TargetingMode = EMobaTargetingMode::Location;
	CastRange = 700.0f;
	ManaCost = 60.0f;
	CooldownDuration = 12.0f;
}

void UMobaAbility_GuardianLight::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	const float Shield = ScaleByLevel(BaseShield, ShieldPerLevel) + GetAbilityPower(GetAbilitySystemComponentFromActorInfo()) * AbilityPowerRatio;
	Caster->AddShield(Shield, ShieldDuration);

	// 커서에 가장 가까운 다른 아군 챔피언 하나(사거리 안).
	TArray<AMobaCharacter*> Allies;
	MobaAbility::FindAlliedChampionsInRadius(Caster, TargetLocation, AllyPickRadius, Allies);
	AMobaCharacter* Best = nullptr;
	for (AMobaCharacter* Ally : Allies)
	{
		if (Ally != Caster && FVector::Dist2D(Ally->GetActorLocation(), Caster->GetActorLocation()) <= CastRange + AllyPickRadius
			&& (!Best || FVector::DistSquared2D(Ally->GetActorLocation(), TargetLocation) < FVector::DistSquared2D(Best->GetActorLocation(), TargetLocation)))
		{
			Best = Ally;
		}
	}
	if (Best)
	{
		Best->AddShield(Shield, ShieldDuration);
	}

	FinishCast();
}

UMobaAbility_BlessedWind::UMobaAbility_BlessedWind()
{
	DisplayName = NSLOCTEXT("Moba", "BlessedWindName", "축복의 바람");
	Description = NSLOCTEXT("Moba", "BlessedWindDesc", "자신과 주변 아군 챔피언의 이동 속도가 2초 동안 30% 오릅니다.");
	TargetingMode = EMobaTargetingMode::None;
	ManaCost = 50.0f;
	CooldownDuration = 14.0f;
}

void UMobaAbility_BlessedWind::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	TArray<AMobaCharacter*> Allies;
	MobaAbility::FindAlliedChampionsInRadius(Caster, Caster->GetActorLocation(), Radius, Allies);
	for (AMobaCharacter* Ally : Allies)
	{
		MobaAbility::ApplySpecToTarget(MobaAbility::MakeStatModSpec(AbilitySystem, Caster, Duration, MoveSpeedMultiplier, 1.0f, MobaTags::State_Hasted), Ally);
	}
	FinishCast();
}

UMobaAbility_Sanctuary::UMobaAbility_Sanctuary()
{
	DisplayName = NSLOCTEXT("Moba", "SanctuaryName", "성역");
	Description = NSLOCTEXT("Moba", "SanctuaryDesc", "넓은 범위의 아군 챔피언(자신 포함)의 체력을 크게 회복하고 기절·속박·침묵·둔화를 풀어 줍니다.");
	TargetingMode = EMobaTargetingMode::None;
	ManaCost = 120.0f;
	CooldownDuration = 90.0f;
	bUltimate = true;
}

void UMobaAbility_Sanctuary::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	const float Heal = ScaleByLevel(BaseHeal, HealPerLevel) + GetAbilityPower(GetAbilitySystemComponentFromActorInfo()) * AbilityPowerRatio;

	FGameplayTagContainer CrowdControl;
	CrowdControl.AddTag(MobaTags::State_Stunned);
	CrowdControl.AddTag(MobaTags::State_Rooted);
	CrowdControl.AddTag(MobaTags::State_Silenced);
	CrowdControl.AddTag(MobaTags::State_Slowed);
	CrowdControl.AddTag(MobaTags::State_Airborne);

	TArray<AMobaCharacter*> Allies;
	MobaAbility::FindAlliedChampionsInRadius(Caster, Caster->GetActorLocation(), Radius, Allies);
	for (AMobaCharacter* Ally : Allies)
	{
		if (UAbilitySystemComponent* AllyAbilitySystem = Ally->GetAbilitySystemComponent())
		{
			AllyAbilitySystem->RemoveActiveEffectsWithGrantedTags(CrowdControl);
		}
		MobaAbility::HealTarget(Ally, Heal);
	}
	FinishCast();
}
