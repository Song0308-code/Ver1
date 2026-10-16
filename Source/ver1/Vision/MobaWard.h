#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Game/MobaTypes.h"
#include "MobaWard.generated.h"

class UStaticMeshComponent;

/** 일정 시간 동안 주변 시야를 밝혀 주는 와드. 적에게는 보이지 않고 공격할 수도 없다. */
UCLASS()
class VER1_API AMobaWard : public AActor, public IMobaTeamInterface
{
	GENERATED_BODY()

public:
	AMobaWard();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	/** 상대 팀이 볼 수 없으면 그 팀의 PC로 보내지 않는다. */
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;

	virtual EMobaTeam GetTeam() const override { return Team; }

	/** 서버 전용. 해당 팀의 와드를 놓는다. Duration초 뒤에 사라진다. */
	static AMobaWard* SpawnWard(UWorld* World, EMobaTeam InTeam, const FVector& Location, float Duration);

protected:
	UFUNCTION()
	void OnRep_Team();

	void ApplyTeamColor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(ReplicatedUsing = OnRep_Team)
	EMobaTeam Team = EMobaTeam::None;
};
