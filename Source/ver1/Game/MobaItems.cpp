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

		// 가격은 Game/MobaBalance.h의 능력치당 골드 가치로 매긴다. 기본은 효율 95~105%,
		// 2단계는 100~120%, 3단계는 100~115%(조합할수록 약간 이득). 기준은 Docs/BALANCE.md.

		// 기본 아이템.
		const int32 LongSword = Add(TEXT("긴 검"), TEXT("공격력 +12"), 400);
		Items[LongSword].AttackDamage = 12.0f;
		const int32 Tome = Add(TEXT("마법서"), TEXT("주문력 +20"), 420);
		Items[Tome].AbilityPower = 20.0f;
		const int32 RubyCrystal = Add(TEXT("붉은 수정"), TEXT("체력 +160"), 410);
		Items[RubyCrystal].MaxHealth = 160.0f;
		const int32 SapphireCrystal = Add(TEXT("푸른 수정"), TEXT("마나 +300"), 380);
		Items[SapphireCrystal].MaxMana = 300.0f;
		const int32 ClothArmor = Add(TEXT("천 갑옷"), TEXT("방어력 +16"), 300);
		Items[ClothArmor].Armor = 16.0f;
		const int32 NullMantle = Add(TEXT("마법 망토"), TEXT("마법 저항력 +22"), 370);
		Items[NullMantle].MagicResist = 22.0f;
		const int32 Dagger = Add(TEXT("단검"), TEXT("공격 속도 +13%"), 310);
		Items[Dagger].AttackSpeedFraction = 0.13f;
		const int32 Boots = Add(TEXT("장화"), TEXT("이동 속도 +25"), 280);
		Items[Boots].MoveSpeed = 25.0f;
		Items[Boots].UniqueGroup = TEXT("Boots");
		const int32 BigSword = Add(TEXT("대검"), TEXT("공격력 +37"), 1250);
		Items[BigSword].AttackDamage = 37.0f;
		const int32 BigRod = Add(TEXT("큰 지팡이"), TEXT("주문력 +60"), 1250);
		Items[BigRod].AbilityPower = 60.0f;

		// 2단계.
		const int32 BattleAxe = Add(TEXT("전투 도끼"), TEXT("공격력 +18, 체력 +250"), 1150, { LongSword, RubyCrystal });
		Items[BattleAxe].AttackDamage = 18.0f;
		Items[BattleAxe].MaxHealth = 250.0f;
		const int32 GaleBow = Add(TEXT("질풍의 활"), TEXT("공격 속도 +30%, 이동 속도 +8"), 760, { Dagger, Dagger });
		Items[GaleBow].AttackSpeedFraction = 0.3f;
		Items[GaleBow].MoveSpeed = 8.0f;
		const int32 WardensPlate = Add(TEXT("수호자의 판금"), TEXT("방어력 +32, 체력 +220"), 1050, { ClothArmor, RubyCrystal });
		Items[WardensPlate].Armor = 32.0f;
		Items[WardensPlate].MaxHealth = 220.0f;
		const int32 SpiritVeil = Add(TEXT("정령의 장막"), TEXT("마법 저항력 +38, 체력 +220"), 1100, { NullMantle, RubyCrystal });
		Items[SpiritVeil].MagicResist = 38.0f;
		Items[SpiritVeil].MaxHealth = 220.0f;
		const int32 ManaOrb = Add(TEXT("마력의 보주"), TEXT("주문력 +35, 마나 +350, 스킬 가속 +12"), 1250, { Tome, SapphireCrystal });
		Items[ManaOrb].AbilityPower = 35.0f;
		Items[ManaOrb].MaxMana = 350.0f;
		Items[ManaOrb].AbilityHaste = 12.0f;
		const int32 VampiricScepter = Add(TEXT("흡혈의 홀"), TEXT("공격력 +16, 생명력 흡수 +10%"), 850, { LongSword });
		Items[VampiricScepter].AttackDamage = 16.0f;
		Items[VampiricScepter].LifeSteal = 0.1f;
		const int32 BerserkerBoots = Add(TEXT("광전사의 군화"), TEXT("이동 속도 +40, 공격 속도 +28%"), 1000, { Boots, Dagger });
		Items[BerserkerBoots].MoveSpeed = 40.0f;
		Items[BerserkerBoots].AttackSpeedFraction = 0.28f;
		Items[BerserkerBoots].UniqueGroup = TEXT("Boots");

		// 3단계.
		const int32 DoomBlade = Add(TEXT("파멸의 대검"), TEXT("공격력 +70, 스킬 가속 +12"), 2650, { BigSword, LongSword, LongSword });
		Items[DoomBlade].AttackDamage = 70.0f;
		Items[DoomBlade].AbilityHaste = 12.0f;
		const int32 ArchmageHat = Add(TEXT("대마법사의 모자"), TEXT("주문력 +115, 스킬 가속 +20"), 2850, { BigRod, Tome, Tome });
		Items[ArchmageHat].AbilityPower = 115.0f;
		Items[ArchmageHat].AbilityHaste = 20.0f;
		const int32 Bulwark = Add(TEXT("수호 군주의 갑주"), TEXT("방어력 +45, 마법 저항력 +45, 체력 +450"), 2700, { WardensPlate, SpiritVeil });
		Items[Bulwark].Armor = 45.0f;
		Items[Bulwark].MagicResist = 45.0f;
		Items[Bulwark].MaxHealth = 450.0f;

		// 소모품. 가방 칸을 쓰지 않으므로 맨 뒤에 둔다(상점에서는 기본 열에 보인다).
		const int32 Potion = Add(TEXT("체력 물약"), TEXT("15초간 체력 120 (최대 5, 키 1)"), 50);
		Items[Potion].bConsumable = true;
		Items[Potion].HealAmount = 120.0f;
		Items[Potion].HealDuration = 15.0f;

		const int32 ControlWard = Add(TEXT("제어 와드"), TEXT("적 와드 노출·차단 (최대 2, 키 3)"), 80);
		Items[ControlWard].bConsumable = true;
		Items[ControlWard].bControlWard = true;

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

