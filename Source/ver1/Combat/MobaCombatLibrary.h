#pragma once

#include "CoreMinimal.h"
#include "Game/MobaTypes.h"

class AActor;
class UAbilitySystemComponent;

namespace MobaCombat
{
	VER1_API UAbilitySystemComponent* GetAbilitySystem(const AActor* Actor);

	VER1_API EMobaTeam GetTeam(const AActor* Actor);

	VER1_API bool IsDead(const AActor* Actor);

	/** 대상이 살아 있고, 능력 시스템을 가지며, 공격자와 다른 팀인지 확인한다. */
	VER1_API bool IsValidEnemyTarget(const AActor* Source, const AActor* Target);

	/** 수평 거리에서 대상의 충돌 반지름을 뺀 값. 사거리 판정에 쓴다. */
	VER1_API float GetDistanceToTargetEdge(const AActor* Source, const AActor* Target);
}
