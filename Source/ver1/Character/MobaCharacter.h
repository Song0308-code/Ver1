#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Game/MobaTypes.h"
#include "GameplayTagContainer.h"
#include "MobaCharacter.generated.h"

class UMobaCombatComponent;
class UStaticMeshComponent;

/** 플레이어가 조종하는 챔피언의 기본 클래스. */
UCLASS()
class VER1_API AMobaCharacter : public ACharacter, public IMobaTeamInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMobaCharacter();

	virtual EMobaTeam GetTeam() const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	UMobaCombatComponent* GetCombatComponent() const { return CombatComponent; }

	bool IsDead() const;

	/** 서버 전용. 체력과 마나를 채우고 지정한 위치에서 되살린다. */
	void Respawn(const FVector& Location, const FRotator& Rotation);

protected:
	/** PlayerState의 팀 변경을 구독하고 현재 팀을 외형에 반영한다. */
	void BindToPlayerStateTeam();

	/** PlayerState의 능력 시스템에 이 캐릭터를 아바타로 연결한다. 서버와 클라이언트 양쪽에서 호출된다. */
	void InitAbilitySystem();

	void HandleTeamChanged(EMobaTeam NewTeam);
	void HandleMoveSpeedChanged(float NewMoveSpeed);
	void HandleOutOfHealth(AActor* DamageInstigator);
	void HandleDeadTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** 사망·부활에 따른 외형, 충돌, 이동 상태를 맞춘다. */
	void ApplyDeadState(bool bDead);

	/** 서버가 옮긴 위치를 조종하는 클라이언트에도 즉시 반영한다. */
	UFUNCTION(Client, Reliable)
	void ClientTeleport(const FVector& Location, const FRotator& Rotation);

	/** 실제 메시가 들어오기 전까지 쓰는 임시 외형. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMobaCombatComponent> CombatComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	FMobaBaseStats BaseStats;

	/** 이미 연결을 마친 능력 시스템. 중복 바인딩을 막는다. */
	TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystem;
};
