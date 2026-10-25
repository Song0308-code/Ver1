#include "AbilitySystem/Abilities/MobaAbilityArchetypes.h"
#include "AbilitySystem/MobaAbilityLibrary.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaCharacter.h"
#include "Combat/MobaAreaIndicator.h"
#include "Combat/MobaCombatLibrary.h"
#include "Combat/MobaProjectile.h"
#include "Combat/MobaSkillshotProjectile.h"
#include "Combat/MobaTrap.h"
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
			if (SilenceDuration > 0.0f)
			{
				MobaAbility::ApplySpecToTarget(MobaAbility::MakeCrowdControlSpec(AbilitySystem, Caster, SilenceDuration, MobaTags::State_Silenced), Enemy);
			}
		}

		OnDetonated(Caster, Location);
	}

	FinishCast();
}

// ---------------------------------------------------------------------------------------------------------------------
// 깡통

UMobaAbility_TinHook::UMobaAbility_TinHook()
{
	DisplayName = NSLOCTEXT("Moba", "TinHookName", "갈고리");
	Description = NSLOCTEXT("Moba", "TinHookDesc", "커서 방향으로 갈고리를 던집니다. 처음 맞은 적에게 마법 피해를 주고 자기 앞으로 끌어옵니다. 끌려오는 동안 대상은 아무것도 할 수 없습니다.");
	TargetingMode = EMobaTargetingMode::Direction;
	CastRange = 1050.0f;
	ManaCost = 100.0f;
	CooldownDuration = 18.0f;
}

void UMobaAbility_TinHook::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
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

	if (AMobaSkillshotProjectile* Hook = GetWorld()->SpawnActor<AMobaSkillshotProjectile>(AMobaSkillshotProjectile::StaticClass(), Caster->GetActorLocation(), Direction.Rotation(), SpawnParams))
	{
		const float Damage = ScaleByLevel(BaseDamage, DamagePerLevel) + GetAbilityPower(AbilitySystem) * AbilityPowerRatio;
		TArray<FGameplayEffectSpecHandle> Effects;
		Effects.Add(MobaAbility::MakeDamageSpec(AbilitySystem, Caster, 0.0f, Damage, 0.0f));
		Effects.Add(MobaAbility::MakeCrowdControlSpec(AbilitySystem, Caster, PullDuration, MobaTags::State_Airborne));
		Hook->Launch(Caster, Direction, HookSpeed, CastRange, HookRadius, Effects);

		// 맞은 대상을 그때의 시전자 앞으로 끌어온다.
		const TWeakObjectPtr<AMobaCharacter> WeakCaster = Caster;
		const float StopDistance = PullStopDistance;
		const float Duration = PullDuration;
		Hook->OnHitActor = [WeakCaster, StopDistance, Duration](AActor* HitActor)
		{
			const AMobaCharacter* Puller = WeakCaster.Get();
			if (!Puller || MobaCombat::IsDead(HitActor))
			{
				return;
			}

			const FVector Toward = (HitActor->GetActorLocation() - Puller->GetActorLocation()).GetSafeNormal2D();
			MobaCombat::ForceMove(HitActor, Puller->GetActorLocation() + Toward * StopDistance, Duration);
		};
	}

	FinishCast();
}

UMobaAbility_TinOverdrive::UMobaAbility_TinOverdrive()
{
	DisplayName = NSLOCTEXT("Moba", "TinOverdriveName", "과부하");
	Description = NSLOCTEXT("Moba", "TinOverdriveDesc", "3초 동안 이동 속도가 60%, 공격 속도가 30% 오릅니다. 끝나면 과열되어 1.5초 동안 25% 느려집니다.");
	TargetingMode = EMobaTargetingMode::None;
	ManaCost = 60.0f;
	CooldownDuration = 15.0f;
}

void UMobaAbility_TinOverdrive::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	MobaAbility::ApplySpecToTarget(MobaAbility::MakeStatModSpec(AbilitySystem, Caster, HasteDuration, HasteMultiplier, AttackSpeedMultiplier, MobaTags::State_Hasted), Caster);

	// 스킬이 끝난 뒤에도 돌아가도록 시전자에 묶은 타이머로 과열을 건다.
	const TWeakObjectPtr<UAbilitySystemComponent> WeakAbilitySystem = AbilitySystem;
	const float Duration = OverheatDuration;
	const float Multiplier = OverheatMultiplier;
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(Caster, [Caster, WeakAbilitySystem, Duration, Multiplier]()
	{
		if (UAbilitySystemComponent* Source = WeakAbilitySystem.Get())
		{
			if (!Caster->IsDead())
			{
				MobaAbility::ApplySpecToTarget(MobaAbility::MakeStatModSpec(Source, Caster, Duration, Multiplier, 1.0f, MobaTags::State_Slowed), Caster);
			}
		}
	}), HasteDuration, false);

	FinishCast();
}

