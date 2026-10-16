#include "AbilitySystem/Abilities/MobaSummonerSpells.h"
#include "AbilitySystem/MobaAbilityLibrary.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaCharacter.h"
#include "Combat/MobaCombatLibrary.h"
#include "Engine/World.h"
#include "Game/MobaEffects.h"
#include "Game/MobaGameplayTags.h"
#include "Player/MobaPlayerState.h"
#include "TimerManager.h"
#include "Vision/MobaVisionSubsystem.h"

namespace
{
	int32 GetChampionLevel(const AMobaCharacter* Caster)
	{
		const AMobaPlayerState* PlayerState = Caster ? Caster->GetPlayerState<AMobaPlayerState>() : nullptr;
		return PlayerState ? PlayerState->GetChampionLevel() : 1;
	}
}

TSubclassOf<UMobaGameplayAbility> MobaSummoner::GetAbilityClass(EMobaSummonerSpell Spell)
{
	switch (Spell)
	{
	case EMobaSummonerSpell::Flash: return UMobaSummoner_Flash::StaticClass();
	case EMobaSummonerSpell::Heal: return UMobaSummoner_Heal::StaticClass();
	case EMobaSummonerSpell::Ignite: return UMobaSummoner_Ignite::StaticClass();
	case EMobaSummonerSpell::Ghost: return UMobaSummoner_Ghost::StaticClass();
	case EMobaSummonerSpell::Smite: return UMobaSummoner_Smite::StaticClass();
	case EMobaSummonerSpell::Barrier: return UMobaSummoner_Barrier::StaticClass();
	default: return nullptr;
	}
}

FText MobaSummoner::GetDisplayName(EMobaSummonerSpell Spell)
{
	const TSubclassOf<UMobaGameplayAbility> AbilityClass = GetAbilityClass(Spell);
	return AbilityClass ? AbilityClass->GetDefaultObject<UMobaGameplayAbility>()->GetDisplayName() : FText::GetEmpty();
}

// ---------------------------------------------------------------------------------------------------------------------
// 점멸 · 유체화 · 회복

UMobaSummoner_Flash::UMobaSummoner_Flash()
{
	DisplayName = NSLOCTEXT("Moba", "FlashName", "점멸");
	Description = NSLOCTEXT("Moba", "FlashDesc", "커서 쪽으로 즉시 짧게 순간이동합니다. 벽을 넘을 수 있습니다.");
	bSummonerSpell = true;
	CastRange = 400.0f;
	ManaCost = 0.0f;
	CooldownDuration = 300.0f;
}

UMobaSummoner_Ghost::UMobaSummoner_Ghost()
{
	DisplayName = NSLOCTEXT("Moba", "GhostName", "유체화");
	Description = NSLOCTEXT("Moba", "GhostDesc", "10초 동안 이동 속도가 40% 오릅니다.");
	bSummonerSpell = true;
	ManaCost = 0.0f;
	CooldownDuration = 210.0f;

	BuffDuration = 10.0f;
	MoveSpeedMultiplier = 1.4f;
	AttackSpeedMultiplier = 1.0f;
}

UMobaSummoner_Heal::UMobaSummoner_Heal()
{
	DisplayName = NSLOCTEXT("Moba", "HealName", "회복");
	Description = NSLOCTEXT("Moba", "HealDesc", "체력을 80(+챔피언 레벨당 15) 회복하고 1초 동안 이동 속도가 30% 오릅니다.");
	TargetingMode = EMobaTargetingMode::None;
	bSummonerSpell = true;
	ManaCost = 0.0f;
	CooldownDuration = 240.0f;
}

void UMobaSummoner_Heal::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	if (UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo())
	{
		const float Health = AbilitySystem->GetNumericAttributeBase(UMobaAttributeSet::GetHealthAttribute());
		const float MaxHealth = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxHealthAttribute());
		const float Heal = BaseHeal + HealPerChampionLevel * GetChampionLevel(Caster);
		AbilitySystem->SetNumericAttributeBase(UMobaAttributeSet::GetHealthAttribute(), FMath::Min(Health + Heal, MaxHealth));

		MobaAbility::ApplySpecToTarget(MobaAbility::MakeStatModSpec(AbilitySystem, Caster, HasteDuration, MoveSpeedMultiplier, 1.0f, MobaTags::State_Hasted), Caster);
	}

	FinishCast();
}

UMobaSummoner_Barrier::UMobaSummoner_Barrier()
{
	DisplayName = NSLOCTEXT("Moba", "BarrierName", "방어막");
	Description = NSLOCTEXT("Moba", "BarrierDesc", "2.5초 동안 피해를 95(+챔피언 레벨당 15)만큼 흡수하는 보호막을 얻습니다.");
	TargetingMode = EMobaTargetingMode::None;
	bSummonerSpell = true;
	ManaCost = 0.0f;
	CooldownDuration = 180.0f;
}

void UMobaSummoner_Barrier::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	Caster->AddShield(BaseShield + ShieldPerChampionLevel * GetChampionLevel(Caster), ShieldDuration);
	FinishCast();
}

// ---------------------------------------------------------------------------------------------------------------------
// 대상 지정

UMobaSummoner_Targeted::UMobaSummoner_Targeted()
{
	TargetingMode = EMobaTargetingMode::Location;
	bSummonerSpell = true;
	ManaCost = 0.0f;
}

