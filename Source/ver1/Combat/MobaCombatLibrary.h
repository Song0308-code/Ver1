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

	/** 넥서스가 파괴되어 경기가 끝났는지. */
	VER1_API bool IsMatchOver(const UObject* WorldContextObject);

	/** 대상이 살아 있고, 무적이 아니며, 능력 시스템을 가지고, 공격자와 다른 팀인지 확인한다. */
	VER1_API bool IsValidEnemyTarget(const AActor* Source, const AActor* Target);

	/** 투사체가 날아가 맞을 지점. 건물처럼 원점이 바닥에 있는 대상은 몸통 높이로 올려 준다. */
	VER1_API FVector GetAimLocation(const AActor* Target);

	/** 수평 거리에서 대상의 충돌 반지름을 뺀 값. 사거리 판정에 쓴다. */
	VER1_API float GetDistanceToTargetEdge(const AActor* Source, const AActor* Target);
}
