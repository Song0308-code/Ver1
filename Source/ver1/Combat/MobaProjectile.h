#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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
	virtual void Tick(float DeltaSeconds) override;

	/** 서버 전용. 생성 직후 호출한다. */
	void InitHoming(AActor* InTarget, const FGameplayEffectSpecHandle& InEffectSpec, float InSpeed);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(Replicated)
	TObjectPtr<AActor> Target;

	UPROPERTY(Replicated)
	float Speed = 2000.0f;

	/** 대상 중심에서 이 거리 안에 들어오면 명중으로 본다. */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float HitRadius = 30.0f;

	/** 도착 시 대상에게 적용할 이펙트. 서버에만 있다. */
	FGameplayEffectSpecHandle EffectSpec;
};
