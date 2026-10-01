#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MobaCombatEvents.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnMobaDamageDealt, AActor* /*Instigator*/, AActor* /*Victim*/, float /*Damage*/);

/** 전투 중 일어난 일을 다른 유닛이 들을 수 있게 전달한다. 서버에서만 호출된다. */
UCLASS()
class VER1_API UMobaCombatEvents : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UMobaCombatEvents* Get(const UObject* WorldContextObject);

	/** 피해가 실제로 들어간 직후. 타워와 미니언이 아군 챔피언을 공격한 적을 노릴 때 쓴다. */
	FOnMobaDamageDealt OnDamageDealt;
};
