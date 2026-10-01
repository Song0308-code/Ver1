#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Game/MobaTypes.h"
#include "GameplayTagContainer.h"
#include "MobaCharacter.generated.h"

class AMobaPlayerState;
class UMobaChampionData;
class UMobaCombatComponent;
class UMobaGameplayAbility;
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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	UMobaCombatComponent* GetCombatComponent() const { return CombatComponent; }

	const UMobaChampionData* GetChampionData() const { return ChampionData; }

	bool IsDead() const;
	bool IsStunned() const;

	/** 죽지도 기절하지도 않아 명령을 받을 수 있는 상태인지. */
	bool CanAct() const { return !IsDead() && !IsStunned(); }

	/** 서버 전용. 체력과 마나를 채우고 지정한 위치에서 되살린다. */
	void Respawn(const FVector& Location, const FRotator& Rotation);

	/** 서버 전용. 즉시 위치를 옮기고 조종하는 클라이언트에도 반영한다. */
	void TeleportChampion(const FVector& Location, const FRotator& Rotation);

	/** 조종하는 클라이언트 또는 서버에서 호출한다. 해당 칸의 스킬을 TargetLocation을 향해 시전한다. */
	void CastAbility(EMobaAbilitySlot Slot, const FVector& TargetLocation);

	/** 해당 칸에 들어 있는 스킬의 기본 정보(CDO). 조종하는 클라이언트와 서버에서만 유효하다. */
	const UMobaGameplayAbility* GetAbilityInSlot(EMobaAbilitySlot Slot) const;

	/** 해당 칸의 남은 재사용 대기 시간과 전체 시간. 대기 중이 아니면 false. */
	bool GetCooldownRemaining(EMobaAbilitySlot Slot, float& OutRemaining, float& OutDuration) const;

protected:
	/** PlayerState의 팀 변경을 구독하고 현재 팀을 외형에 반영한다. */
	void BindToPlayerStateTeam();

	/** PlayerState의 능력 시스템에 이 캐릭터를 아바타로 연결한다. 서버와 클라이언트 양쪽에서 호출된다. */
	void InitAbilitySystem();

	void HandleTeamChanged(EMobaTeam NewTeam);
	void HandleMoveSpeedChanged(float NewMoveSpeed);
	void HandleOutOfHealth(AActor* DamageInstigator);
	void HandleDeadTagChanged(const FGameplayTag Tag, int32 NewCount);
	void HandleStunnedTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** 사망·부활에 따른 외형과 충돌을 맞춘다. */
	void ApplyDeadState(bool bDead);

	/** 사망 또는 기절 중이면 이동을 막고, 아니면 다시 걸을 수 있게 한다. */
	void UpdateMovementLock();

	/** 서버 전용. 챔피언 데이터(없으면 기본값)에 따라 스탯과 스킬을 처음 한 번 부여한다. */
	void GrantChampionKit(AMobaPlayerState* MobaPlayerState);

	UFUNCTION(Server, Reliable)
	void ServerCastAbility(EMobaAbilitySlot Slot, FVector_NetQuantize TargetLocation);

	/** 서버가 옮긴 위치를 조종하는 클라이언트에도 즉시 반영한다. */
	UFUNCTION(Client, Reliable)
	void ClientTeleport(const FVector& Location, const FRotator& Rotation);

	/** 실제 메시가 들어오기 전까지 쓰는 임시 외형. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMobaCombatComponent> CombatComponent;

	/** 이 챔피언의 정의. 서버가 스폰 직후 정한다. 비어 있으면 아래 기본값을 쓴다. */
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Champion")
	TObjectPtr<UMobaChampionData> ChampionData;

	/** 챔피언 데이터가 없을 때 쓰는 기본 스탯. */
	UPROPERTY(EditDefaultsOnly, Category = "Champion|Fallback")
	FMobaBaseStats BaseStats;

	/** 챔피언 데이터가 없을 때 쓰는 기본 스킬. */
	UPROPERTY(EditDefaultsOnly, Category = "Champion|Fallback")
	TMap<EMobaAbilitySlot, TSubclassOf<UMobaGameplayAbility>> DefaultAbilities;

	/** 이미 연결을 마친 능력 시스템. 중복 바인딩을 막는다. */
	TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystem;
};
