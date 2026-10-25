#pragma once

#include "CoreMinimal.h"

struct FMobaItemDefinition;
class AMobaGameMode;

/**
 * 밸런스 기준. 아이템 가격은 능력치마다 정한 골드 가치로 매기고, 경제·성장 곡선은 라인 하나를
 * 혼자 맡았을 때의 추정치로 확인한다. 수치와 목표 범위의 근거는 Docs/BALANCE.md.
 */
namespace MobaBalance
{
	/** 능력치 1당 골드 가치. 공격 속도와 생명력 흡수는 1%당. */
	constexpr float GoldPerAttackDamage = 34.0f;
	constexpr float GoldPerAbilityPower = 21.0f;
	constexpr float GoldPerHealth = 2.6f;
	constexpr float GoldPerMana = 1.3f;
	constexpr float GoldPerArmor = 19.0f;
	constexpr float GoldPerMagicResist = 17.0f;
	constexpr float GoldPerAttackSpeedPercent = 24.0f;
	constexpr float GoldPerMoveSpeed = 11.0f;
	constexpr float GoldPerLifeStealPercent = 32.0f;
	constexpr float GoldPerAbilityHaste = 25.0f;
	constexpr float GoldPerCritPercent = 30.0f;
	constexpr float GoldPerArmorPenetration = 28.0f;
	constexpr float GoldPerMagicPenetration = 26.0f;
	constexpr float GoldPerHealthRegen = 40.0f;
	constexpr float GoldPerManaRegen = 50.0f;

	/** 치명타가 된 기본 공격은 공격력 몫이 이만큼 커진다. */
	constexpr float CritDamageMultiplier = 1.75f;

	/** 치명타 확률 Chance에서 0~1 난수 Roll이 치명타인지. */
	inline bool IsCritical(float Chance, float Roll) { return Chance > 0.0f && Roll < Chance; }

	/** 미니언 웨이브가 나와서 라인 가운데에 닿기까지 걸리는 시간(초). 추정에만 쓴다. */
	constexpr float LaneTravelSeconds = 25.0f;

	/** 아이템 능력치를 골드로 환산한 값. 소모품은 0. */
	VER1_API float GetStatGoldValue(const FMobaItemDefinition& Item);

	/** 골드 효율(능력치 가치 / 가격). 1이면 가격만큼의 능력치. */
	VER1_API float GetGoldEfficiency(const FMobaItemDefinition& Item);

	/** 조합 단계. 재료가 없으면 1, 기본 아이템으로 만들면 2, 그 위가 3. */
	VER1_API int32 GetItemTier(int32 ItemIndex);

	/** 레벨 1에서 시작해 TotalExperience만큼 얻었을 때의 레벨. */
	VER1_API int32 GetLevelForExperience(float TotalExperience);

	struct FLaneEstimate
	{
		float Gold = 0.0f;
		float Experience = 0.0f;
		int32 Level = 1;
	};

	/**
	 * 라인 하나를 혼자 맡아 웨이브 경험치를 모두 받고, 미니언 골드는 CsFraction만큼 막타를 쳤다고 할 때
	 * MatchSeconds 시점의 누적 골드(시작 골드·초당 골드 포함)와 경험치. 처치·포탑·정글은 넣지 않는다.
	 */
	VER1_API FLaneEstimate EstimateLane(const AMobaGameMode& Mode, float MatchSeconds, float CsFraction);

	/** 역할별 레벨 1 능력치와 레벨당 성장의 허용 범위(Docs/BALANCE.md 4절). */
	struct FStatRange
	{
		float Min = 0.0f;
		float Max = 0.0f;
		bool Contains(float Value) const { return Value >= Min - KINDA_SMALL_NUMBER && Value <= Max + KINDA_SMALL_NUMBER; }
	};

	struct FChampionRoleRange
	{
		const TCHAR* RoleName = TEXT("");
		FStatRange Health, AttackDamage, Armor, MagicResist, MoveSpeed, AttackRange;
		FStatRange HealthGrowth, AttackDamageGrowth, ArmorGrowth;
	};

	inline FChampionRoleRange GetMarksmanRange()
	{
		return { TEXT("원거리 딜러"), { 560, 620 }, { 54, 62 }, { 22, 28 }, { 28, 32 }, { 325, 335 }, { 525, 575 }, { 90, 105 }, { 2.5f, 3.5f }, { 4.0f, 4.8f } };
	}

	inline FChampionRoleRange GetTankSupportRange()
	{
		return { TEXT("탱커 서포터"), { 620, 690 }, { 56, 64 }, { 32, 40 }, { 30, 34 }, { 330, 345 }, { 125, 175 }, { 100, 115 }, { 3.0f, 4.0f }, { 4.5f, 5.5f } };
	}
}