UMobaAbility_TinPowerPunch::UMobaAbility_TinPowerPunch()
{
	DisplayName = NSLOCTEXT("Moba", "TinPowerPunchName", "감전 펀치");
	Description = NSLOCTEXT("Moba", "TinPowerPunchDesc", "5초 안의 다음 기본 공격이 공격력의 100%만큼 추가 물리 피해를 주고 대상을 1초 동안 띄웁니다.");
	TargetingMode = EMobaTargetingMode::None;
	ManaCost = 30.0f;
	CooldownDuration = 9.0f;
}

void UMobaAbility_TinPowerPunch::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	FMobaAttackBonus Bonus;
	Bonus.BonusPhysicalDamage = GetAttackDamage(GetAbilitySystemComponentFromActorInfo()) * AttackDamageRatio;
	Bonus.AirborneDuration = AirborneDuration;
	Caster->SetEmpoweredAttack(Bonus, Duration);
	FinishCast();
}

UMobaAbility_TinDischarge::UMobaAbility_TinDischarge()
{
	DisplayName = NSLOCTEXT("Moba", "TinDischargeName", "방전");
	Description = NSLOCTEXT("Moba", "TinDischargeDesc", "주변 적에게 마법 피해를 주고 0.5초 동안 침묵시킵니다.");
	TargetingMode = EMobaTargetingMode::None;
	CastRange = 0.0f;
	ManaCost = 100.0f;
	CooldownDuration = 80.0f;
	bUltimate = true;

	Radius = 600.0f;
	Delay = 0.0f;
	BaseDamage = 250.0f;
	DamagePerLevel = 125.0f;
	AbilityPowerRatio = 1.0f;
	StunDuration = 0.0f;
	SilenceDuration = 0.5f;
}

// ---------------------------------------------------------------------------------------------------------------------
// 젖소

UMobaAbility_CowSlam::UMobaAbility_CowSlam()
{
	DisplayName = NSLOCTEXT("Moba", "CowSlamName", "지면 충격");
	Description = NSLOCTEXT("Moba", "CowSlamDesc", "주변 땅을 내리쳐 적에게 마법 피해를 주고 0.75초 동안 띄웁니다.");
	TargetingMode = EMobaTargetingMode::None;
	CastRange = 0.0f;
	ManaCost = 70.0f;
	CooldownDuration = 14.0f;
	bUltimate = false;

	Radius = 350.0f;
	Delay = 0.0f;
	BaseDamage = 70.0f;
	DamagePerLevel = 40.0f;
	AbilityPowerRatio = 0.6f;
	StunDuration = 0.0f;
	AirborneDuration = 0.75f;
}

void UMobaAbility_CowSlam::OnDetonated(AMobaCharacter* Caster, const FVector& Location)
{
	TArray<AActor*> Enemies;
	MobaAbility::FindEnemiesInRadius(Caster, Location, Radius, Enemies);
	for (AActor* Enemy : Enemies)
	{
		Caster->NotifyCrowdControlApplied(Enemy);
	}
}

UMobaAbility_CowCharge::UMobaAbility_CowCharge()
{
	DisplayName = NSLOCTEXT("Moba", "CowChargeName", "돌진 밀치기");
	Description = NSLOCTEXT("Moba", "CowChargeDesc", "커서 근처의 적에게 달려가 들이받아 마법 피해를 주고 반대쪽으로 450만큼 밀어냅니다. 밀려나는 동안 대상은 아무것도 할 수 없습니다.");
	TargetingMode = EMobaTargetingMode::Location;
	CastRange = 650.0f;
	ManaCost = 60.0f;
	CooldownDuration = 12.0f;
	bMovementAbility = true;
}

bool UMobaAbility_CowCharge::ResolveTarget(const AMobaCharacter* Caster, FVector& InOutTargetLocation) const
{
	PendingTarget = MobaAbility::PickEnemyNearCursor(Caster, InOutTargetLocation, TargetPickRadius, CastRange, [](const AActor*) { return true; });
	if (!PendingTarget.IsValid())
	{
		return false;
	}
	InOutTargetLocation = PendingTarget->GetActorLocation();
	return true;
}

