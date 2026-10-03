#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Game/MobaTypes.h"
#include "MobaAnimInstance.generated.h"

/**
 * 챔피언·미니언·몬스터의 애니메이션 블루프린트가 부모로 삼는 클래스.
 * 이동 속도와 상태를 매 프레임 채워 주고, 기본 공격과 스킬 시전 시점을 이벤트로 알려 준다.
 * 스켈레탈 메시와 애니메이션 에셋이 들어오면 이 클래스를 부모로 애니메이션 블루프린트를 만들어 챔피언 데이터의 AnimClass에 지정한다.
 */
UCLASS()
class VER1_API UMobaAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	/** 기본 공격의 선딜레이가 시작될 때. 공격 몽타주를 재생하면 된다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Moba")
	void OnBasicAttack(AActor* Target);

	/** 스킬이 발동됐을 때. 칸에 맞는 몽타주를 재생하면 된다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Moba")
	void OnAbilityCast(EMobaAbilitySlot Slot);

	void HandleAttackStarted(AActor* Target);
	void HandleAbilityCast(EMobaAbilitySlot Slot);

	/** 수평 이동 속도. */
	UPROPERTY(BlueprintReadOnly, Category = "Moba")
	float Speed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Moba")
	bool bIsMoving = false;

	UPROPERTY(BlueprintReadOnly, Category = "Moba")
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, Category = "Moba")
	bool bIsStunned = false;

	UPROPERTY(BlueprintReadOnly, Category = "Moba")
	bool bIsRecalling = false;

	/** 지금까지 한 기본 공격 횟수. 이벤트 대신 값의 변화로 전이를 걸 때 쓴다. */
	UPROPERTY(BlueprintReadOnly, Category = "Moba")
	int32 AttackCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Moba")
	int32 AbilityCastCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Moba")
	EMobaAbilitySlot LastAbilitySlot = EMobaAbilitySlot::Q;

	FDelegateHandle AttackStartedHandle;
	FDelegateHandle AbilityCastHandle;
};
