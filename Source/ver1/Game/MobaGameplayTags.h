#pragma once

#include "NativeGameplayTags.h"

namespace MobaTags
{
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Stunned);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Slowed);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Hasted);

	/** 속박: 움직일 수 없고 돌진·순간이동 스킬도 못 쓴다. 공격과 다른 스킬은 된다. */
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Rooted);

	/** 침묵: 스킬을 못 쓴다. 이동·공격·소환사 주문은 된다. */
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Silenced);

	/** 은신: 적에게 보이지 않는다(적 제어 와드 근처에서는 드러난다). 공격하거나 스킬을 쓰면 풀린다. */
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Stealthed);

	/** 에어본: 공중에 떠 있는 동안 아무것도 못 한다. 함께 State_Stunned가 붙어 기절처럼 막힌다. */
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Airborne);

	/** 기본 공격 피해 스펙에 붙는다. 생명력 흡수처럼 기본 공격에만 걸리는 효과가 본다. */
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_BasicAttack);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invulnerable);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Recalling);

	/** 정글 보상으로 받는 버프. 효과는 태그를 보고 각 시스템이 적용한다. */
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Buff_Blue);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Buff_Red);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Buff_Dragon);

	/** 원소별 드래곤 보상. Buff_Dragon과 함께 붙고, 개수가 그 원소를 잡은 횟수다. */
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Buff_Dragon_Infernal);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Buff_Dragon_Mountain);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Buff_Dragon_Cloud);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Buff_Dragon_Ocean);

	/** 드래곤 4마리를 잡은 팀의 영혼. 원소 태그(Buff_Soul_*)가 함께 붙는다. */
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Buff_Soul);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Buff_Soul_Ocean);

	/** 장로 드래곤 처치 보상. 적 챔피언을 체력 20% 아래로 깎으면 바로 처치한다. */
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Buff_Elder);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Buff_Baron);

	/** 와드. 피해량과 상관없이 기본 공격 한 번에 체력 1씩만 깎인다. */
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Unit_Ward);

	/** 스킬이 어느 칸(QWER)에 들어 있는지. 어빌리티 스펙에 붙인다. */
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Slot_Q);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Slot_W);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Slot_E);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Slot_R);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Slot_D);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Slot_F);

	/** 칸별 재사용 대기 중 표시. 쿨다운 이펙트가 부여한다. */
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Slot_Q);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Slot_W);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Slot_E);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Slot_R);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Slot_D);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Slot_F);

	/** 데미지 이펙트에 SetByCaller로 넘기는 피해량. */
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage_Physical);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage_Magic);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage_True);

	/** 범용 이펙트에 SetByCaller로 넘기는 값. */
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Cooldown);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Cost_Mana);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Duration);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Multiplier_MoveSpeed);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Multiplier_AttackSpeed);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Bonus_AttackDamage);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Bonus_AbilityPower);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Bonus_Armor);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Bonus_MagicResist);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Bonus_MoveSpeed);
}