int32 MobaItems::GetPotionItemIndex()
{
	const TArray<FMobaItemDefinition>& Items = GetItems();
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		if (Items[Index].bConsumable && Items[Index].HealAmount > 0.0f)
		{
			return Index;
		}
	}
	return INDEX_NONE;
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

TArray<FString> MobaItems::DescribeStats(const FMobaItemDefinition& Item)
{
	TArray<FString> Lines;
	const auto Add = [&Lines](float Value, const TCHAR* Label, const TCHAR* Suffix)
	{
		if (Value != 0.0f)
		{
			Lines.Add(FString::Printf(TEXT("%s +%d%s"), Label, FMath::RoundToInt(Value), Suffix));
		}
	};
	Add(Item.AttackDamage, TEXT("공격력"), TEXT(""));
	Add(Item.AbilityPower, TEXT("주문력"), TEXT(""));
	Add(Item.MaxHealth, TEXT("체력"), TEXT(""));
	Add(Item.MaxMana, TEXT("마나"), TEXT(""));
	Add(Item.Armor, TEXT("방어력"), TEXT(""));
	Add(Item.MagicResist, TEXT("마법 저항력"), TEXT(""));
	Add(Item.AttackSpeedFraction * 100.0f, TEXT("공격 속도"), TEXT("%"));
	Add(Item.MoveSpeed, TEXT("이동 속도"), TEXT(""));
	Add(Item.LifeSteal * 100.0f, TEXT("생명력 흡수"), TEXT("%"));
	Add(Item.AbilityHaste, TEXT("스킬 가속"), TEXT(""));
	if (Item.bConsumable && Item.HealAmount > 0.0f)
	{
		Lines.Add(FString::Printf(TEXT("사용: %d초 동안 체력 %d 회복 (키 1, 최대 %d개)"), FMath::RoundToInt(Item.HealDuration), FMath::RoundToInt(Item.HealAmount), MaxPotions));
	}
	if (Item.bControlWard)
	{
		Lines.Add(FString::Printf(TEXT("사용: 근처 적 와드를 드러내고 끈다 (키 3, 최대 %d개)"), MaxControlWards));
	}
	if (*Item.UniqueGroup)
	{
		Lines.Add(TEXT("장화 계열은 하나만 가질 수 있습니다"));
	}
	return Lines;
}

TArray<int32> MobaItems::GetItemsBuiltFrom(int32 ItemIndex)
{
	TArray<int32> Result;
	const TArray<FMobaItemDefinition>& Items = GetItems();
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		if (Items[Index].Components.Contains(ItemIndex))
		{
			Result.Add(Index);
		}
	}
	return Result;
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
