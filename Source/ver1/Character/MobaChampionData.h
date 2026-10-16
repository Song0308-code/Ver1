#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "Character/MobaAnimationComponent.h"
#include "Engine/DataAsset.h"
#include "Game/MobaTypes.h"
#include "MobaChampionData.generated.h"

class UAnimInstance;
class UMaterialInterface;
class UMobaPassive;
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

	/** 항상 켜져 있는 고유 능력. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TSubclassOf<UMobaPassive> Passive;

	/** 비어 있으면 임시 원기둥 외형을 쓴다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Appearance")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	/** MobaAnimInstance를 부모로 만든 애니메이션 블루프린트를 지정한다. 비어 있으면 아래 Animations로 직접 재생한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Appearance")
	TSoftClassPtr<UAnimInstance> AnimClass;

	/** 애니메이션 블루프린트 없이 재생할 애니메이션. AnimClass가 지정돼 있으면 쓰지 않는다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Appearance")
	FMobaAnimationSet Animations;

	/** 캡슐 중심을 기준으로 한 메시의 위치·회전·크기. 보통 발이 바닥에 닿도록 Z를 캡슐 절반 높이만큼 내리고 요를 -90으로 둔다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Appearance")
	FVector MeshOffset = FVector(0.0, 0.0, -88.0);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Appearance")
	FRotator MeshRotation = FRotator(0.0, -90.0, 0.0);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Appearance")
	float MeshScale = 1.0f;

	/** 지정하면 메시의 모든 머티리얼을 이것으로 바꾼다. 같은 메시를 쓰는 챔피언을 색으로 구분할 때 쓴다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Appearance")
	TSoftObjectPtr<UMaterialInterface> MeshMaterial;

	/** 메시가 없을 때 쓰는 임시 원기둥의 굵기·높이 배율. 챔피언을 눈으로 구분하기 위한 것이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Appearance")
	FVector PlaceholderScale = FVector(1.0, 1.0, 1.0);
};