void UMobaAbility_CowCharge::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	AActor* Target = PendingTarget.Get();
	PendingTarget.Reset();
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	if (!Target || !AbilitySystem)
	{
		FinishCast();
		return;
	}

	// 대상 바로 앞까지 달려간 뒤 들이받는다.
	const FVector Direction = (Target->GetActorLocation() - Caster->GetActorLocation()).GetSafeNormal2D();
	MobaCombat::ForceMove(Caster, Target->GetActorLocation() - Direction * 110.0f, DashDuration);

	const float Damage = ScaleByLevel(BaseDamage, DamagePerLevel) + GetAbilityPower(AbilitySystem) * AbilityPowerRatio;
	const TWeakObjectPtr<AActor> WeakTarget = Target;
	const TWeakObjectPtr<UAbilitySystemComponent> WeakAbilitySystem = AbilitySystem;
	const float Distance = KnockbackDistance;
	const float PushDuration = KnockbackDuration;
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(Caster, [Caster, WeakTarget, WeakAbilitySystem, Damage, Distance, PushDuration]()
	{
		AActor* Victim = WeakTarget.Get();
		UAbilitySystemComponent* Source = WeakAbilitySystem.Get();
		if (!Victim || !Source || Caster->IsDead() || MobaCombat::IsDead(Victim))
		{
			return;
		}

		MobaAbility::ApplySpecToTarget(MobaAbility::MakeDamageSpec(Source, Caster, 0.0f, Damage, 0.0f), Victim);
		if (MobaCombat::IsDead(Victim))
		{
			return;
		}

		const FVector PushDirection = (Victim->GetActorLocation() - Caster->GetActorLocation()).GetSafeNormal2D();
		MobaAbility::ApplySpecToTarget(MobaAbility::MakeCrowdControlSpec(Source, Caster, PushDuration, MobaTags::State_Airborne), Victim);
		MobaCombat::ForceMove(Victim, Victim->GetActorLocation() + PushDirection * Distance, PushDuration, true);
		Caster->NotifyCrowdControlApplied(Victim);
	}), DashDuration, false);

	FinishCast();
}

UMobaAbility_CowBellow::UMobaAbility_CowBellow()
{
	DisplayName = NSLOCTEXT("Moba", "CowBellowName", "함성");
	Description = NSLOCTEXT("Moba", "CowBellowDesc", "주변 아군 챔피언(자신 포함)에게 2.5초 동안 피해를 흡수하는 보호막을 줍니다.");
	TargetingMode = EMobaTargetingMode::None;
	ManaCost = 60.0f;
	CooldownDuration = 14.0f;
}

void UMobaAbility_CowBellow::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	const float Shield = ScaleByLevel(BaseShield, ShieldPerLevel) + GetAbilityPower(GetAbilitySystemComponentFromActorInfo()) * AbilityPowerRatio;
	TArray<AMobaCharacter*> Allies;
	MobaAbility::FindAlliedChampionsInRadius(Caster, Caster->GetActorLocation(), Radius, Allies);
	for (AMobaCharacter* Ally : Allies)
	{
		Ally->AddShield(Shield, ShieldDuration);
	}
	FinishCast();
}

UMobaAbility_CowUnbreakable::UMobaAbility_CowUnbreakable()
{
	DisplayName = NSLOCTEXT("Moba", "CowUnbreakableName", "불굴");
	Description = NSLOCTEXT("Moba", "CowUnbreakableDesc", "걸린 기절·에어본·속박·침묵·둔화를 풀고 6초 동안 받는 피해가 50% 줄어듭니다.");
	TargetingMode = EMobaTargetingMode::None;
	ManaCost = 100.0f;
	CooldownDuration = 100.0f;
	bUltimate = true;
	bUsableWhileCrowdControlled = true;
	ActivationBlockedTags.RemoveTag(MobaTags::State_Stunned);
}

void UMobaAbility_CowUnbreakable::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();

	FGameplayTagContainer CrowdControl;
	CrowdControl.AddTag(MobaTags::State_Stunned);
	CrowdControl.AddTag(MobaTags::State_Rooted);
	CrowdControl.AddTag(MobaTags::State_Silenced);
	CrowdControl.AddTag(MobaTags::State_Slowed);
	CrowdControl.AddTag(MobaTags::State_Airborne);
	AbilitySystem->RemoveActiveEffectsWithGrantedTags(CrowdControl);

	MobaAbility::ApplySpecToTarget(MobaAbility::MakeStatBuffSpec(AbilitySystem, Caster, Duration, FMobaStatBonus(), FGameplayTagContainer(MobaTags::Buff_Unbreakable)), Caster);
	FinishCast();
}

