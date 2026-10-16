// 게임 규칙 자동화 테스트. 에디터의 세션 프론트엔드 → 자동화에서 "Moba"로 찾거나,
// 명령줄에서 -ExecCmds="Automation RunTests Moba; Quit"으로 실행한다.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Abilities/MobaAbilityArchetypes.h"
#include "AbilitySystem/MobaDamageExecution.h"
#include "Character/MobaChampionData.h"
#include "Character/MobaPassive.h"
#include "Game/MobaGameMode.h"
#include "Game/MobaItems.h"
#include "Map/MobaJungleCamp.h"
#include "Player/MobaPlayerState.h"

namespace
{
	constexpr EAutomationTestFlags MobaTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMobaDamageResistanceTest, "Moba.Rules.DamageResistance", MobaTestFlags)
bool FMobaDamageResistanceTest::RunTest(const FString& Parameters)
{
	// 방어력 100이면 피해가 절반, 0이면 그대로, 음수면 더 많이 받는다.
	TestEqual(TEXT("저항 0"), UMobaDamageExecution::GetResistanceMultiplier(0.0f), 1.0f);
	TestEqual(TEXT("저항 100"), UMobaDamageExecution::GetResistanceMultiplier(100.0f), 0.5f);
	TestEqual(TEXT("저항 50"), UMobaDamageExecution::GetResistanceMultiplier(50.0f), 100.0f / 150.0f, KINDA_SMALL_NUMBER);
	TestTrue(TEXT("음수 저항은 피해를 늘린다"), UMobaDamageExecution::GetResistanceMultiplier(-20.0f) > 1.0f);
	TestTrue(TEXT("음수 저항도 두 배를 넘지 않는다"), UMobaDamageExecution::GetResistanceMultiplier(-1000.0f) < 2.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMobaKillBountyTest, "Moba.Rules.KillBounty", MobaTestFlags)
bool FMobaKillBountyTest::RunTest(const FString& Parameters)
{
	const AMobaGameMode* Mode = GetDefault<AMobaGameMode>();

	TestEqual(TEXT("기본 현상금"), Mode->CalculateKillBounty(0, 0), 300.0f);
	TestEqual(TEXT("1연속 처치는 기본"), Mode->CalculateKillBounty(1, 0), 300.0f);
	TestEqual(TEXT("3연속 처치 중인 상대"), Mode->CalculateKillBounty(3, 0), 600.0f);
	TestEqual(TEXT("현상금 상한"), Mode->CalculateKillBounty(20, 0), 1000.0f);
	TestEqual(TEXT("1연속 죽음은 기본"), Mode->CalculateKillBounty(0, 1), 300.0f);
	TestEqual(TEXT("2연속 죽음"), Mode->CalculateKillBounty(0, 2), 250.0f);
	TestEqual(TEXT("현상금 하한"), Mode->CalculateKillBounty(0, 50), 100.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMobaRespawnTimeTest, "Moba.Rules.RespawnTime", MobaTestFlags)
bool FMobaRespawnTimeTest::RunTest(const FString& Parameters)
{
	const AMobaGameMode* Mode = GetDefault<AMobaGameMode>();

	TestEqual(TEXT("1레벨"), Mode->GetRespawnTime(1), 5.0f);
	TestEqual(TEXT("6레벨"), Mode->GetRespawnTime(6), 17.5f);
	TestEqual(TEXT("18레벨은 상한"), Mode->GetRespawnTime(18), 47.5f);
	TestTrue(TEXT("레벨이 오르면 줄지 않는다"), Mode->GetRespawnTime(10) >= Mode->GetRespawnTime(9));
	TestTrue(TEXT("상한 50초"), Mode->GetRespawnTime(100) <= 50.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMobaMinionGrowthTest, "Moba.Rules.MinionGrowth", MobaTestFlags)
bool FMobaMinionGrowthTest::RunTest(const FString& Parameters)
{
	const AMobaGameMode* Mode = GetDefault<AMobaGameMode>();

	TestEqual(TEXT("시작 직후"), Mode->GetMinionUpgradeCount(0.0f), 0);
	TestEqual(TEXT("89초"), Mode->GetMinionUpgradeCount(89.0f), 0);
	TestEqual(TEXT("90초"), Mode->GetMinionUpgradeCount(90.0f), 1);
	TestEqual(TEXT("100초 웨이브"), Mode->GetMinionUpgradeCount(100.0f), 1);
	TestEqual(TEXT("상한 20회"), Mode->GetMinionUpgradeCount(100000.0f), 20);

	// 미니언 기본 수치가 말이 되는지.
	for (const EMobaMinionType Type : { EMobaMinionType::Melee, EMobaMinionType::Caster, EMobaMinionType::Super })
	{
		const FMobaMinionConfig& Config = Mode->GetMinionConfig(Type);
		TestTrue(TEXT("미니언 체력"), Config.Stats.MaxHealth > 0.0f);
		TestTrue(TEXT("미니언 공격력"), Config.Stats.AttackDamage > 0.0f);
		TestTrue(TEXT("미니언 골드"), Config.GoldReward > 0.0f);
	}
	TestTrue(TEXT("원거리 미니언이 근접보다 사거리가 길다"), Mode->GetMinionConfig(EMobaMinionType::Caster).Stats.AttackRange > Mode->GetMinionConfig(EMobaMinionType::Melee).Stats.AttackRange);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMobaExperienceCurveTest, "Moba.Rules.ExperienceCurve", MobaTestFlags)
bool FMobaExperienceCurveTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("1→2레벨"), AMobaPlayerState::GetExperienceForNextLevel(1), 280.0f);
	TestEqual(TEXT("2→3레벨"), AMobaPlayerState::GetExperienceForNextLevel(2), 380.0f);
	for (int32 Level = 1; Level < AMobaPlayerState::MaxChampionLevel; ++Level)
	{
		TestTrue(FString::Printf(TEXT("%d레벨 필요 경험치가 늘어난다"), Level), AMobaPlayerState::GetExperienceForNextLevel(Level + 1) > AMobaPlayerState::GetExperienceForNextLevel(Level));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMobaUltimateLevelTest, "Moba.Rules.UltimateLevels", MobaTestFlags)
bool FMobaUltimateLevelTest::RunTest(const FString& Parameters)
{
	// 궁극기는 6, 11, 16레벨에 한 단계씩, 최대 3단계.
	const UMobaGameplayAbility* Ultimate = GetDefault<UMobaAbility_GroundAoE>();
	TestEqual(TEXT("궁극기 최대 단계"), Ultimate->GetMaxLevel(), 3);
	TestFalse(TEXT("5레벨에는 못 배운다"), Ultimate->CanLevelUp(0, 5));
	TestTrue(TEXT("6레벨에 배운다"), Ultimate->CanLevelUp(0, 6));
	TestFalse(TEXT("10레벨에는 2단계 불가"), Ultimate->CanLevelUp(1, 10));
	TestTrue(TEXT("11레벨에 2단계"), Ultimate->CanLevelUp(1, 11));
	TestTrue(TEXT("16레벨에 3단계"), Ultimate->CanLevelUp(2, 16));
	TestFalse(TEXT("3단계가 끝"), Ultimate->CanLevelUp(3, 18));

	// 일반 스킬은 레벨 제한 없이 5단계까지.
	const UMobaGameplayAbility* Basic = GetDefault<UMobaAbility_Skillshot>();
	TestEqual(TEXT("일반 스킬 최대 단계"), Basic->GetMaxLevel(), 5);
	TestTrue(TEXT("1레벨에 배운다"), Basic->CanLevelUp(0, 1));
	TestFalse(TEXT("5단계가 끝"), Basic->CanLevelUp(5, 18));
	TestFalse(TEXT("뱅가드 R도 궁극기"), GetDefault<UMobaAbility_Frenzy>()->CanLevelUp(0, 5));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMobaItemTableTest, "Moba.Data.Items", MobaTestFlags)
bool FMobaItemTableTest::RunTest(const FString& Parameters)
{
	const TArray<FMobaItemDefinition>& Items = MobaItems::GetItems();
	TestTrue(TEXT("아이템이 있다"), Items.Num() > 0);
	TestTrue(TEXT("상점 화면에 다 들어간다"), Items.Num() <= 12);
	TestNull(TEXT("범위 밖 번호"), MobaItems::GetItem(Items.Num()));

	int32 UniqueCount = 0;
	for (const FMobaItemDefinition& Item : Items)
	{
		TestTrue(FString::Printf(TEXT("%s 가격"), Item.Name), Item.Cost > 0);
		const bool bHasStat = Item.AttackDamage > 0.0f || Item.AbilityPower > 0.0f || Item.MaxHealth > 0.0f || Item.MaxMana > 0.0f
			|| Item.Armor > 0.0f || Item.MagicResist > 0.0f || Item.MoveSpeed > 0.0f || Item.AttackSpeedFraction > 0.0f;
		TestTrue(FString::Printf(TEXT("%s 능력치"), Item.Name), bHasStat);
		UniqueCount += Item.bUnique ? 1 : 0;
	}
	TestEqual(TEXT("하나만 가질 수 있는 아이템(장화)"), UniqueCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMobaJungleCampTest, "Moba.Data.JungleCamps", MobaTestFlags)
bool FMobaJungleCampTest::RunTest(const FString& Parameters)
{
	const UEnum* CampEnum = StaticEnum<EMobaCampType>();
	for (int32 Index = 0; Index < CampEnum->NumEnums() - 1; ++Index)
	{
		const EMobaCampType Type = static_cast<EMobaCampType>(CampEnum->GetValueByIndex(Index));
		const FMobaCampDefinition Definition = AMobaJungleCamp::GetDefinition(Type);
		const FString Name = CampEnum->GetNameStringByIndex(Index);

		TestTrue(Name + TEXT(" 몬스터"), Definition.Monsters.Num() > 0);
		TestTrue(Name + TEXT(" 이름"), !Definition.DisplayName.IsEmpty());
		TestTrue(Name + TEXT(" 첫 등장"), Definition.InitialSpawnTime > 0.0f);
		TestTrue(Name + TEXT(" 재생성"), Definition.RespawnTime > 0.0f);
		for (const FMobaMonsterConfig& Monster : Definition.Monsters)
		{
			TestTrue(Name + TEXT(" 몬스터 체력"), Monster.Stats.MaxHealth > 0.0f);
		}
	}

	TestTrue(TEXT("바위게는 반격하지 않는다"), AMobaJungleCamp::GetDefinition(EMobaCampType::Scuttle).Monsters[0].bPassive);
	TestTrue(TEXT("바론은 드래곤보다 늦게 나온다"), AMobaJungleCamp::GetDefinition(EMobaCampType::Baron).InitialSpawnTime > AMobaJungleCamp::GetDefinition(EMobaCampType::Dragon).InitialSpawnTime);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMobaChampionDataTest, "Moba.Data.Champions", MobaTestFlags)
bool FMobaChampionDataTest::RunTest(const FString& Parameters)
{
	const TArray<FSoftObjectPath>& Roster = GetDefault<AMobaGameMode>()->GetChampionRoster();
	TestTrue(TEXT("고를 수 있는 챔피언이 셋 이상"), Roster.Num() >= 3);

	for (const FSoftObjectPath& Path : Roster)
	{
		const UMobaChampionData* Champion = Cast<UMobaChampionData>(Path.TryLoad());
		const FString Name = Path.GetAssetName();
		if (!TestNotNull(Name + TEXT(" 로드"), Champion))
		{
			continue;
		}

		TestFalse(Name + TEXT(" 이름"), Champion->DisplayName.IsEmpty());
		TestEqual(Name + TEXT(" 스킬 4개"), Champion->Abilities.Num(), 4);
		TestNotNull(Name + TEXT(" 패시브"), Champion->Passive.Get());
		TestTrue(Name + TEXT(" 체력"), Champion->BaseStats.MaxHealth > 0.0f);
		TestTrue(Name + TEXT(" 사거리"), Champion->BaseStats.AttackRange > 0.0f);
		TestFalse(Name + TEXT(" 메시"), Champion->Mesh.IsNull());

		int32 Ultimates = 0;
		for (const TPair<EMobaAbilitySlot, TSubclassOf<UMobaGameplayAbility>>& Entry : Champion->Abilities)
		{
			const UMobaGameplayAbility* Ability = Entry.Value ? Entry.Value->GetDefaultObject<UMobaGameplayAbility>() : nullptr;
			if (TestNotNull(Name + TEXT(" 스킬 클래스"), Ability))
			{
				TestFalse(Name + TEXT(" 스킬 설명"), Ability->GetDescription().IsEmpty());
				Ultimates += Ability->GetMaxLevel() == 3 ? 1 : 0;
				TestEqual(Name + TEXT(" 궁극기는 R"), Ability->GetMaxLevel() == 3, Entry.Key == EMobaAbilitySlot::R);
			}
		}
		TestEqual(Name + TEXT(" 궁극기 하나"), Ultimates, 1);
	}
	return true;
}

#endif
