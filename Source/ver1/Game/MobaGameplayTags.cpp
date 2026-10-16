#include "Game/MobaGameplayTags.h"

namespace MobaTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "사망 상태. 부활할 때 제거된다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Stunned, "State.Stunned", "기절. 이동·공격·스킬 사용이 불가능하다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Slowed, "State.Slowed", "이동 속도 감소 중.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Hasted, "State.Hasted", "이동·공격 속도 증가 중.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_BasicAttack, "Damage.BasicAttack", "기본 공격으로 준 피해.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invulnerable, "State.Invulnerable", "무적. 공격 대상이 되지 않는다. 앞선 건물이 남아 있는 건물에 붙는다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Recalling, "State.Recalling", "귀환을 준비하는 중. 움직이거나 맞으면 취소된다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Buff_Blue, "Buff.Blue", "푸른 파수꾼 버프. 마나 재생이 늘고 재사용 대기 시간이 줄어든다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Buff_Red, "Buff.Red", "붉은 덩굴정령 버프. 체력 재생이 늘고 기본 공격이 둔화와 추가 피해를 준다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Buff_Dragon, "Buff.Dragon", "드래곤 처치 보상. 경기 끝까지 유지되고 잡을 때마다 겹친다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Buff_Baron, "Buff.Baron", "내셔 남작 처치 보상.");

	UE_DEFINE_GAMEPLAY_TAG(Ability_Slot_Q, "Ability.Slot.Q");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Slot_W, "Ability.Slot.W");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Slot_E, "Ability.Slot.E");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Slot_R, "Ability.Slot.R");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Slot_D, "Ability.Slot.D");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Slot_F, "Ability.Slot.F");

	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Slot_Q, "Cooldown.Slot.Q");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Slot_W, "Cooldown.Slot.W");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Slot_E, "Cooldown.Slot.E");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Slot_R, "Cooldown.Slot.R");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Slot_D, "Cooldown.Slot.D");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Slot_F, "Cooldown.Slot.F");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage_Physical, "Data.Damage.Physical", "방어력으로 줄어드는 물리 피해량.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage_Magic, "Data.Damage.Magic", "마법 저항력으로 줄어드는 마법 피해량.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage_True, "Data.Damage.True", "줄어들지 않는 고정 피해량.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Cooldown, "Data.Cooldown", "재사용 대기 시간(초).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Cost_Mana, "Data.Cost.Mana", "마나 변화량. 소모는 음수로 넘긴다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Duration, "Data.Duration", "지속 시간(초).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Multiplier_MoveSpeed, "Data.Multiplier.MoveSpeed", "이동 속도 배율. 1이면 변화 없음.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Bonus_AttackDamage, "Data.Bonus.AttackDamage", "더해 줄 공격력.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Bonus_AbilityPower, "Data.Bonus.AbilityPower", "더해 줄 주문력.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Multiplier_AttackSpeed, "Data.Multiplier.AttackSpeed", "공격 속도 배율. 1이면 변화 없음.");
}
