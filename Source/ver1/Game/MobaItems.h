#pragma once

#include "CoreMinimal.h"

/** 상점에서 파는 아이템 하나. 사면 수치가 그대로 더해지고, 팔면 다시 빠진다. */
struct FMobaItemDefinition
{
	const TCHAR* Name = TEXT("");
	const TCHAR* Description = TEXT("");
	int32 Cost = 0;

	float AttackDamage = 0.0f;
	float AbilityPower = 0.0f;
	float MaxHealth = 0.0f;
	float MaxMana = 0.0f;
	float Armor = 0.0f;
	float MagicResist = 0.0f;
	float MoveSpeed = 0.0f;

	/** 기본 공격 속도에 곱해 더하는 비율. 0.12면 12%. */
	float AttackSpeedFraction = 0.0f;

	/** true면 한 개만 가질 수 있다. */
	bool bUnique = false;
};

namespace MobaItems
{
	/** 가방 칸 수. */
	constexpr int32 MaxInventorySlots = 6;

	/** 팔 때 돌려받는 비율. */
	constexpr float SellRefundFraction = 0.7f;

	/** 상점 목록. 아이템 번호는 이 배열의 인덱스다. 에셋이 들어오기 전까지 코드로 정의한다. */
	VER1_API const TArray<FMobaItemDefinition>& GetItems();

	/** 번호가 범위를 벗어나면 nullptr. */
	VER1_API const FMobaItemDefinition* GetItem(int32 ItemIndex);
}
