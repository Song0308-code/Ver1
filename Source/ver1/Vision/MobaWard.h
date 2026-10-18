#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "Game/MobaTypes.h"
#include "MobaWard.generated.h"

class UAbilitySystemComponent;
class UMobaAttributeSet;
class UStaticMeshComponent;

/**
 * 주변 시야를 밝혀 주는 와드.
 * 일반 와드는 정해진 시간 동안 남고 적에게 보이지 않는다. 다만 적의 제어 와드 근처에서는 드러나고 시야가 꺼진다.
 * 제어 와드는 시간 제한이 없고, 근처의 적 일반 와드를 드러내며, 자신은 시야 안의 적에게 보인다.
 * 둘 다 클릭해서 기본 공격으로만 부술 수 있다(한 번에 1씩, 미니언·포탑은 노리지 않는다).
 */
UCLASS()
class VER1_API AMobaWard : public AActor, public IMobaTeamInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMobaWard();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	/** 상대 팀이 볼 수 없으면 그 팀의 PC로 보내지 않는다. */
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;

	virtual EMobaTeam GetTeam() const override { return Team; }
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	bool IsControlWard() const { return bControlWard; }

	/** 제어 와드가 적 일반 와드를 드러내고 끄는 거리. */
	static constexpr float RevealRadius = 900.0f;

	/** 서버 전용. 해당 팀의 와드를 놓는다. Duration이 0보다 크면 그 시간 뒤에 사라진다. Owner는 부서졌을 때 알림에 쓴다. */
	static AMobaWard* SpawnWard(UWorld* World, EMobaTeam InTeam, const FVector& Location, float Duration, bool bInControlWard = false);

protected:
	UFUNCTION()
	void OnRep_Team();

	void ApplyTeamColor();
	void HandleOutOfHealth(AActor* DamageInstigator);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UMobaAttributeSet> AttributeSet;

	UPROPERTY(ReplicatedUsing = OnRep_Team)
	EMobaTeam Team = EMobaTeam::None;

	UPROPERTY(ReplicatedUsing = OnRep_Team, VisibleInstanceOnly, Category = "Ward")
	bool bControlWard = false;

	/** 부술 때 필요한 기본 공격 횟수. */
	static constexpr int32 WardHits = 3;
	static constexpr int32 ControlWardHits = 4;

	/** 부순 챔피언이 받는 골드. */
	static constexpr float KillGold = 30.0f;
};