bool UMobaSummoner_Targeted::ResolveTarget(const AMobaCharacter* Caster, FVector& InOutTargetLocation) const
{
	PendingTarget.Reset();

	// 커서가 사거리 밖을 가리켜도 그 근처의 대상은 고를 수 있게, 사거리로 당기기 전의 위치를 쓴다.
	const FVector Cursor = InOutTargetLocation;
	TArray<AActor*> Candidates;
	MobaAbility::FindEnemiesInRadius(Caster, Cursor, TargetPickRadius, Candidates);

	const UMobaVisionSubsystem* Vision = UMobaVisionSubsystem::Get(Caster);
	const EMobaTeam CasterTeam = MobaCombat::GetTeam(Caster);
	float BestDistance = TNumericLimits<float>::Max();

	for (AActor* Candidate : Candidates)
	{
		if (!IsAllowedTarget(Candidate) || MobaCombat::GetDistanceToTargetEdge(Caster, Candidate) > CastRange)
		{
			continue;
		}
		if (Vision && Vision->IsHiddenFromTeam(Candidate, CasterTeam))
		{
			continue;
		}

		const float Distance = FVector::DistSquared2D(Candidate->GetActorLocation(), Cursor);
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			PendingTarget = Candidate;
		}
	}

	if (!PendingTarget.IsValid())
	{
		return false;
	}

	InOutTargetLocation = PendingTarget->GetActorLocation();
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
// 점화

UMobaSummoner_Ignite::UMobaSummoner_Ignite()
{
	DisplayName = NSLOCTEXT("Moba", "IgniteName", "점화");
	Description = NSLOCTEXT("Moba", "IgniteDesc", "커서 근처의 적 챔피언을 5초 동안 불태워 고정 피해 70(+챔피언 레벨당 20)을 줍니다.");
	CastRange = 600.0f;
	CooldownDuration = 180.0f;
}

bool UMobaSummoner_Ignite::IsAllowedTarget(const AActor* Candidate) const
{
	return Candidate && Candidate->IsA<AMobaCharacter>();
}

void UMobaSummoner_Ignite::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	BurnTarget = PendingTarget;
	BurnSource = Caster;
	RemainingTicks = FMath::Max(BurnTicks, 1);
	DamagePerTick = (BaseTotalDamage + TotalDamagePerChampionLevel * GetChampionLevel(Caster)) / RemainingTicks;
	PendingTarget.Reset();

	// 첫 피해는 1초 뒤. 스킬 자체는 바로 끝내고 피해는 타이머로 이어 간다.
	// EndAbility가 이 객체에 묶인 타이머를 모두 지우므로, 객체에 묶이지 않은 람다로 건다.
	TWeakObjectPtr<UMobaSummoner_Ignite> WeakThis(this);
	Caster->GetWorldTimerManager().SetTimer(BurnTimer, FTimerDelegate::CreateLambda([WeakThis]()
	{
		if (UMobaSummoner_Ignite* Ignite = WeakThis.Get())
		{
			Ignite->ApplyBurnTick();
		}
	}), 1.0f, true);
	MobaEffects::Play(Caster, EMobaEffect::Hit, TargetLocation, 1.0f, BurnTarget.Get());

	FinishCast();
}

void UMobaSummoner_Ignite::ApplyBurnTick()
{
	AMobaCharacter* Source = BurnSource.Get();
	AActor* Target = BurnTarget.Get();
	if (!Source || !Target || MobaCombat::IsDead(Target) || --RemainingTicks < 0)
	{
		if (Source)
		{
			Source->GetWorldTimerManager().ClearTimer(BurnTimer);
		}
		return;
	}

	MobaAbility::ApplySpecToTarget(MobaAbility::MakeDamageSpec(MobaCombat::GetAbilitySystem(Source), Source, 0.0f, 0.0f, DamagePerTick), Target);

	if (RemainingTicks == 0)
	{
		Source->GetWorldTimerManager().ClearTimer(BurnTimer);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// 강타

UMobaSummoner_Smite::UMobaSummoner_Smite()
{
	DisplayName = NSLOCTEXT("Moba", "SmiteName", "강타");
	Description = NSLOCTEXT("Moba", "SmiteDesc", "커서 근처의 몬스터나 적 미니언에게 고정 피해 600을 줍니다.");
	CastRange = 500.0f;
	CooldownDuration = 90.0f;
}

bool UMobaSummoner_Smite::IsAllowedTarget(const AActor* Candidate) const
{
	// 폰 중 챔피언이 아닌 것은 미니언과 몬스터다.
	return Candidate && !Candidate->IsA<AMobaCharacter>();
}

void UMobaSummoner_Smite::ExecuteCast(AMobaCharacter* Caster, const FVector& TargetLocation)
{
	if (AActor* Target = PendingTarget.Get())
	{
		MobaAbility::ApplySpecToTarget(MobaAbility::MakeDamageSpec(GetAbilitySystemComponentFromActorInfo(), Caster, 0.0f, 0.0f, Damage), Target);
		MobaEffects::Play(Caster, EMobaEffect::Hit, MobaCombat::GetAimLocation(Target), 2.0f, Target);
	}
	PendingTarget.Reset();

	FinishCast();
}
