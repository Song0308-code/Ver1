#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Game/MobaTypes.h"
#include "MobaAnimationComponent.generated.h"

class UAnimationAsset;
class UAnimSequenceBase;
class USkeletalMeshComponent;

/** 애니메이션 블루프린트 없이 재생할 애니메이션 묶음. */
USTRUCT(BlueprintType)
struct FMobaAnimationSet
{
	GENERATED_BODY()

	/** 이동 속도에 따라 섞는 블렌드 스페이스, 또는 한 가지 반복 애니메이션. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TSoftObjectPtr<UAnimationAsset> Locomotion;

	/** 블렌드 스페이스의 속도 축에 넣기 전에 이동 속도에 곱하는 값. 애니메이션의 보폭과 실제 이동 속도를 맞출 때 쓴다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	float LocomotionSpeedScale = 1.0f;

	/** 기본 공격마다 돌아가며 재생한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TArray<TSoftObjectPtr<UAnimSequenceBase>> AttackAnimations;

	/** 칸별 스킬 애니메이션. 없는 칸은 기본 공격 애니메이션을 쓴다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TMap<EMobaAbilitySlot, TSoftObjectPtr<UAnimSequenceBase>> AbilityAnimations;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TSoftObjectPtr<UAnimSequenceBase> DeathAnimation;

	bool IsEmpty() const { return Locomotion.IsNull() && AttackAnimations.Num() == 0 && DeathAnimation.IsNull(); }
};

/**
 * 메시 하나를 단일 노드 모드로 돌리며 이동·공격·스킬·사망 애니메이션을 직접 고른다.
 * 애니메이션 블루프린트를 만들기 전까지 쓰는 간단한 방식이다. 챔피언 데이터에 AnimClass가 있으면 쓰지 않는다.
 * 모든 PC에서 각자 돌아가며, 공격·스킬 시점은 이미 모든 PC로 전달되는 이벤트를 듣는다.
 */
UCLASS(ClassGroup = (Moba), meta = (BlueprintSpawnableComponent))
class VER1_API UMobaAnimationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMobaAnimationComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 메시와 애니메이션 묶음을 정하고 재생을 시작한다. 묶음이 비어 있으면 아무것도 하지 않는다. */
	void Configure(USkeletalMeshComponent* InMesh, const FMobaAnimationSet& InSet);

	bool HasDeathAnimation() const { return DeathAnimation != nullptr; }

protected:
	void HandleAttackStarted(AActor* Target);
	void HandleAbilityCast(EMobaAbilitySlot Slot);

	/** 한 번만 재생하는 애니메이션. MaxDuration이 0보다 크면 그 안에 끝나도록 빠르게 재생한다. */
	void PlayOneShot(UAnimSequenceBase* Animation, float MaxDuration, bool bCancelOnMove);

	void PlayLocomotion();

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(Transient)
	TObjectPtr<UAnimationAsset> Locomotion;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAnimSequenceBase>> AttackAnimations;

	UPROPERTY(Transient)
	TMap<EMobaAbilitySlot, TObjectPtr<UAnimSequenceBase>> AbilityAnimations;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> DeathAnimation;

	float LocomotionSpeedScale = 1.0f;

	/** 블렌드 스페이스에서 이동 속도를 넣을 축(0 = X, 1 = Y). 이름이 Speed인 축을 찾고, 없으면 X. */
	int32 SpeedAxis = 0;

	int32 NextAttackIndex = 0;
	double OneShotEndTime = 0.0;
	bool bPlayingOneShot = false;
	bool bOneShotCancelsOnMove = false;
	bool bPlayingDeath = false;

	FDelegateHandle AttackStartedHandle;
	FDelegateHandle AbilityCastHandle;
};
