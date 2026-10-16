#include "Game/MobaItems.h"

namespace
{
	TArray<FMobaItemDefinition> BuildItems()
	{
		TArray<FMobaItemDefinition> Items;

		const auto Add = [&Items](const TCHAR* Name, const TCHAR* Description, int32 Cost, TArray<int32> Components = {}) -> int32
		{
			FMobaItemDefinition& Item = Items.AddDefaulted_GetRef();
			Item.Name = Name;
			Item.Description = Description;
			Item.Cost = Cost;
			Item.Components = MoveTemp(Components);
			return Items.Num() - 1;
		};

		// 기본 아이템.
		const int32 LongSword = Add(TEXT("긴 검"), TEXT("공격력 +10"), 350);
		Items[LongSword].AttackDamage = 10.0f;
		const int32 Tome = Add(TEXT("마법서"), TEXT("주문력 +20"), 400);
		Items[Tome].AbilityPower = 20.0f;
		const int32 RubyCrystal = Add(TEXT("붉은 수정"), TEXT("체력 +150"), 400);
		Items[RubyCrystal].MaxHealth = 150.0f;
		const int32 SapphireCrystal = Add(TEXT("푸른 수정"), TEXT("마나 +250"), 350);
		Items[SapphireCrystal].MaxMana = 250.0f;
		const int32 ClothArmor = Add(TEXT("천 갑옷"), TEXT("방어력 +15"), 300);
		Items[ClothArmor].Armor = 15.0f;
		const int32 NullMantle = Add(TEXT("마법 망토"), TEXT("마법 저항력 +25"), 450);
		Items[NullMantle].MagicResist = 25.0f;
		const int32 Dagger = Add(TEXT("단검"), TEXT("공격 속도 +12%"), 300);
		Items[Dagger].AttackSpeedFraction = 0.12f;
		const int32 Boots = Add(TEXT("장화"), TEXT("이동 속도 +25"), 300);
		Items[Boots].MoveSpeed = 25.0f;
		Items[Boots].UniqueGroup = TEXT("Boots");
		const int32 BigSword = Add(TEXT("대검"), TEXT("공격력 +40"), 1300);
		Items[BigSword].AttackDamage = 40.0f;
		const int32 BigRod = Add(TEXT("큰 지팡이"), TEXT("주문력 +60"), 1250);
		Items[BigRod].AbilityPower = 60.0f;

		// 2단계.
		const int32 BattleAxe = Add(TEXT("전투 도끼"), TEXT("공격력 +20, 체력 +250"), 1100, { LongSword, RubyCrystal });
		Items[BattleAxe].AttackDamage = 20.0f;
		Items[BattleAxe].MaxHealth = 250.0f;
		const int32 GaleBow = Add(TEXT("질풍의 활"), TEXT("공격 속도 +30%, 이동 속도 +10"), 1000, { Dagger, Dagger });
		Items[GaleBow].AttackSpeedFraction = 0.3f;
		Items[GaleBow].MoveSpeed = 10.0f;
		const int32 WardensPlate = Add(TEXT("수호자의 판금"), TEXT("방어력 +35, 체력 +200"), 1100, { ClothArmor, RubyCrystal });
		Items[WardensPlate].Armor = 35.0f;
		Items[WardensPlate].MaxHealth = 200.0f;
		const int32 SpiritVeil = Add(TEXT("정령의 장막"), TEXT("마법 저항력 +40, 체력 +200"), 1200, { NullMantle, RubyCrystal });
		Items[SpiritVeil].MagicResist = 40.0f;
		Items[SpiritVeil].MaxHealth = 200.0f;
		const int32 ManaOrb = Add(TEXT("마력의 보주"), TEXT("주문력 +40, 마나 +400"), 1100, { Tome, SapphireCrystal });
		Items[ManaOrb].AbilityPower = 40.0f;
		Items[ManaOrb].MaxMana = 400.0f;
		const int32 BerserkerBoots = Add(TEXT("광전사의 군화"), TEXT("이동 속도 +45, 공격 속도 +30%"), 1100, { Boots, Dagger });
		Items[BerserkerBoots].MoveSpeed = 45.0f;
		Items[BerserkerBoots].AttackSpeedFraction = 0.3f;
		Items[BerserkerBoots].UniqueGroup = TEXT("Boots");

		// 3단계.
		const int32 DoomBlade = Add(TEXT("파멸의 대검"), TEXT("공격력 +75"), 3000, { BigSword, LongSword, LongSword });
		Items[DoomBlade].AttackDamage = 75.0f;
		const int32 ArchmageHat = Add(TEXT("대마법사의 모자"), TEXT("주문력 +120"), 3200, { BigRod, Tome, Tome });
		Items[ArchmageHat].AbilityPower = 120.0f;
		const int32 Bulwark = Add(TEXT("수호 군주의 갑주"), TEXT("방어력 +50, 마법 저항력 +50, 체력 +450"), 2900, { WardensPlate, SpiritVeil });
		Items[Bulwark].Armor = 50.0f;
		Items[Bulwark].MagicResist = 50.0f;
		Items[Bulwark].MaxHealth = 450.0f;

		return Items;
	}

