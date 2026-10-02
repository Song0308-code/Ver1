#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Game/MobaTypes.h"
#include "GameplayEffectTypes.h"
#include "MobaProjectile.generated.h"

class UStaticMeshComponent;

/** 대상을 끝까지 따라가 맞히는 유도 투사체. 서버에서 생성하고 도착 시 서버가 이펙트를 적용한다. */
UCLASS()
class VER1_API AMobaProjectile : public AActor
{
	GENERATED_BODY()

public:
	AMobaProjectile();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 서버 전용. 생성 직후 호출한다. */
	void InitHoming(AActor* InTarget, const FGameplayEffectSpecHandle& InEffectSpec, float InSpeed, float InVisualScale, EMobaTeam InSourceTeam);

	/** 서버 전용. 명중했을 때 피해와 함께 적용할 이펙트(둔화 등)를 더한다. */
	void AddOnHitEffect(const FGameplayEffectSpecHandle& InEffectSpec) { ExtraEffectSpecs.Add(InEffectSpec); }

protected:
	UFUNCTION()
	void OnRep_Visual();

	/** 크기와 쏜 팀의 색을 외형에 반영한다. */
	void ApplyVisual();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(Replicated)
	TObjectPtr<AActor> Target;

	UPROPERTY(Replicated)
	float Speed = 1600.0f;

	UPROPERTY(ReplicatedUsing = OnRep_Visual)
	float VisualScale = 0.4f;

	UPROPERTY(ReplicatedUsing = OnRep_Visual)
	EMobaTeam SourceTeam = EMobaTeam::None;

	/** 대상의 조준점에서 이 거리 안에 들어오면 명중으로 본다. */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float HitRadius = 30.0f;

	/** 도착 시 대상에게 적용할 이펙트. 서버에만 있다. */
	FGameplayEffectSpecHandle EffectSpec;

	TArray<FGameplayEffectSpecHandle> ExtraEffectSpecs;
};
