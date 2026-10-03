#include "Game/MobaItems.h"

namespace
{
	TArray<FMobaItemDefinition> BuildItems()
	{
		TArray<FMobaItemDefinition> Items;

		const auto Add = [&Items](const TCHAR* Name, const TCHAR* Description, int32 Cost) -> FMobaItemDefinition&
		{
			FMobaItemDefinition& Item = Items.AddDefaulted_GetRef();
			Item.Name = Name;
			Item.Description = Description;
			Item.Cost = Cost;
			return Item;
		};

		Add(TEXT("긴 검"), TEXT("공격력 +10"), 350).AttackDamage = 10.0f;
		Add(TEXT("마법서"), TEXT("주문력 +20"), 400).AbilityPower = 20.0f;
		Add(TEXT("붉은 수정"), TEXT("체력 +150"), 400).MaxHealth = 150.0f;
		Add(TEXT("푸른 수정"), TEXT("마나 +250"), 350).MaxMana = 250.0f;
		Add(TEXT("천 갑옷"), TEXT("방어력 +15"), 300).Armor = 15.0f;
		Add(TEXT("마법 망토"), TEXT("마법 저항력 +25"), 450).MagicResist = 25.0f;
		Add(TEXT("단검"), TEXT("공격 속도 +12%"), 300).AttackSpeedFraction = 0.12f;

		FMobaItemDefinition& Boots = Add(TEXT("장화"), TEXT("이동 속도 +25 (하나만)"), 300);
		Boots.MoveSpeed = 25.0f;
		Boots.bUnique = true;

		Add(TEXT("대검"), TEXT("공격력 +40"), 1300).AttackDamage = 40.0f;
		Add(TEXT("큰 지팡이"), TEXT("주문력 +60"), 1250).AbilityPower = 60.0f;

		return Items;
	}
}

const TArray<FMobaItemDefinition>& MobaItems::GetItems()
{
	static const TArray<FMobaItemDefinition> Items = BuildItems();
	return Items;
}

const FMobaItemDefinition* MobaItems::GetItem(int32 ItemIndex)
{
	const TArray<FMobaItemDefinition>& Items = GetItems();
	return Items.IsValidIndex(ItemIndex) ? &Items[ItemIndex] : nullptr;
}
