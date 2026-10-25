#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Game/MobaTypes.h"
#include "MobaTrap.generated.h"

class AMobaCharacter;
class UStaticMeshComponent;

/**
 * 진진자라 E의 덫. 놓은 뒤 잠시 지나면 작동하고, 적 챔피언이 밟으면 터져 주변 적에게 피해·둔화·표식을 준다.
 * 상대 팀 PC로는 보내지 않아 보이지 않는다.
 */
UCLASS()
class VER1_API AMobaTrap : public AActor
{
	GENERATED_BODY()

public:
	AMobaTrap();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;
	virtual void BeginPlay() override;

	/** 서버 전용. 생성 직후 호출한다. */
	void Arm(AMobaCharacter* InOwner, float InDamage, float InArmDelay, float InLifetime);

	EMobaTeam GetTeam() const { return Team; }

	UPROPERTY(EditDefaultsOnly, Category = "Trap")
	float TriggerRadius = 150.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Trap")
	float ExplosionRadius = 260.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Trap")
	float SlowMultiplier = 0.65f;

	UPROPERTY(EditDefaultsOnly, Category = "Trap")
	float SlowDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Trap")
	float MarkDuration = 4.0f;

protected:
	UFUNCTION()
	void OnRep_Team();

	void CheckTrigger();
	void Explode();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(ReplicatedUsing = OnRep_Team)
	EMobaTeam Team = EMobaTeam::None;

	TWeakObjectPtr<AMobaCharacter> TrapOwner;
	float Damage = 0.0f;
	double ArmTime = 0.0;
	FTimerHandle CheckTimer;
};
