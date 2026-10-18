#include "Combat/MobaCombatLibrary.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/MobaCharacter.h"
#include "Engine/World.h"
#include "Game/MobaGameState.h"
#include "Game/MobaGameplayTags.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

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

	// 정글 몬스터는 챔피언하고만 싸운다. 미니언과 타워는 몬스터를 건드리지 않는다.
	if (SourceTeam == EMobaTeam::Neutral && !Target->IsA<AMobaCharacter>())
	{
		return false;
	}
	if (TargetTeam == EMobaTeam::Neutral && !Source->IsA<AMobaCharacter>())
	{
		return false;
	}

	const UAbilitySystemComponent* TargetAbilitySystem = GetAbilitySystem(Target);
	return TargetAbilitySystem
		&& !TargetAbilitySystem->HasMatchingGameplayTag(MobaTags::State_Dead)
		&& !TargetAbilitySystem->HasMatchingGameplayTag(MobaTags::State_Invulnerable);
}

bool MobaCombat::IsMatchOver(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const AMobaGameState* GameState = World ? World->GetGameState<AMobaGameState>() : nullptr;
	return GameState && GameState->IsMatchOver();
}

FVector MobaCombat::GetAimLocation(const AActor* Target)
{
	// 폰은 캡슐 중심이 원점이라 그대로 쓴다.
	if (Target->IsA<APawn>())
	{
		return Target->GetActorLocation();
	}

	return Target->GetActorLocation() + FVector(0.0, 0.0, FMath::Min(Target->GetSimpleCollisionHalfHeight(), 150.0f));
}

float MobaCombat::GetDistanceToTargetEdge(const AActor* Source, const AActor* Target)
{
	const float CenterDistance = FVector::Dist2D(Source->GetActorLocation(), Target->GetActorLocation());
	return FMath::Max(CenterDistance - Target->GetSimpleCollisionRadius(), 0.0f);
}

void MobaCombat::BindUnitMovement(ACharacter* Unit, UAbilitySystemComponent* AbilitySystem, UMobaAttributeSet* AttributeSet)
{
	if (!Unit || !AbilitySystem || !AttributeSet)
	{
		return;
	}

	UCharacterMovementComponent* Movement = Unit->GetCharacterMovement();
	Movement->MaxWalkSpeed = AttributeSet->GetMoveSpeed();
	AttributeSet->OnMoveSpeedChanged.AddWeakLambda(Unit, [Movement](float NewMoveSpeed)
	{
		Movement->MaxWalkSpeed = NewMoveSpeed;
	});

	const auto UpdateLock = [Unit, AbilitySystem, Movement](const FGameplayTag, int32)
	{
		const bool bLocked = AbilitySystem->HasMatchingGameplayTag(MobaTags::State_Stunned) || AbilitySystem->HasMatchingGameplayTag(MobaTags::State_Rooted);
		if (bLocked)
		{
			if (AController* Controller = Unit->GetController())
			{
				Controller->StopMovement();
			}
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
		else if (Movement->MovementMode == MOVE_None && !AbilitySystem->HasMatchingGameplayTag(MobaTags::State_Dead))
		{
			Movement->SetMovementMode(MOVE_Walking);
		}
	};
	AbilitySystem->RegisterGameplayTagEvent(MobaTags::State_Stunned, EGameplayTagEventType::NewOrRemoved).AddWeakLambda(Unit, UpdateLock);
	AbilitySystem->RegisterGameplayTagEvent(MobaTags::State_Rooted, EGameplayTagEventType::NewOrRemoved).AddWeakLambda(Unit, UpdateLock);
}