	/** Item을 만드는 데 쓸 수 있는 재료를 Owned에서 찾아 Used에 표시하고, 깎이는 금액을 돌려준다. */
	int32 ConsumeComponents(const FMobaItemDefinition& Item, const TArray<uint8>& Owned, TArray<bool>& Used)
	{
		int32 Saved = 0;
		for (const int32 ComponentIndex : Item.Components)
		{
			const FMobaItemDefinition* Component = MobaItems::GetItem(ComponentIndex);
			if (!Component)
			{
				continue;
			}

			int32 Found = INDEX_NONE;
			for (int32 Slot = 0; Slot < Owned.Num(); ++Slot)
			{
				if (!Used[Slot] && Owned[Slot] == ComponentIndex)
				{
					Found = Slot;
					break;
				}
			}

			if (Found != INDEX_NONE)
			{
				Used[Found] = true;
				Saved += Component->Cost;
			}
			else
			{
				// 이 재료는 없지만 그 재료의 재료를 가지고 있을 수 있다.
				Saved += ConsumeComponents(*Component, Owned, Used);
			}
		}
		return Saved;
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

FMobaPurchasePlan MobaItems::PlanPurchase(int32 ItemIndex, const TArray<uint8>& Owned)
{
	FMobaPurchasePlan Plan;
	const FMobaItemDefinition* Item = GetItem(ItemIndex);
	if (!Item)
	{
		return Plan;
	}

	TArray<bool> Used;
	Used.Init(false, Owned.Num());
	Plan.Cost = FMath::Max(Item->Cost - ConsumeComponents(*Item, Owned, Used), 0);

	for (int32 Slot = Owned.Num() - 1; Slot >= 0; --Slot)
	{
		if (Used[Slot])
		{
			Plan.ConsumedSlots.Add(Slot);
		}
	}
	return Plan;
}

bool MobaItems::CanPurchase(int32 ItemIndex, const TArray<uint8>& Owned, const FMobaPurchasePlan& Plan)
{
	const FMobaItemDefinition* Item = GetItem(ItemIndex);
	if (!Item || Owned.Num() - Plan.ConsumedSlots.Num() + 1 > MaxInventorySlots)
	{
		return false;
	}

	// 같은 묶음의 아이템은 재료로 쓰여 사라지는 것이 아니면 함께 가질 수 없다.
	if (*Item->UniqueGroup)
	{
		for (int32 Slot = 0; Slot < Owned.Num(); ++Slot)
		{
			const FMobaItemDefinition* OwnedItem = GetItem(Owned[Slot]);
			if (OwnedItem && FCString::Strcmp(OwnedItem->UniqueGroup, Item->UniqueGroup) == 0 && !Plan.ConsumedSlots.Contains(Slot))
			{
				return false;
			}
		}
	}
	return true;
}

FString MobaItems::DescribeComponents(int32 ItemIndex)
{
	const FMobaItemDefinition* Item = GetItem(ItemIndex);
	if (!Item)
	{
		return FString();
	}

	TArray<FString> Names;
	for (const int32 ComponentIndex : Item->Components)
	{
		if (const FMobaItemDefinition* Component = GetItem(ComponentIndex))
		{
			Names.Add(Component->Name);
		}
	}
	return FString::Join(Names, TEXT(" + "));
}