// ---------------------------------------------------------------------------------------------------------------------
// 노머고

namespace
{
	AMobaSkillshotProjectile* SpawnSkillshot(AMobaCharacter* Caster, const FVector& TargetLocation, FVector& OutDirection)
	{
		OutDirection = TargetLocation - Caster->GetActorLocation();
		OutDirection.Z = 0.0;
		if (!OutDirection.Normalize())
		{
			OutDirection = Caster->GetActorForwardVector();
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Caster;
		SpawnParams.Instigator = Caster;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return Caster->GetWorld()->SpawnActor<AMobaSkillshotProjectile>(AMobaSkillshotProjectile::StaticClass(), Caster->GetActorLocation(), OutDirection.Rotation(), SpawnParams);
	}
}

UMobaAbility_NomagoPulse::UMobaAbility_NomagoPulse()
{
	DisplayName = NSLOCTEXT("Moba", "NomagoPulseName", "파동탄");
	Description = NSLOCTEXT("Moba", "NomagoPulseDesc", "커서 방향으로 파동탄을 쏩니다. 처음 맞은 적에게 물리 피해를 주고, 맞으면 W·E·R의 재사용 대기가 1.5초 줄어듭니다.");
	TargetingMode = EMobaTargetingMode::Direction;
	CastRange = 1150.0f;
	ManaCost = 30.0f;
	CooldownDuration = 5.0f;
}

void UMobaAbility_NomagoPulse::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	FVector Direction;
	if (AMobaSkillshotProjectile* Projectile = SpawnSkillshot(Caster, TargetLocation, Direction))
	{
		const float Damage = ScaleByLevel(BaseDamage, DamagePerLevel) + GetAttackDamage(AbilitySystem) * AttackDamageRatio + GetAbilityPower(AbilitySystem) * AbilityPowerRatio;
		Projectile->Launch(Caster, Direction, ProjectileSpeed, CastRange, ProjectileRadius, { MobaAbility::MakeDamageSpec(AbilitySystem, Caster, Damage, 0.0f, 0.0f) });

		const TWeakObjectPtr<AMobaCharacter> WeakCaster = Caster;
		const float Refund = CooldownRefund;
		Projectile->OnHitActor = [WeakCaster, Refund](AActor* HitActor)
		{
			if (AMobaCharacter* Owner = WeakCaster.Get())
			{
				for (const EMobaAbilitySlot Slot : { EMobaAbilitySlot::W, EMobaAbilitySlot::E, EMobaAbilitySlot::R })
				{
					MobaAbility::ReduceCooldown(Owner->GetAbilitySystemComponent(), Slot, Refund);
				}
				Owner->NotifyAbilityHit(HitActor);
			}
		};
	}

	FinishCast();
}

UMobaAbility_NomagoMark::UMobaAbility_NomagoMark()
{
	DisplayName = NSLOCTEXT("Moba", "NomagoMarkName", "표식탄");
	Description = NSLOCTEXT("Moba", "NomagoMarkDesc", "적을 꿰뚫고 날아가며 맞은 적 챔피언에게 4초 동안 표식을 남깁니다. 표식은 노머고의 기본 공격이나 다른 스킬에 맞으면 터집니다(피해는 패시브 설명).");
	TargetingMode = EMobaTargetingMode::Direction;
	CastRange = 1000.0f;
	ManaCost = 50.0f;
	CooldownDuration = 10.0f;
}

void UMobaAbility_NomagoMark::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	FVector Direction;
	if (AMobaSkillshotProjectile* Projectile = SpawnSkillshot(Caster, TargetLocation, Direction))
	{
		Projectile->bPierce = true;
		Projectile->Launch(Caster, Direction, ProjectileSpeed, CastRange, ProjectileRadius, {});

		const TWeakObjectPtr<AMobaCharacter> WeakCaster = Caster;
		const TWeakObjectPtr<UAbilitySystemComponent> WeakAbilitySystem = AbilitySystem;
		const float Duration = MarkDuration;
		Projectile->OnHitActor = [WeakCaster, WeakAbilitySystem, Duration](AActor* HitActor)
		{
			AMobaCharacter* Owner = WeakCaster.Get();
			UAbilitySystemComponent* Source = WeakAbilitySystem.Get();
			if (!Owner || !Source || !HitActor->IsA<AMobaCharacter>())
			{
				return;
			}

			// 이미 있던 표식이 이 스킬로 터지지 않도록, 알린 다음에 표식을 붙인다.
			Owner->NotifyAbilityHit(HitActor);
			MobaAbility::ApplySpecToTarget(MobaAbility::MakeCrowdControlSpec(Source, Owner, Duration, MobaTags::State_Marked), HitActor);
		};
	}

	FinishCast();
}

