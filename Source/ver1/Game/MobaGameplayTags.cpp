#include "Game/MobaGameplayTags.h"

namespace MobaTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "사망 상태. 부활할 때 제거된다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Stunned, "State.Stunned", "기절. 이동·공격·스킬 사용이 불가능하다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Slowed, "State.Slowed", "이동 속도 감소 중.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Hasted, "State.Hasted", "이동·공격 속도 증가 중.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Rooted, "State.Rooted", "속박. 이동과 이동 스킬만 막힌다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Silenced, "State.Silenced", "침묵. 스킬만 막힌다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Stealthed, "State.Stealthed", "은신. 적에게 보이지 않는다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Airborne, "State.Airborne", "에어본. 기절과 같이 모든 행동이 막힌다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_BasicAttack, "Damage.BasicAttack", "기본 공격으로 준 피해.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invulnerable, "State.Invulnerable", "무적. 공격 대상이 되지 않는다. 앞선 건물이 남아 있는 건물에 붙는다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Recalling, "State.Recalling", "귀환을 준비하는 중. 움직이거나 맞으면 취소된다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Displaced, "State.Displaced", "강제 이동 중. 스스로 걷지 못한다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Marked, "State.Marked", "노머고의 표식. 다음 공격에 터진다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Buff_Blue, "Buff.Blue", "푸른 파수꾼 버프. 마나 재생이 늘고 재사용 대기 시간이 줄어든다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Buff_Red, "Buff.Red", "붉은 덩굴정령 버프. 체력 재생이 늘고 기본 공격이 둔화와 추가 피해를 준다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Buff_Dragon, "Buff.Dragon", "드래곤 처치 보상. 경기 끝까지 유지되고 잡을 때마다 겹친다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Buff_Dragon_Infernal, "Buff.Dragon.Infernal", "화염 드래곤: 공격력·주문력.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Buff_Dragon_Mountain, "Buff.Dragon.Mountain", "대지 드래곤: 방어력·마법 저항력.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Buff_Dragon_Cloud, "Buff.Dragon.Cloud", "바람 드래곤: 이동 속도.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Buff_Dragon_Ocean, "Buff.Dragon.Ocean", "바다 드래곤: 체력 재생.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Buff_Soul, "Buff.Soul", "드래곤의 영혼(드래곤 4마리).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Buff_Soul_Ocean, "Buff.Soul.Ocean", "바다의 영혼: 큰 체력 재생.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Buff_Elder, "Buff.Elder", "장로 드래곤: 체력 20% 아래의 적 챔피언을 바로 처치.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Buff_Baron, "Buff.Baron", "내셔 남작 처치 보상.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Buff_Unbreakable, "Buff.Unbreakable", "받는 피해 감소(젖소 불굴).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Buff_Overclock, "Buff.Overclock", "노머고 패시브 공격 속도 중첩.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Buff_Conversion, "Buff.Conversion", "진진자라 패시브: 공격 속도를 공격력으로.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Buff_BaronAura, "Buff.BaronAura", "바론 버프 챔피언 근처의 강화된 미니언.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unit_Ward, "Unit.Ward", "와드. 기본 공격 한 번에 체력 1씩만 깎인다.");

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
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Bonus_Armor, "Data.Bonus.Armor", "더해 줄 방어력.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Bonus_MagicResist, "Data.Bonus.MagicResist", "더해 줄 마법 저항력.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Bonus_MoveSpeed, "Data.Bonus.MoveSpeed", "더해 줄 이동 속도.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Multiplier_AttackSpeed, "Data.Multiplier.AttackSpeed", "공격 속도 배율. 1이면 변화 없음.");
}
