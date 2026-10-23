#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "MobaGameplayEffects.generated.h"

/**
 * 스킬 재사용 대기 이펙트. 지속 시간은 MobaTags::Data_Cooldown으로,
 * 칸별 쿨다운 태그는 스펙의 DynamicGrantedTags로 넘긴다.
 */
UCLASS()
class VER1_API UMobaCooldownEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UMobaCooldownEffect();
};

/** 마나를 즉시 더하거나 뺀다. 변화량은 MobaTags::Data_Cost_Mana로 넘긴다. */
UCLASS()
class VER1_API UMobaManaCostEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UMobaManaCostEffect();
};

/**
 * 일정 시간 동안 이동 속도와 공격 속도에 배율을 건다. 둔화와 가속 모두 이것으로 처리한다.
 * Data_Duration, Data_Multiplier_MoveSpeed, Data_Multiplier_AttackSpeed를 모두 넘겨야 한다.
 */
UCLASS()
class VER1_API UMobaStatModEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UMobaStatModEffect();
};

/** 수치 변화 없이 일정 시간 동안 태그만 부여한다(기절 등). 태그는 DynamicGrantedTags로 넘긴다. */
UCLASS()
class VER1_API UMobaTimedTagEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UMobaTimedTagEffect();
};

/**
 * 일정 시간 동안 공격력·주문력·방어력·마법 저항력·이동 속도를 더하고 버프 태그를 부여한다. 정글 보상에 쓴다.
 * Data_Duration과 Data_Bonus_*를 모두 넘겨야 하고(MobaAbility::MakeStatBuffSpec), 태그는 DynamicGrantedTags로 넘긴다.
 */
UCLASS()
class VER1_API UMobaAttributeBuffEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UMobaAttributeBuffEffect();
};
