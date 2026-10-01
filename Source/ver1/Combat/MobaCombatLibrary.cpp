#include "Combat/MobaCombatLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Game/MobaGameplayTags.h"
#include "GameFramework/Actor.h"

UAbilitySystemComponent* MobaCombat::GetAbilitySystem(const AActor* Actor)
{
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
}

EMobaTeam MobaCombat::GetTeam(const AActor* Actor)
{
	const IMobaTeamInterface* TeamActor = Cast<IMobaTeamInterface>(Actor);
	return TeamActor ? TeamActor->GetTeam() : EMobaTeam::None;
}

bool MobaCombat::IsDead(const AActor* Actor)
{
	const UAbilitySystemComponent* AbilitySystem = GetAbilitySystem(Actor);
	return AbilitySystem && AbilitySystem->HasMatchingGameplayTag(MobaTags::State_Dead);
}

bool MobaCombat::IsValidEnemyTarget(const AActor* Source, const AActor* Target)
{
	if (!IsValid(Source) || !IsValid(Target) || Source == Target)
	{
		return false;
	}

	const EMobaTeam SourceTeam = GetTeam(Source);
	const EMobaTeam TargetTeam = GetTeam(Target);
	if (SourceTeam == EMobaTeam::None || TargetTeam == EMobaTeam::None || SourceTeam == TargetTeam)
	{
		return false;
	}

	return GetAbilitySystem(Target) != nullptr && !IsDead(Target);
}

float MobaCombat::GetDistanceToTargetEdge(const AActor* Source, const AActor* Target)
{
	const float CenterDistance = FVector::Dist2D(Source->GetActorLocation(), Target->GetActorLocation());
	return FMath::Max(CenterDistance - Target->GetSimpleCollisionRadius(), 0.0f);
}