UMobaAbility_NomagoPhase::UMobaAbility_NomagoPhase()
{
	DisplayName = NSLOCTEXT("Moba", "NomagoPhaseName", "위상 도약");
	Description = NSLOCTEXT("Moba", "NomagoPhaseDesc", "커서 쪽으로 짧게 순간이동한 뒤, 근처의 적(챔피언 우선)에게 유도탄을 쏴 마법 피해를 줍니다.");
	TargetingMode = EMobaTargetingMode::Location;
	CastRange = 475.0f;
	ManaCost = 80.0f;
	CooldownDuration = 18.0f;
	bMovementAbility = true;
}

void UMobaAbility_NomagoPhase::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	FVector Facing = TargetLocation - Caster->GetActorLocation();
	Facing.Z = 0.0;
	Caster->TeleportChampion(TargetLocation, Facing.IsNearlyZero() ? Caster->GetActorRotation() : Facing.Rotation());

	AActor* Target = MobaAbility::PickEnemyNearCursor(Caster, Caster->GetActorLocation(), BoltSearchRadius, BoltSearchRadius, [](const AActor* Candidate) { return Candidate->IsA<AMobaCharacter>(); });
	if (!Target)
	{
		Target = MobaAbility::PickEnemyNearCursor(Caster, Caster->GetActorLocation(), BoltSearchRadius, BoltSearchRadius, [](const AActor*) { return true; });
	}

	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	if (Target && AbilitySystem)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Caster;
		SpawnParams.Instigator = Caster;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AMobaProjectile* Bolt = GetWorld()->SpawnActor<AMobaProjectile>(AMobaProjectile::StaticClass(), Caster->GetActorLocation(), Caster->GetActorRotation(), SpawnParams))
		{
			const float Damage = ScaleByLevel(BaseDamage, DamagePerLevel) + GetAttackDamage(AbilitySystem) * AttackDamageRatio + GetAbilityPower(AbilitySystem) * AbilityPowerRatio;
			Bolt->InitHoming(Target, MobaAbility::MakeDamageSpec(AbilitySystem, Caster, 0.0f, Damage, 0.0f), BoltSpeed, 0.6f, Caster->GetTeam());
		}

		// 유도탄은 반드시 맞으므로 쏠 때 적중으로 친다.
		Caster->NotifyAbilityHit(Target);
	}

	FinishCast();
}

UMobaAbility_NomagoBarrage::UMobaAbility_NomagoBarrage()
{
	DisplayName = NSLOCTEXT("Moba", "NomagoBarrageName", "궤도 포격");
	Description = NSLOCTEXT("Moba", "NomagoBarrageDesc", "1초 동안 충전한 뒤 커서 방향으로 맵을 가로지르는 넓은 포격을 쏩니다. 지나가는 모든 적에게 마법 피해를 주며, 챔피언이 아닌 대상은 절반만 받습니다.");
	TargetingMode = EMobaTargetingMode::Direction;
	CastRange = 16000.0f;
	ManaCost = 100.0f;
	CooldownDuration = 100.0f;
	bUltimate = true;
}

void UMobaAbility_NomagoBarrage::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	const float Damage = ScaleByLevel(BaseDamage, DamagePerLevel) + GetAttackDamage(AbilitySystem) * AttackDamageRatio + GetAbilityPower(AbilitySystem) * AbilityPowerRatio;
	const TWeakObjectPtr<UAbilitySystemComponent> WeakAbilitySystem = AbilitySystem;
	const float Speed = BeamSpeed;
	const float Range = CastRange;
	const float Radius = BeamRadius;
	const float Scale = NonChampionDamageScale;

	// 충전이 끝나면 그때의 시전자 위치에서 쏜다.
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(Caster, [Caster, TargetLocation, WeakAbilitySystem, Damage, Speed, Range, Radius, Scale]()
	{
		if (Caster->IsDead() || !WeakAbilitySystem.IsValid())
		{
			return;
		}

		FVector Direction;
		AMobaSkillshotProjectile* Beam = SpawnSkillshot(Caster, TargetLocation, Direction);
		if (!Beam)
		{
			return;
		}

		Beam->bPierce = true;
		Beam->Launch(Caster, Direction, Speed, Range, Radius, {});
		Beam->OnHitActor = [Caster, WeakAbilitySystem, Damage, Scale](AActor* HitActor)
		{
			if (UAbilitySystemComponent* Source = WeakAbilitySystem.Get())
			{
				const bool bChampion = HitActor->IsA<AMobaCharacter>();
				MobaAbility::ApplySpecToTarget(MobaAbility::MakeDamageSpec(Source, Caster, 0.0f, bChampion ? Damage : Damage * Scale, 0.0f), HitActor);
				if (bChampion && IsValid(Caster))
				{
					Caster->NotifyAbilityHit(HitActor);
				}
			}
		};
	}), ChargeTime, false);

	FinishCast();
}

