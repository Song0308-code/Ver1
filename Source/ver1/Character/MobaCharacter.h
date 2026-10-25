#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Character/MobaPassive.h"
#include "Game/MobaTypes.h"
#include "GameplayTagContainer.h"
#include "MobaCharacter.generated.h"

class AMobaPlayerState;
class AMobaWard;
class UMobaAnimationComponent;
class UMobaPassive;
class UMobaChampionData;
struct FMobaItemDefinition;
class UMobaCombatComponent;
class UMobaGameplayAbility;
class UStaticMeshComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMobaAbilityCast, EMobaAbilitySlot /*Slot*/);

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
	virtual void BeginPlay() override;

	/** 상대 팀이 볼 수 없으면 그 팀의 PC로 보내지 않는다. */
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	UMobaCombatComponent* GetCombatComponent() const { return CombatComponent; }

	const UMobaChampionData* GetChampionData() const { return ChampionData; }

	bool IsDead() const;
	bool IsStunned() const;

	/** 죽지도 기절하지도 않아 명령을 받을 수 있는 상태인지. */
	bool CanAct() const { return !IsDead() && !IsStunned(); }

	/** 속박 중인지. 속박은 이동만 막는다. */
	bool IsRooted() const;

	/** 지금 움직일 수 있는지(행동 가능하고 속박되지 않음). */
	bool CanMove() const { return CanAct() && !IsRooted() && !IsDisplaced(); }

	/** 끌려가거나 밀려나거나 돌진하는 중인지. */
	bool IsDisplaced() const;

	/** 서버 전용. 체력과 마나를 채우고 지정한 위치에서 되살린다. */
	void Respawn(const FVector& Location, const FRotator& Rotation);

	/** 서버 전용. 즉시 위치를 옮기고 조종하는 클라이언트에도 반영한다. */
	void TeleportChampion(const FVector& Location, const FRotator& Rotation);

	/** 조종하는 클라이언트 또는 서버에서 호출한다. 해당 칸의 스킬을 TargetLocation을 향해 시전한다. */
	void CastAbility(EMobaAbilitySlot Slot, const FVector& TargetLocation);

	/** 해당 칸에 들어 있는 스킬의 기본 정보(CDO). 조종하는 클라이언트와 서버에서만 유효하다. */
	const UMobaGameplayAbility* GetAbilityInSlot(EMobaAbilitySlot Slot) const;

	/** 해당 칸 스킬의 현재 단계. 아직 찍지 않았으면 0. */
	int32 GetAbilityLevel(EMobaAbilitySlot Slot) const;

	/** 지금 스킬 포인트를 써서 해당 칸을 한 단계 올릴 수 있는지. */
	bool CanLevelUpAbility(EMobaAbilitySlot Slot) const;

	/** 조종하는 클라이언트 또는 서버에서 호출한다. 스킬 포인트를 써서 해당 칸을 한 단계 올린다. */
	void LevelUpAbility(EMobaAbilitySlot Slot);

	/** 해당 칸의 남은 재사용 대기 시간과 전체 시간. 대기 중이 아니면 false. */
	bool GetCooldownRemaining(EMobaAbilitySlot Slot, float& OutRemaining, float& OutDuration) const;

	/** 조종하는 클라이언트 또는 서버에서 호출한다. 체력 물약을 하나 마신다. 이미 마시는 중이면 회복이 이어진다. */
	void UsePotion();

	/** 물약으로 아직 회복할 양. 0보다 크면 마시는 중이다. */
	float GetPotionHealRemaining() const { return PotionHealRemaining; }

	/** 조종하는 클라이언트 또는 서버에서 호출한다. 전령의 눈을 써서 가까운 라인에 아군 전령을 소환한다. */
	void UseHeraldEye();

	/** 조종하는 클라이언트 또는 서버에서 호출한다. 제어 와드를 놓는다. 이미 놓은 것이 있으면 그것은 사라진다. */
	void PlaceControlWard(const FVector& Location);

	/** 조종하는 클라이언트 또는 서버에서 호출한다. 지정한 위치(멀면 놓을 수 있는 거리까지)에 와드를 놓는다. */
	void PlaceWard(const FVector& Location);

	/** 와드를 다시 놓을 수 있을 때까지 남은 시간(초). */
	float GetWardCooldownRemaining() const;

	float GetWardPlaceRange() const { return WardPlaceRange; }

	/** 서버 전용. 은신 중이면 푼다. 기본 공격을 쏘거나 스킬을 쓸 때 부른다. */
	void BreakStealth();

	/** 서버 전용. Duration초 동안 Amount만큼 피해를 먼저 흡수하는 보호막을 더한다. 여러 개면 먼저 사라질 것부터 깎인다. */
	void AddShield(float Amount, float Duration);

	/** 서버 전용. 기본 공격을 쏘기 직전에 패시브와 강화된 공격(SetEmpoweredAttack)이 더할 것을 받는다. */
	FMobaAttackBonus ApplyPassiveOnBasicAttack(AActor* Target);

	/** 서버 전용. Duration초 안에 쏘는 다음 기본 공격 한 번에 Bonus를 더한다. */
	void SetEmpoweredAttack(const FMobaAttackBonus& Bonus, float Duration);

	/** 서버 전용. 스킬이 Target에게 군중 제어를 걸었을 때 부른다. 적 챔피언이면 패시브에 알린다. */
	void NotifyCrowdControlApplied(AActor* Target);

	/** 서버 전용. 스킬이 적 Target에게 맞았을 때 부른다. 패시브에 알린다. */
	void NotifyAbilityHit(AActor* Target);

	/** 챔피언 데이터의 레벨 1 능력치. */
	const FMobaBaseStats& GetBaseStats() const;

	/** 서버 전용. 패시브가 지금 기본 공격을 막는지(재장전 등). */
	bool CanStartBasicAttack() const;

	/** 서버 전용. 기본 공격 주기에 쓸 공격 속도(패시브가 바꿀 수 있다). */
	float GetEffectiveAttackSpeed(float AttackSpeed) const;

	/** 이 챔피언의 패시브 정의(클래스 기본값). 이름·설명을 화면에 보여 줄 때 쓴다. 없으면 nullptr. */
	const UMobaPassive* GetPassiveDefaults() const;

	/** 패시브 중첩(또는 켜짐 여부). 모든 PC로 복제된다. */
	int32 GetPassiveStacks() const { return PassiveStacks; }

	/** 서버 전용. 패시브가 호출한다. */
	void SetPassiveStacks(int32 Stacks) { PassiveStacks = Stacks; }

	/** 스킬이 발동될 때 모든 PC에서 호출된다. 애니메이션·사운드 연결용. */
	FOnMobaAbilityCast OnAbilityCast;

	/** 서버 전용. 스킬이 마나와 쿨다운을 소모하고 실제로 발동됐을 때 스킬이 호출한다. */
	void NotifyAbilityCast(EMobaAbilitySlot Slot);

	/** 자기 팀 우물 안에 있는지. */
	bool IsInFountain() const;

	/** 지금 상점을 쓸 수 있는지. 우물 안이거나 죽어 있을 때만 된다. */
	bool CanShop() const { return IsDead() || IsInFountain(); }

	bool CanBuyItem(int32 ItemIndex) const;

	/** 지금 가방 기준으로 이 아이템을 사는 데 드는 골드(가진 재료만큼 깎인 값). */
	int32 GetItemPurchaseCost(int32 ItemIndex) const;

	/** 조종하는 클라이언트 또는 서버에서 호출한다. */
	void BuyItem(int32 ItemIndex);

	/** 가방의 Slot번째 아이템을 판다. */
	void SellItem(int32 Slot);

	/** 조종하는 클라이언트 또는 서버에서 호출한다. 제자리에서 귀환을 준비하고, 끝나면 우물로 돌아간다. */
	void StartRecall();

	bool IsRecalling() const;

	/** 귀환까지 남은 시간(초). 귀환 중이 아니면 0. */
	float GetRecallRemaining() const;

	float GetRecallDuration() const { return RecallDuration; }

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

	UFUNCTION(Server, Reliable)
	void ServerLevelUpAbility(EMobaAbilitySlot Slot);

	UFUNCTION(Server, Reliable)
	void ServerPlaceWard(FVector_NetQuantize Location);

	UFUNCTION(Server, Reliable)
	void ServerUsePotion();

	UFUNCTION(Server, Reliable)
	void ServerPlaceControlWard(FVector_NetQuantize Location);

	UFUNCTION(Server, Reliable)
	void ServerUseHeraldEye();

	/** 놓을 위치를 놓을 수 있는 거리 안의 발밑 높이로 맞춘다. */
	FVector ClampWardLocation(const FVector& Location) const;

	/** 서버 전용. 내가 놓은 제어 와드(한 번에 하나). */
	TWeakObjectPtr<AMobaWard> ActiveControlWard;

	/** 물약으로 남은 회복량과 초당 회복량. 남은 양은 화면 표시용으로 복제된다. */
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Items")
	float PotionHealRemaining = 0.0f;

	float PotionHealPerSecond = 0.0f;

	UFUNCTION(Server, Reliable)
	void ServerBuyItem(int32 ItemIndex);

	UFUNCTION(Server, Reliable)
	void ServerSellItem(int32 Slot);

	UFUNCTION(Server, Reliable)
	void ServerStartRecall();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAbilityCast(EMobaAbilitySlot Slot);

	/** 조종하는 클라이언트에서, 서버에 보내기 전에 이 시전이 받아들여질 만한지 미리 본다(단계, 쿨다운, 마나). */
	bool CanCastLocally(EMobaAbilitySlot Slot) const;

	/** 시전 동작과 이펙트를 이 PC에서 재생한다. */
	void PlayAbilityCastLocally(EMobaAbilitySlot Slot);

	/** 조종하는 클라이언트가 미리 재생해 둔 시전. 서버의 알림이 오면 다시 재생하지 않는다. 칸별 재생 시각(0이면 없음). */
	double PredictedCastTimes[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

	UFUNCTION()
	void OnRep_ChampionData();

	/** 챔피언 데이터의 메시·애니메이션을 입힌다. 메시가 없으면 임시 원기둥의 크기만 맞춘다. */
	void ApplyChampionAppearance();

	/** 서버 전용. 아이템의 수치를 더하거나(Sign = 1) 뺀다(Sign = -1). */
	void ApplyItemStats(const FMobaItemDefinition& Item, float Sign);

	/** 서버 전용. */
	void TickRecall();
	void CancelRecall();
	void FinishRecall();
	void HandleDamageDealt(AActor* DamageInstigator, AActor* Victim, float Damage);

	/** 서버 전용. 레벨이 올랐을 때 성장 스탯을 더한다. */
	void HandleLevelUp(int32 NewLevel);

	/** 서버 전용. 기본 재생과 우물 회복을 적용한다. */
	void TickRegen();

	const FMobaStatGrowth& GetStatGrowth() const;

	/** 서버가 옮긴 위치를 조종하는 클라이언트에도 즉시 반영한다. */
	UFUNCTION(Client, Reliable)
	void ClientTeleport(const FVector& Location, const FRotator& Rotation);

	/** 실제 메시가 들어오기 전까지 쓰는 임시 외형. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMobaCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMobaAnimationComponent> AnimationComponent;

	/** 서버 전용. 남아 있는 보호막들. 합이 능력치 Shield와 같도록 UpdateShields가 맞춘다. */
	struct FShieldEntry
	{
		float Amount = 0.0f;
		double ExpireTime = 0.0;
	};
	TArray<FShieldEntry> Shields;

	/** 받은 피해만큼 줄어든 Shield 능력치를 목록에 반영하고, 시간이 지난 보호막을 없앤다. */
	void UpdateShields();

	/** 서버에서만 만드는 패시브 인스턴스. */
	UPROPERTY(Transient)
	TObjectPtr<UMobaPassive> PassiveInstance;

	/** 서버 전용. 다음 기본 공격 강화와 그 만료 시각(월드 시간). */
	FMobaAttackBonus EmpoweredAttack;
	double EmpoweredAttackExpireTime = 0.0;

	UPROPERTY(VisibleInstanceOnly, Replicated, Category = "Passive")
	int32 PassiveStacks = 0;

	/** 메시가 있을 때 사망 애니메이션을 보여 준 뒤 숨기기까지의 시간(초). */
	UPROPERTY(EditDefaultsOnly, Category = "Appearance")
	float DeathHideDelay = 2.5f;

	/** 실제 메시를 쓰는지. 임시 원기둥은 발밑의 팀 색 원판으로 바뀐다. */
	bool bUsingSkeletalMesh = false;

	FTimerHandle DeathHideTimer;

	/** 이 챔피언의 정의. 서버가 스폰 직후 정한다. 비어 있으면 아래 기본값을 쓴다. */
	UPROPERTY(ReplicatedUsing = OnRep_ChampionData, EditDefaultsOnly, BlueprintReadOnly, Category = "Champion")
	TObjectPtr<UMobaChampionData> ChampionData;

	/** 챔피언 데이터가 없을 때 쓰는 기본 스탯. */
	UPROPERTY(EditDefaultsOnly, Category = "Champion|Fallback")
	FMobaBaseStats BaseStats;

	/** 챔피언 데이터가 없을 때 쓰는 레벨당 성장 스탯. */
	UPROPERTY(EditDefaultsOnly, Category = "Champion|Fallback")
	FMobaStatGrowth StatGrowth;

	/** 챔피언 데이터가 없을 때 쓰는 기본 스킬. */
	UPROPERTY(EditDefaultsOnly, Category = "Champion|Fallback")
	TMap<EMobaAbilitySlot, TSubclassOf<UMobaGameplayAbility>> DefaultAbilities;

	/** 처음 스폰된 자기 팀 우물의 위치. 이 근처에서는 체력과 마나가 빠르게 차고 상점을 쓸 수 있다. */
	UPROPERTY(Replicated)
	FVector HomeLocation = FVector::ZeroVector;

	/** 귀환을 준비하는 시간(초). */
	UPROPERTY(EditDefaultsOnly, Category = "Recall")
	float RecallDuration = 8.0f;

	/** 귀환이 끝나는 시각(서버 월드 시간). 귀환 중이 아니면 0. */
	UPROPERTY(Replicated)
	float RecallEndServerTime = 0.0f;

	/** 귀환을 시작한 시각(서버의 월드 시간). 직후의 잔여 속도로 취소되지 않게 하는 데 쓴다. */
	double RecallStartTime = 0.0;

	FTimerHandle RecallTimer;
	FTimerHandle RecallCheckTimer;
	FDelegateHandle DamageDealtHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Fountain")
	float FountainRadius = 900.0f;

	/** 우물 안에서 1초에 회복하는 최대 체력·마나의 비율. */
	UPROPERTY(EditDefaultsOnly, Category = "Fountain")
	float FountainHealFractionPerSecond = 0.085f;

	/** 블루 버프가 더해 주는 초당 마나 재생. */
	UPROPERTY(EditDefaultsOnly, Category = "Buffs")
	float BlueBuffManaRegenPerSecond = 6.0f;

	/** 레드 버프가 1초에 회복시키는 최대 체력의 비율. */
	UPROPERTY(EditDefaultsOnly, Category = "Buffs")
	float RedBuffHealFractionPerSecond = 0.005f;

	UPROPERTY(EditDefaultsOnly, Category = "Ward")
	float WardPlaceRange = 600.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Ward")
	float WardDuration = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Ward")
	float WardCooldown = 30.0f;

	/** 한 챔피언이 동시에 유지할 수 있는 와드 수. 넘치면 가장 오래된 것이 사라진다. */
	UPROPERTY(EditDefaultsOnly, Category = "Ward")
	int32 MaxActiveWards = 3;

	/** 서버에서 관리하는, 이 챔피언이 놓은 와드. */
	TArray<TWeakObjectPtr<AMobaWard>> ActiveWards;

	FTimerHandle RegenTimer;

	/** 이미 연결을 마친 능력 시스템. 중복 바인딩을 막는다. */
	TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystem;
};
