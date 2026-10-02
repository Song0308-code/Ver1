#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Game/MobaTypes.h"
#include "MobaMonster.generated.h"

class AMobaJungleCamp;
class UMobaCombatComponent;
class UStaticMeshComponent;

/** 정글 몬스터 한 마리의 수치. */
USTRUCT(BlueprintType)
struct FMobaMonsterConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Monster")
	FMobaBaseStats Stats;

	UPROPERTY(EditAnywhere, Category = "Monster")
	bool bRanged = false;

	UPROPERTY(EditAnywhere, Category = "Monster")
	float ProjectileSpeed = 1200.0f;

	/** 맞아도 반격하지 않는다(바위게). */
	UPROPERTY(EditAnywhere, Category = "Monster")
	bool bPassive = false;

	/** 막타를 친 챔피언이 받는 골드와 경험치. */
	UPROPERTY(EditAnywhere, Category = "Monster")
	float GoldReward = 30.0f;

	UPROPERTY(EditAnywhere, Category = "Monster")
	float ExperienceReward = 50.0f;

	/** 임시 외형의 크기 배율. 충돌 캡슐도 함께 커진다. */
	UPROPERTY(EditAnywhere, Category = "Monster")
	float VisualScale = 1.0f;

	/** 캠프 중심에서 떨어진 자리. */
	UPROPERTY(EditAnywhere, Category = "Monster")
	FVector2D Offset = FVector2D::ZeroVector;
};

/**
 * 캠프에 서 있다가 자기나 같은 캠프의 몬스터를 때린 챔피언에게 반격하는 중립 몬스터.
 * 캠프에서 너무 멀어지면 싸움을 포기하고 돌아가 체력을 채운다.
 */
UCLASS()
class VER1_API AMobaMonster : public ACharacter, public IMobaTeamInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMobaMonster();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual EMobaTeam GetTeam() const override { return EMobaTeam::Neutral; }
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }

	/** 서버 전용. 스폰을 마치기 전에 호출한다. */
	void InitMonster(AMobaJungleCamp* InCamp, const FMobaMonsterConfig& InConfig, const FVector& InHomeLocation);

	/** 서버 전용. 경기가 끝났을 때 움직임과 공격을 멈춘다. */
	void StopActing();

	const FMobaMonsterConfig& GetConfig() const { return Config; }
	AMobaJungleCamp* GetCamp() const { return Camp; }

protected:
	UFUNCTION()
	void OnRep_VisualScale();

	void ApplyAppearance();

	void UpdateAI();

	/** 싸움을 포기하고 제자리로 걸어간다. */
	void ReturnHome();

	void HandleOutOfHealth(AActor* DamageInstigator);
	void HandleDamageDealt(AActor* DamageInstigator, AActor* Victim, float Damage);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMobaCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UMobaAttributeSet> AttributeSet;

	UPROPERTY(ReplicatedUsing = OnRep_VisualScale)
	float VisualScale = 1.0f;

	UPROPERTY()
	TObjectPtr<AMobaJungleCamp> Camp;

	FMobaMonsterConfig Config;
	FVector HomeLocation = FVector::ZeroVector;

	/** 제자리에서 이보다 멀어지거나 대상이 이보다 멀리 달아나면 포기한다. */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float LeashRange = 800.0f;

	/** 싸우지 않는 동안 1초에 회복하는 최대 체력의 비율. */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float IdleHealFractionPerSecond = 0.25f;

	FTimerHandle AITimer;
	FDelegateHandle DamageDealtHandle;
	bool bActing = true;
	bool bReturning = false;
};