// ---------------------------------------------------------------------------------------------------------------------
// 진진자라

namespace
{
	/** 서버 전용. 시전자가 Seconds 동안 스스로 걷지 못하게 한다(조준·제자리 사격). */
	void HoldInPlace(AMobaCharacter* Caster, float Seconds)
	{
		UAbilitySystemComponent* AbilitySystem = Caster->GetAbilitySystemComponent();
		if (!AbilitySystem)
		{
			return;
		}
		AbilitySystem->AddLooseGameplayTag(MobaTags::State_Displaced, 1, EGameplayTagReplicationState::TagOnly);
		const TWeakObjectPtr<UAbilitySystemComponent> WeakAbilitySystem = AbilitySystem;
		FTimerHandle Handle;
		Caster->GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([WeakAbilitySystem]()
		{
			if (UAbilitySystemComponent* Source = WeakAbilitySystem.Get())
			{
				Source->RemoveLooseGameplayTag(MobaTags::State_Displaced, 1, EGameplayTagReplicationState::TagOnly);
			}
		}), Seconds, false);
	}

	/** 튕기는 탄 하나를 Target에게 쏜다. 닿으면 HitSoFar에 넣고 다음 대상을 찾는다. */
	void FireRicochet(AMobaCharacter* Caster, const TWeakObjectPtr<UAbilitySystemComponent>& WeakAbilitySystem, const FVector& From, AActor* Target,
		TSharedRef<TArray<TWeakObjectPtr<AActor>>> HitSoFar, int32 MaxTargets, float BounceRadius, float Speed, float Damage)
	{
		UAbilitySystemComponent* AbilitySystem = WeakAbilitySystem.Get();
		if (!AbilitySystem || !Target)
		{
			return;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Caster;
		SpawnParams.Instigator = Caster;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AMobaProjectile* Bolt = Caster->GetWorld()->SpawnActor<AMobaProjectile>(AMobaProjectile::StaticClass(), From, FRotator::ZeroRotator, SpawnParams);
		if (!Bolt)
		{
			return;
		}

		HitSoFar->Add(Target);
		Bolt->InitHoming(Target, MobaAbility::MakeDamageSpec(AbilitySystem, Caster, Damage, 0.0f, 0.0f), Speed, 0.5f, Caster->GetTeam());
		Bolt->OnArrived = [Caster, WeakAbilitySystem, HitSoFar, MaxTargets, BounceRadius, Speed, Damage](AActor* Arrived)
		{
			if (!IsValid(Caster))
			{
				return;
			}
			Caster->NotifyAbilityHit(Arrived);
			if (HitSoFar->Num() >= MaxTargets)
			{
				return;
			}

			// 아직 맞지 않은 가장 가까운 적에게 튕긴다.
			TArray<AActor*> Enemies;
			MobaAbility::FindEnemiesInRadius(Caster, Arrived->GetActorLocation(), BounceRadius, Enemies);
			AActor* Next = nullptr;
			for (AActor* Enemy : Enemies)
			{
				if (!HitSoFar->Contains(Enemy) && (!Next || FVector::DistSquared2D(Enemy->GetActorLocation(), Arrived->GetActorLocation()) < FVector::DistSquared2D(Next->GetActorLocation(), Arrived->GetActorLocation())))
				{
					Next = Enemy;
				}
			}
			FireRicochet(Caster, WeakAbilitySystem, Arrived->GetActorLocation(), Next, HitSoFar, MaxTargets, BounceRadius, Speed, Damage);
		};
	}
}

UMobaAbility_JinRicochet::UMobaAbility_JinRicochet()
{
	DisplayName = NSLOCTEXT("Moba", "JinRicochetName", "도탄");
	Description = NSLOCTEXT("Moba", "JinRicochetDesc", "커서 근처의 적에게 탄을 쏩니다. 맞을 때마다 근처의 다른 적에게 튕겨 최대 4명에게 물리 피해를 줍니다.");
	TargetingMode = EMobaTargetingMode::Location;
	CastRange = 550.0f;
	ManaCost = 40.0f;
	CooldownDuration = 7.0f;
}

