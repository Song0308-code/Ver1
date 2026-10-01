#include "Game/MobaGameplayTags.h"

namespace MobaTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "사망 상태. 부활할 때 제거된다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage_Physical, "Data.Damage.Physical", "방어력으로 줄어드는 물리 피해량.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage_Magic, "Data.Damage.Magic", "마법 저항력으로 줄어드는 마법 피해량.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage_True, "Data.Damage.True", "줄어들지 않는 고정 피해량.");
}
