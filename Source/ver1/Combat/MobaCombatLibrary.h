#pragma once

#include "CoreMinimal.h"
#include "Game/MobaTypes.h"

class AActor;
class ACharacter;
class UAbilitySystemComponent;
class UMobaAttributeSet;

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

	/** Target이 은신 중이고, ViewerTeam의 제어 와드 근처가 아니어서 그 팀에게 보이지 않는지. */
	VER1_API bool IsStealthedFrom(const AActor* Target, EMobaTeam ViewerTeam);

	/**
	 * 서버 전용. 챔피언이 아닌 유닛(미니언·몬스터)의 이동을 능력치와 상태에 맞춘다.
	 * 이동 속도 능력치(둔화·가속 포함)를 걷는 속도에 반영하고, 기절·에어본·속박 중에는 멈춰 세운다.
	 */
	VER1_API void BindUnitMovement(ACharacter* Unit, UAbilitySystemComponent* AbilitySystem, UMobaAttributeSet* AttributeSet);

	/** 수평 거리에서 대상의 충돌 반지름을 뺀 값. 사거리 판정에 쓴다. */
	VER1_API float GetDistanceToTargetEdge(const AActor* Source, const AActor* Target);
}
