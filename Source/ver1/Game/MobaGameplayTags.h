#pragma once

#include "NativeGameplayTags.h"

namespace MobaTags
{
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);

	/** 데미지 이펙트에 SetByCaller로 넘기는 피해량. */
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage_Physical);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage_Magic);
	VER1_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage_True);
}
