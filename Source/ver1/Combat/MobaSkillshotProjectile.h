#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Game/MobaTypes.h"
#include "GameplayEffectTypes.h"
#include "MobaSkillshotProjectile.generated.h"

class UStaticMeshComponent;

/**
 * 직선으로 날아가 처음 닿은 적에게 이펙트를 적용하는 투사체. 벽은 통과한다.
 * 서버가 판정하고, 클라이언트는 같은 방향·속도로 직접 움직여 보여 준다.
 */
UCLASS()
class VER1_API AMobaSkillshotProjectile : public AActor
{
	GENERATED_BODY()

public:
	AMobaSkillshotProjectile();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 서버 전용. 생성 직후 호출한다. */
	void Launch(AActor* InSource, const FVector& InDirection, float InSpeed, float InMaxRange, float InRadius, const TArray<FGameplayEffectSpecHandle>& InHitEffects);

protected:
	UFUNCTION()
	void OnRep_Visual();

	/** 반지름과 쏜 팀의 색을 외형에 반영한다. */
	void ApplyVisual();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(Replicated)
	FVector_NetQuantizeNormal Direction = FVector::ForwardVector;

	UPROPERTY(Replicated)
	float Speed = 0.0f;

	UPROPERTY(Replicated)
	float MaxRange = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_Visual)
	float Radius = 50.0f;

	UPROPERTY(ReplicatedUsing = OnRep_Visual)
	EMobaTeam SourceTeam = EMobaTeam::None;

	/** 이 투사체를 쏜 액터. 적 판정의 기준이 된다. 서버에만 있다. */
	UPROPERTY()
	TObjectPtr<AActor> Source;

	/** 명중 시 순서대로 적용할 이펙트. 서버에만 있다. */
	TArray<FGameplayEffectSpecHandle> HitEffects;

	float DistanceTravelled = 0.0f;
};
