#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "Character/MobaAnimationComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Game/MobaTypes.h"
#include "MobaMinion.generated.h"

class AMobaLanePath;
class UMobaCombatComponent;
class USkeletalMesh;
class UStaticMeshComponent;

/** 미니언 한 종류의 수치. */
USTRUCT(BlueprintType)
struct FMobaMinionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Minion")
	FMobaBaseStats Stats;

	UPROPERTY(EditAnywhere, Category = "Minion")
	bool bRanged = false;

	UPROPERTY(EditAnywhere, Category = "Minion")
	float ProjectileSpeed = 1200.0f;

	/** 막타를 친 챔피언이 받는 골드. */
	UPROPERTY(EditAnywhere, Category = "Minion")
	float GoldReward = 20.0f;

	/** 근처의 적 챔피언들이 나눠 받는 경험치. */
	UPROPERTY(EditAnywhere, Category = "Minion")
	float ExperienceReward = 60.0f;

	/** 비어 있으면 임시 상자 외형을 쓴다. */
	UPROPERTY(EditAnywhere, Category = "Appearance")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditAnywhere, Category = "Appearance")
	float MeshScale = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Appearance")
	FMobaAnimationSet Animations;
};

/** 라인을 따라 걸으며 만나는 적을 공격하는 미니언. 서버의 AI 컨트롤러가 조종한다. */
UCLASS()
class VER1_API AMobaMinion : public ACharacter, public IMobaTeamInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMobaMinion();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual EMobaTeam GetTeam() const override { return Team; }
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }

	/** 서버 전용. 스폰을 마치기 전에 호출한다. */
	void InitMinion(EMobaTeam InTeam, EMobaMinionType InType, AMobaLanePath* InLanePath, const FMobaMinionConfig& InConfig);

	/** 서버 전용. 경기가 끝났을 때 움직임과 공격을 멈춘다. */
	void StopActing();

	EMobaMinionType GetMinionType() const { return MinionType; }
	const FMobaMinionConfig& GetConfig() const { return Config; }

protected:
	UFUNCTION()
	void OnRep_Appearance();

	void ApplyAppearance();

	/** 대상을 고르거나, 없으면 라인을 따라 전진한다. */
	void UpdateAI();

	AActor* FindBestTarget() const;
	void AdvanceAlongLane();

	void HandleOutOfHealth(AActor* DamageInstigator);
	void HandleDamageDealt(AActor* DamageInstigator, AActor* Victim, float Damage);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMobaCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMobaAnimationComponent> AnimationComponent;

	/** 메시가 있을 때 죽고 나서 사라지기까지의 시간(초). 쓰러지는 동작을 보여 준다. */
	UPROPERTY(EditDefaultsOnly, Category = "Appearance")
	float DeathLingerTime = 1.3f;

	bool bUsingSkeletalMesh = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UMobaAttributeSet> AttributeSet;

	UPROPERTY(ReplicatedUsing = OnRep_Appearance)
	EMobaTeam Team = EMobaTeam::None;

	UPROPERTY(ReplicatedUsing = OnRep_Appearance)
	EMobaMinionType MinionType = EMobaMinionType::Melee;

	UPROPERTY()
	TObjectPtr<AMobaLanePath> LanePath;

	FMobaMinionConfig Config;

	/** 이 거리 안의 적을 새 대상으로 삼는다. */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AcquisitionRange = 700.0f;

	/** 대상이 이 거리보다 멀어지면 포기한다. */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float LeashRange = 1000.0f;

	/** 라인 위에서 한 번에 내다보는 거리. */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float LaneLookAhead = 800.0f;

	FTimerHandle AITimer;
	FDelegateHandle DamageDealtHandle;
	bool bActing = true;
};