bool UMobaAbility_JinRicochet::ResolveTarget(const AMobaCharacter* Caster, FVector& InOutTargetLocation) const
{
	PendingTarget = MobaAbility::PickEnemyNearCursor(Caster, InOutTargetLocation, TargetPickRadius, CastRange, [](const AActor*) { return true; });
	if (!PendingTarget.IsValid())
	{
		return false;
	}
	InOutTargetLocation = PendingTarget->GetActorLocation();
	return true;
}

void UMobaAbility_JinRicochet::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	AActor* Target = PendingTarget.Get();
	PendingTarget.Reset();
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	if (Target && AbilitySystem)
	{
		const float Damage = ScaleByLevel(BaseDamage, DamagePerLevel) + GetAttackDamage(AbilitySystem) * AttackDamageRatio;
		FireRicochet(Caster, AbilitySystem, Caster->GetActorLocation(), Target, MakeShared<TArray<TWeakObjectPtr<AActor>>>(), MaxTargets, BounceRadius, BoltSpeed, Damage);
	}
	FinishCast();
}

UMobaAbility_JinAimedShot::UMobaAbility_JinAimedShot()
{
	DisplayName = NSLOCTEXT("Moba", "JinAimedShotName", "조준 사격");
	Description = NSLOCTEXT("Moba", "JinAimedShotDesc", "0.6초 동안 제자리에서 조준한 뒤 아주 긴 사거리로 저격합니다. 미니언은 꿰뚫어 75% 피해를 주고 처음 맞은 챔피언에서 멈춥니다. 표식이 있는 챔피언은 1.5초 동안 속박됩니다.");
	TargetingMode = EMobaTargetingMode::Direction;
	CastRange = 2500.0f;
	ManaCost = 60.0f;
	CooldownDuration = 12.0f;
}

void UMobaAbility_JinAimedShot::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	HoldInPlace(Caster, AimTime);

	const float Damage = ScaleByLevel(BaseDamage, DamagePerLevel) + GetAttackDamage(AbilitySystem) * AttackDamageRatio;
	const TWeakObjectPtr<UAbilitySystemComponent> WeakAbilitySystem = AbilitySystem;
	const float Speed = ShotSpeed;
	const float Range = CastRange;
	const float Radius = ShotRadius;
	const float Root = RootDuration;
	const float Scale = NonChampionDamageScale;
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(Caster, [Caster, TargetLocation, WeakAbilitySystem, Damage, Speed, Range, Radius, Root, Scale]()
	{
		if (Caster->IsDead() || !WeakAbilitySystem.IsValid())
		{
			return;
		}

		FVector Direction;
		AMobaSkillshotProjectile* Shot = SpawnSkillshot(Caster, TargetLocation, Direction);
		if (!Shot)
		{
			return;
		}
		Shot->bPierce = true;
		Shot->Launch(Caster, Direction, Speed, Range, Radius, {});
		const TWeakObjectPtr<AMobaSkillshotProjectile> WeakShot = Shot;
		Shot->OnHitActor = [Caster, WeakAbilitySystem, WeakShot, Damage, Root, Scale](AActor* HitActor)
		{
			UAbilitySystemComponent* Source = WeakAbilitySystem.Get();
			if (!Source || !IsValid(Caster))
			{
				return;
			}

			const bool bChampion = HitActor->IsA<AMobaCharacter>();
			MobaAbility::ApplySpecToTarget(MobaAbility::MakeDamageSpec(Source, Caster, bChampion ? Damage : Damage * Scale, 0.0f, 0.0f), HitActor);
			if (!bChampion)
			{
				return;
			}

			if (!MobaCombat::IsDead(HitActor) && MobaAbility::ConsumeMark(HitActor))
			{
				MobaAbility::ApplySpecToTarget(MobaAbility::MakeCrowdControlSpec(Source, Caster, Root, MobaTags::State_Rooted), HitActor);
			}
			Caster->NotifyAbilityHit(HitActor);
			if (AMobaSkillshotProjectile* Projectile = WeakShot.Get())
			{
				Projectile->StopPierce();
			}
		};
	}), AimTime, false);

	FinishCast();
}

