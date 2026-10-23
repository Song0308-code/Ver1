#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Game/MobaTypes.h"
#include "MobaPassive.generated.h"

class AMobaCharacter;

/** 패시브가 기본 공격 한 번에 더하는 피해와 효과. */
struct FMobaAttackBonus
{
	float BonusPhysicalDamage = 0.0f;
	float BonusMagicDamage = 0.0f;
	float ManaRestore = 0.0f;
};

/**
 * 챔피언의 고유 패시브. 서버에서 챔피언마다 하나씩 만들어지고, 챔피언이 전투 사건을 알려 준다.
 * 화면에 보여 줄 중첩 수는 챔피언의 복제 값(PassiveStacks)으로 클라이언트에 전해진다.
 */
UCLASS(Abstract)
class VER1_API UMobaPassive : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AMobaCharacter* InOwner) { Owner = InOwner; }

	const FText& GetDisplayName() const { return DisplayName; }
	const FText& GetDescription() const { return Description; }

	/** HUD 패시브 칸에 숫자로 보여 줄 최대 중첩. 0이면 숫자 대신 켜짐·꺼짐만 보여 준다. */
	int32 GetMaxStacks() const { return MaxStacks; }

	/** 기본 공격 투사체를 쏘기 직전. 이번 공격에 더할 것을 돌려준다. */
	virtual FMobaAttackBonus OnBasicAttack(AActor* Target) { return FMobaAttackBonus(); }

	/** 스킬이 발동됐을 때. */
	virtual void OnAbilityCast(EMobaAbilitySlot Slot) {}

	/** 피해를 받았을 때. */
	virtual void OnDamageTaken(float Damage) {}

	/** 0.25초마다(챔피언의 재생 주기). 죽어 있을 때는 오지 않는다. */
	virtual void Tick(float DeltaSeconds) {}

	/** 부활했을 때. */
	virtual void OnRespawned() {}

protected:
	/** 챔피언의 중첩 표시를 바꾼다. */
	void SetStacks(int32 Stacks) const;

	UPROPERTY(Transient)
	TObjectPtr<AMobaCharacter> Owner;

	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	FText Description;

	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	int32 MaxStacks = 0;
};

/** 레인저: 기본 공격 세 번째마다 공격력의 일부만큼 추가 물리 피해를 준다. */
UCLASS()
class VER1_API UMobaPassive_Volley : public UMobaPassive
{
	GENERATED_BODY()

public:
	UMobaPassive_Volley();

	virtual FMobaAttackBonus OnBasicAttack(AActor* Target) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	int32 AttacksPerProc = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float BonusAttackDamageRatio = 0.4f;

	int32 AttackCount = 0;
};

/** 뱅가드: 한동안 피해를 받지 않으면 최대 체력에 비례해 빠르게 회복한다. */
UCLASS()
class VER1_API UMobaPassive_Resolve : public UMobaPassive
{
	GENERATED_BODY()

public:
	UMobaPassive_Resolve();

	virtual void OnDamageTaken(float Damage) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnRespawned() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float OutOfCombatDelay = 5.0f;

	/** 1초에 회복하는 최대 체력의 비율. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float HealFractionPerSecond = 0.015f;

	float TimeSinceDamage = 0.0f;
};

/** 미스틱: 스킬을 쓰면 다음 기본 공격이 마법 피해를 더하고 마나를 회복한다. */
UCLASS()
class VER1_API UMobaPassive_Charge : public UMobaPassive
{
	GENERATED_BODY()

public:
	UMobaPassive_Charge();

	virtual FMobaAttackBonus OnBasicAttack(AActor* Target) override;
	virtual void OnAbilityCast(EMobaAbilitySlot Slot) override;
	virtual void OnRespawned() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float BonusMagicDamage = 30.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float AbilityPowerRatio = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float ManaRestore = 15.0f;

	bool bCharged = false;
};

/** 섀도우: 그림자 숨기(W)를 쓴 뒤 첫 기본 공격이 공격력의 일부만큼 추가 물리 피해를 준다. */
UCLASS()
class VER1_API UMobaPassive_Ambush : public UMobaPassive
{
	GENERATED_BODY()

public:
	UMobaPassive_Ambush();

	virtual FMobaAttackBonus OnBasicAttack(AActor* Target) override;
	virtual void OnAbilityCast(EMobaAbilitySlot Slot) override;
	virtual void OnRespawned() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Passive")
	float BonusAttackDamageRatio = 0.5f;

	bool bReady = false;
};
