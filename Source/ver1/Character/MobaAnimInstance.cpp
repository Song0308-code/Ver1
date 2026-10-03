#include "Character/MobaAnimInstance.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaCharacter.h"
#include "Combat/MobaCombatComponent.h"
#include "Combat/MobaCombatLibrary.h"
#include "Game/MobaGameplayTags.h"
#include "GameFramework/Pawn.h"

void UMobaAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	APawn* Pawn = TryGetPawnOwner();
	if (!Pawn)
	{
		return;
	}

	if (UMobaCombatComponent* Combat = Pawn->FindComponentByClass<UMobaCombatComponent>())
	{
		AttackStartedHandle = Combat->OnAttackStarted.AddUObject(this, &UMobaAnimInstance::HandleAttackStarted);
	}

	if (AMobaCharacter* Champion = Cast<AMobaCharacter>(Pawn))
	{
		AbilityCastHandle = Champion->OnAbilityCast.AddUObject(this, &UMobaAnimInstance::HandleAbilityCast);
	}
}

void UMobaAnimInstance::NativeUninitializeAnimation()
{
	if (APawn* Pawn = TryGetPawnOwner())
	{
		if (UMobaCombatComponent* Combat = Pawn->FindComponentByClass<UMobaCombatComponent>())
		{
			Combat->OnAttackStarted.Remove(AttackStartedHandle);
		}

		if (AMobaCharacter* Champion = Cast<AMobaCharacter>(Pawn))
		{
			Champion->OnAbilityCast.Remove(AbilityCastHandle);
		}
	}

	Super::NativeUninitializeAnimation();
}

void UMobaAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const APawn* Pawn = TryGetPawnOwner();
	if (!Pawn)
	{
		return;
	}

	Speed = Pawn->GetVelocity().Size2D();
	bIsMoving = Speed > 5.0f;

	const UAbilitySystemComponent* AbilitySystem = MobaCombat::GetAbilitySystem(Pawn);
	bIsDead = AbilitySystem && AbilitySystem->HasMatchingGameplayTag(MobaTags::State_Dead);
	bIsStunned = AbilitySystem && AbilitySystem->HasMatchingGameplayTag(MobaTags::State_Stunned);
	bIsRecalling = AbilitySystem && AbilitySystem->HasMatchingGameplayTag(MobaTags::State_Recalling);
}

void UMobaAnimInstance::HandleAttackStarted(AActor* Target)
{
	++AttackCount;
	OnBasicAttack(Target);
}

void UMobaAnimInstance::HandleAbilityCast(EMobaAbilitySlot Slot)
{
	++AbilityCastCount;
	LastAbilitySlot = Slot;
	OnAbilityCast(Slot);
}
