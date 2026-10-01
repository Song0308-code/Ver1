#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "Engine/DataAsset.h"
#include "Game/MobaTypes.h"
#include "MobaChampionData.generated.h"

class UAnimInstance;
class UMobaGameplayAbility;
class USkeletalMesh;

/** 챔피언 한 명의 정의. 스탯, 기본 공격 방식, 스킬 네 개, 외형을 담는다. */
UCLASS(BlueprintType)
class VER1_API UMobaChampionData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Champion")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	FMobaBaseStats BaseStats;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	FMobaStatGrowth StatGrowth;

	/** true면 기본 공격이 투사체, false면 근접. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Basic Attack")
	bool bRangedBasicAttack = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Basic Attack", meta = (EditCondition = "bRangedBasicAttack"))
	float BasicAttackProjectileSpeed = 1500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TMap<EMobaAbilitySlot, TSubclassOf<UMobaGameplayAbility>> Abilities;

	/** 비어 있으면 임시 원기둥 외형을 쓴다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Appearance")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Appearance")
	TSoftClassPtr<UAnimInstance> AnimClass;
};
