#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "MobaAttributeSet.generated.h"

/** 유닛이 처음 생성될 때의 기본 스탯. */
USTRUCT(BlueprintType)
struct FMobaBaseStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MaxHealth = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MaxMana = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float AttackDamage = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float AbilityPower = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float Armor = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MagicResist = 30.0f;

	/** 초당 공격 횟수. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float AttackSpeed = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MoveSpeed = 335.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float AttackRange = 550.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float HealthRegenPerSecond = 1.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float ManaRegenPerSecond = 1.6f;
};

/** 챔피언 레벨이 1 오를 때마다 더해지는 스탯. */
USTRUCT(BlueprintType)
struct FMobaStatGrowth
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Growth")
	float MaxHealth = 95.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Growth")
	float MaxMana = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Growth")
	float AttackDamage = 3.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Growth")
	float Armor = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Growth")
	float MagicResist = 1.3f;

	/** 기본 공격 속도에 곱해 더하는 비율. 0.025면 레벨당 2.5%. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Growth")
	float AttackSpeedFraction = 0.025f;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMobaOutOfHealth, AActor* /*Instigator*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMobaMoveSpeedChanged, float /*NewMoveSpeed*/);

UCLASS()
class VER1_API UMobaAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	/** 불굴(Buff_Unbreakable) 상태에서 받는 피해 배율. */
	static constexpr float UnbreakableDamageTaken = 0.5f;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	/** 서버 전용. 아직 어떤 효과도 적용되지 않은 상태에서 기본 스탯을 채운다. */
	void InitFromBaseStats(const FMobaBaseStats& Stats);

	/** 서버에서 체력이 0이 되는 순간 호출된다. */
	FOnMobaOutOfHealth OnOutOfHealth;

	/** 서버와 클라이언트 양쪽에서 이동 속도가 바뀔 때 호출된다. */
	FOnMobaMoveSpeedChanged OnMoveSpeedChanged;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UMobaAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UMobaAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Mana, Category = "Attributes")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS_BASIC(UMobaAttributeSet, Mana)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMana, Category = "Attributes")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS_BASIC(UMobaAttributeSet, MaxMana)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackDamage, Category = "Attributes")
	FGameplayAttributeData AttackDamage;
	ATTRIBUTE_ACCESSORS_BASIC(UMobaAttributeSet, AttackDamage)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AbilityPower, Category = "Attributes")
	FGameplayAttributeData AbilityPower;
	ATTRIBUTE_ACCESSORS_BASIC(UMobaAttributeSet, AbilityPower)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Armor, Category = "Attributes")
	FGameplayAttributeData Armor;
	ATTRIBUTE_ACCESSORS_BASIC(UMobaAttributeSet, Armor)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MagicResist, Category = "Attributes")
	FGameplayAttributeData MagicResist;
	ATTRIBUTE_ACCESSORS_BASIC(UMobaAttributeSet, MagicResist)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackSpeed, Category = "Attributes")
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS_BASIC(UMobaAttributeSet, AttackSpeed)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeed, Category = "Attributes")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS_BASIC(UMobaAttributeSet, MoveSpeed)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackRange, Category = "Attributes")
	FGameplayAttributeData AttackRange;
	ATTRIBUTE_ACCESSORS_BASIC(UMobaAttributeSet, AttackRange)

	/** 피해를 체력보다 먼저 흡수하는 보호막. 챔피언이 시간에 따라 줄이고 없앤다(AMobaCharacter::AddShield). */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Shield, Category = "Attributes")
	FGameplayAttributeData Shield;
	ATTRIBUTE_ACCESSORS_BASIC(UMobaAttributeSet, Shield)

	/** 기본 공격으로 준 피해 중 체력으로 돌려받는 비율. 0.1이면 10%. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_LifeSteal, Category = "Attributes")
	FGameplayAttributeData LifeSteal;
	ATTRIBUTE_ACCESSORS_BASIC(UMobaAttributeSet, LifeSteal)

	/** 기본 공격이 치명타가 될 확률. 0.25면 25%(1을 넘으면 항상). 치명타는 공격력 몫에 MobaBalance::CritDamageMultiplier를 곱한다. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CritChance, Category = "Attributes")
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS_BASIC(UMobaAttributeSet, CritChance)

	/** 스킬 가속. 스킬 재사용 대기 시간이 100 / (100 + 가속) 배가 된다. 소환사 주문에는 적용되지 않는다. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AbilityHaste, Category = "Attributes")
	FGameplayAttributeData AbilityHaste;
	ATTRIBUTE_ACCESSORS_BASIC(UMobaAttributeSet, AbilityHaste)

	/** 방어력·마법 저항력이 이미 반영된 최종 피해량. 서버에서만 쓰이고 즉시 체력에서 차감된다. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS_BASIC(UMobaAttributeSet, IncomingDamage)

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxMana(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_AttackDamage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_AbilityPower(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Armor(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MagicResist(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_AttackSpeed(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_AttackRange(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_LifeSteal(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_CritChance(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Shield(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_AbilityHaste(const FGameplayAttributeData& OldValue);
};
