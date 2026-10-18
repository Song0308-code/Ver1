#pragma once

#include "CoreMinimal.h"

/**
 * 상점에서 파는 아이템 하나. 사면 수치가 그대로 더해지고, 팔면 다시 빠진다.
 * 재료(Components)가 있는 아이템은 가진 재료만큼 싸게 살 수 있고, 사면 그 재료가 가방에서 사라진다.
 */
struct FMobaItemDefinition
{
	const TCHAR* Name = TEXT("");
	const TCHAR* Description = TEXT("");

	/** 재료까지 포함한 총 가격. 팔 때도 이 값을 기준으로 돌려준다. */
	int32 Cost = 0;

	/** 재료 아이템 번호. 같은 재료가 두 번 들어갈 수 있다. 재료는 항상 자기보다 앞 번호다. */
	TArray<int32> Components;

	float AttackDamage = 0.0f;
	float AbilityPower = 0.0f;
	float MaxHealth = 0.0f;
	float MaxMana = 0.0f;
	float Armor = 0.0f;
	float MagicResist = 0.0f;
	float MoveSpeed = 0.0f;

	/** 기본 공격 속도에 곱해 더하는 비율. 0.12면 12%. */
	float AttackSpeedFraction = 0.0f;

	/** 생명력 흡수 비율. 0.1이면 10%. */
	float LifeSteal = 0.0f;

	float AbilityHaste = 0.0f;

	/** 소모품은 가방 칸을 쓰지 않고 개수로 쌓인다(최대 MaxPotions). 키 1로 써서 HealAmount를 HealDuration초에 걸쳐 회복한다. */
	bool bConsumable = false;
	float HealAmount = 0.0f;
	float HealDuration = 0.0f;

	/** 같은 묶음 이름의 아이템은 하나만 가질 수 있다(예: 장화 계열). 비어 있으면 제한 없음. */
	const TCHAR* UniqueGroup = TEXT("");

	bool IsBasic() const { return Components.Num() == 0; }
};

/** 아이템 하나를 살 때 실제로 내는 값과 재료로 쓰여 사라질 가방 칸. */
struct FMobaPurchasePlan
{
	int32 Cost = 0;

	/** 가방 칸 번호. 큰 번호부터 들어 있어 이 순서대로 지우면 된다. */
	TArray<int32> ConsumedSlots;
};

namespace MobaItems
{
	/** 가방 칸 수. */
	constexpr int32 MaxInventorySlots = 6;

	/** 체력 물약을 들고 있을 수 있는 최대 개수. */
	constexpr int32 MaxPotions = 5;

	/** 소모품(체력 물약)의 아이템 번호. 없으면 INDEX_NONE. */
	VER1_API int32 GetPotionItemIndex();

	/** 팔 때 돌려받는 비율. */
	constexpr float SellRefundFraction = 0.7f;

	/** 상점 목록. 아이템 번호는 이 배열의 인덱스다. 에셋이 들어오기 전까지 코드로 정의한다. */
	VER1_API const TArray<FMobaItemDefinition>& GetItems();

	/** 번호가 범위를 벗어나면 nullptr. */
	VER1_API const FMobaItemDefinition* GetItem(int32 ItemIndex);

	/**
	 * 가방(Owned)을 가진 상태에서 ItemIndex를 살 때의 값과 쓰일 재료를 계산한다.
	 * 재료를 가지고 있으면 그 재료의 총 가격만큼, 없으면 그 재료의 재료를 가지고 있는지 다시 찾아 깎는다.
	 * 골드·칸 수·하나만 가질 수 있는지는 확인하지 않는다(CanPurchase 참고).
	 */
	VER1_API FMobaPurchasePlan PlanPurchase(int32 ItemIndex, const TArray<uint8>& Owned);

	/** 칸 수와 하나만 가질 수 있는 묶음 규칙상 살 수 있는지. 골드는 보지 않는다. */
	VER1_API bool CanPurchase(int32 ItemIndex, const TArray<uint8>& Owned, const FMobaPurchasePlan& Plan);

	/** "긴 검 + 붉은 수정"처럼 재료 이름을 이은 문자열. 재료가 없으면 빈 문자열. */
	VER1_API FString DescribeComponents(int32 ItemIndex);
}
