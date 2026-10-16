#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MobaAreaIndicator.generated.h"

class UStaticMeshComponent;

/** 곧 효과가 터질 범위를 바닥에 보여 주는 임시 표시. 모든 플레이어에게 보인다. */
UCLASS()
class VER1_API AMobaAreaIndicator : public AActor
{
	GENERATED_BODY()

public:
	AMobaAreaIndicator();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 서버 전용. 반지름을 정하고 Duration초 뒤에 사라지게 한다. */
	void Show(float InRadius, float Duration);

protected:
	UFUNCTION()
	void OnRep_Radius();

	void ApplyRadiusToMesh();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(ReplicatedUsing = OnRep_Radius)
	float Radius = 100.0f;
};