UMobaAbility_JinTrap::UMobaAbility_JinTrap()
{
	DisplayName = NSLOCTEXT("Moba", "JinTrapName", "덫");
	Description = NSLOCTEXT("Moba", "JinTrapDesc", "지정한 곳에 상대에게 보이지 않는 덫을 놓습니다(최대 2개, 60초). 1초 뒤 작동하고, 적 챔피언이 밟으면 터져 주변 적에게 마법 피해와 2초 35% 둔화, 챔피언에게는 표식을 남깁니다.");
	TargetingMode = EMobaTargetingMode::Location;
	CastRange = 750.0f;
	ManaCost = 30.0f;
	CooldownDuration = 2.0f;
}

void UMobaAbility_JinTrap::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	FVector Location = TargetLocation;
	Location.Z = Caster->GetActorLocation().Z - Caster->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.0f;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Caster;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AMobaTrap* Trap = GetWorld()->SpawnActor<AMobaTrap>(AMobaTrap::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams))
	{
		const float Damage = ScaleByLevel(BaseDamage, DamagePerLevel) + GetAttackDamage(AbilitySystem) * AttackDamageRatio + GetAbilityPower(AbilitySystem) * AbilityPowerRatio;
		Trap->Arm(Caster, Damage, ArmDelay, Lifetime);

		// 많으면 가장 오래된 덫부터 없앤다.
		ActiveTraps.RemoveAll([](const TWeakObjectPtr<AActor>& Existing) { return !Existing.IsValid(); });
		ActiveTraps.Add(Trap);
		while (ActiveTraps.Num() > MaxTraps)
		{
			ActiveTraps[0]->Destroy();
			ActiveTraps.RemoveAt(0);
		}
	}
	FinishCast();
}

UMobaAbility_JinCurtainCall::UMobaAbility_JinCurtainCall()
{
	DisplayName = NSLOCTEXT("Moba", "JinCurtainCallName", "정밀 사격");
	Description = NSLOCTEXT("Moba", "JinCurtainCallDesc", "제자리에 서서 조준한 방향으로 강한 탄 세 발을 쏩니다. 각 탄은 처음 맞은 적에게 물리 피해를 주고, 대상이 잃은 체력 1%당 피해가 1% 늘어납니다. 세 번째 발은 50% 더 강합니다.");
	TargetingMode = EMobaTargetingMode::Direction;
	CastRange = 3000.0f;
	ManaCost = 100.0f;
	CooldownDuration = 110.0f;
	bUltimate = true;
}

void UMobaAbility_JinCurtainCall::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	HoldInPlace(Caster, FirstShotDelay + ShotInterval * (ShotCount - 1) + 0.1f);

	const float Damage = ScaleByLevel(BaseDamage, DamagePerLevel) + GetAttackDamage(AbilitySystem) * AttackDamageRatio;
	const TWeakObjectPtr<UAbilitySystemComponent> WeakAbilitySystem = AbilitySystem;
	for (int32 Index = 0; Index < ShotCount; ++Index)
	{
		const float ShotDamage = Index == ShotCount - 1 ? Damage * LastShotMultiplier : Damage;
		const float Speed = ShotSpeed;
		const float Range = CastRange;
		const float Radius = ShotRadius;
		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(Caster, [Caster, TargetLocation, WeakAbilitySystem, ShotDamage, Speed, Range, Radius]()
		{
			if (Caster->IsDead() || !WeakAbilitySystem.IsValid())
			{
				return;
			}

			FVector Direction;
			AMobaSkillshotProjectile* Shot = SpawnSkillshot(Caster, TargetLocation, Direction);
			if (!Shot)
			{
				return;
			}
			Shot->Launch(Caster, Direction, Speed, Range, Radius, {});
			Shot->OnHitActor = [Caster, WeakAbilitySystem, ShotDamage](AActor* HitActor)
			{
				UAbilitySystemComponent* Source = WeakAbilitySystem.Get();
				const UAbilitySystemComponent* TargetAbilitySystem = MobaCombat::GetAbilitySystem(HitActor);
				if (!Source || !TargetAbilitySystem || !IsValid(Caster))
				{
					return;
				}

				// 잃은 체력 1%당 피해 1% 증가.
				const float MaxHealth = TargetAbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxHealthAttribute());
				const float Health = TargetAbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetHealthAttribute());
				const float Missing = MaxHealth > 0.0f ? FMath::Clamp(1.0f - Health / MaxHealth, 0.0f, 1.0f) : 0.0f;
				MobaAbility::ApplySpecToTarget(MobaAbility::MakeDamageSpec(Source, Caster, ShotDamage * (1.0f + Missing), 0.0f, 0.0f), HitActor);
				Caster->NotifyAbilityHit(HitActor);
			};
		}), FirstShotDelay + ShotInterval * Index, false);
	}

	FinishCast();
}
