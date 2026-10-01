#include "Game/MobaGameplayTags.h"

namespace MobaTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "사망 상태. 부활할 때 제거된다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Stunned, "State.Stunned", "기절. 이동·공격·스킬 사용이 불가능하다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Slowed, "State.Slowed", "이동 속도 감소 중.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Hasted, "State.Hasted", "이동·공격 속도 증가 중.");

	UE_DEFINE_GAMEPLAY_TAG(Ability_Slot_Q, "Ability.Slot.Q");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Slot_W, "Ability.Slot.W");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Slot_E, "Ability.Slot.E");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Slot_R, "Ability.Slot.R");

	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Slot_Q, "Cooldown.Slot.Q");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Slot_W, "Cooldown.Slot.W");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Slot_E, "Cooldown.Slot.E");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Slot_R, "Cooldown.Slot.R");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage_Physical, "Data.Damage.Physical", "방어력으로 줄어드는 물리 피해량.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage_Magic, "Data.Damage.Magic", "마법 저항력으로 줄어드는 마법 피해량.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage_True, "Data.Damage.True", "줄어들지 않는 고정 피해량.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Cooldown, "Data.Cooldown", "재사용 대기 시간(초).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Cost_Mana, "Data.Cost.Mana", "마나 변화량. 소모는 음수로 넘긴다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Duration, "Data.Duration", "지속 시간(초).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Multiplier_MoveSpeed, "Data.Multiplier.MoveSpeed", "이동 속도 배율. 1이면 변화 없음.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Multiplier_AttackSpeed, "Data.Multiplier.AttackSpeed", "공격 속도 배율. 1이면 변화 없음.");
}
