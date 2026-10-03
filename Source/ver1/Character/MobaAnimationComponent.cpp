#include "Character/MobaAnimationComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendSpace.h"
#include "Character/MobaCharacter.h"
#include "Combat/MobaCombatComponent.h"
#include "Combat/MobaCombatLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

namespace
{
	/** 이 속도를 넘게 움직이면 공격 동작을 끊고 걷는 동작으로 돌아간다. */
	constexpr float MoveCancelSpeed = 40.0f;
}

UMobaAnimationComponent::UMobaAnimationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UMobaAnimationComponent::Configure(USkeletalMeshComponent* InMesh, const FMobaAnimationSet& InSet)
{
	if (!InMesh || InSet.IsEmpty())
	{
		return;
	}

	Mesh = InMesh;
	Locomotion = InSet.Locomotion.LoadSynchronous();
	LocomotionSpeedScale = InSet.LocomotionSpeedScale;

	SpeedAxis = 0;
	if (const UBlendSpace* BlendSpace = Cast<UBlendSpace>(Locomotion))
	{
		for (int32 Axis = 0; Axis < 2; ++Axis)
		{
			if (BlendSpace->GetBlendParameter(Axis).DisplayName.Equals(TEXT("Speed"), ESearchCase::IgnoreCase))
			{
				SpeedAxis = Axis;
				break;
			}
		}
	}
	DeathAnimation = InSet.DeathAnimation.LoadSynchronous();

	AttackAnimations.Reset();
	for (const TSoftObjectPtr<UAnimSequenceBase>& Animation : InSet.AttackAnimations)
	{
		if (UAnimSequenceBase* Loaded = Animation.LoadSynchronous())
		{
			AttackAnimations.Add(Loaded);
		}
	}

	AbilityAnimations.Reset();
	for (const TPair<EMobaAbilitySlot, TSoftObjectPtr<UAnimSequenceBase>>& Entry : InSet.AbilityAnimations)
	{
		if (UAnimSequenceBase* Loaded = Entry.Value.LoadSynchronous())
		{
			AbilityAnimations.Add(Entry.Key, Loaded);
		}
	}

	// 공격·스킬 시점을 듣는다. 다시 설정될 수 있으므로 이전 구독은 지운다.
	if (UMobaCombatComponent* Combat = GetOwner()->FindComponentByClass<UMobaCombatComponent>())
	{
		Combat->OnAttackStarted.Remove(AttackStartedHandle);
		AttackStartedHandle = Combat->OnAttackStarted.AddUObject(this, &UMobaAnimationComponent::HandleAttackStarted);
	}

	if (AMobaCharacter* Champion = Cast<AMobaCharacter>(GetOwner()))
	{
		Champion->OnAbilityCast.Remove(AbilityCastHandle);
		AbilityCastHandle = Champion->OnAbilityCast.AddUObject(this, &UMobaAnimationComponent::HandleAbilityCast);
	}

	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	bPlayingOneShot = false;
	bPlayingDeath = false;
	PlayLocomotion();
	SetComponentTickEnabled(true);
}

void UMobaAnimationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMobaCombatComponent* Combat = GetOwner()->FindComponentByClass<UMobaCombatComponent>())
	{
		Combat->OnAttackStarted.Remove(AttackStartedHandle);
	}

	if (AMobaCharacter* Champion = Cast<AMobaCharacter>(GetOwner()))
	{
		Champion->OnAbilityCast.Remove(AbilityCastHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UMobaAnimationComponent::PlayLocomotion()
{
	bPlayingOneShot = false;
	if (Mesh && Locomotion)
	{
		Mesh->PlayAnimation(Locomotion, true);
	}
}

void UMobaAnimationComponent::PlayOneShot(UAnimSequenceBase* Animation, float MaxDuration, bool bCancelOnMove)
{
	if (!Mesh || !Animation || bPlayingDeath)
	{
		return;
	}

	const float Length = Animation->GetPlayLength();
	const float PlayRate = MaxDuration > 0.0f && Length > MaxDuration ? FMath::Min(Length / MaxDuration, 3.0f) : 1.0f;

	Mesh->PlayAnimation(Animation, false);
	Mesh->SetPlayRate(PlayRate);

	bPlayingOneShot = true;
	bOneShotCancelsOnMove = bCancelOnMove;
	OneShotEndTime = GetWorld()->GetTimeSeconds() + Length / PlayRate;
}

void UMobaAnimationComponent::HandleAttackStarted(AActor* Target)
{
	if (AttackAnimations.Num() == 0)
	{
		return;
	}

	// 공격 주기 안에 동작이 끝나야 다음 공격과 겹치지 않는다.
	const UAbilitySystemComponent* AbilitySystem = MobaCombat::GetAbilitySystem(GetOwner());
	const float AttackSpeed = AbilitySystem ? AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetAttackSpeedAttribute()) : 1.0f;
	const float Period = AttackSpeed > 0.0f ? 1.0f / AttackSpeed : 1.0f;

	UAnimSequenceBase* Animation = AttackAnimations[NextAttackIndex % AttackAnimations.Num()];
	++NextAttackIndex;
	PlayOneShot(Animation, Period * 0.9f, true);
}

void UMobaAnimationComponent::HandleAbilityCast(EMobaAbilitySlot Slot)
{
	UAnimSequenceBase* Animation = nullptr;
	if (const TObjectPtr<UAnimSequenceBase>* Found = AbilityAnimations.Find(Slot))
	{
		Animation = *Found;
	}
	else if (AttackAnimations.Num() > 0)
	{
		Animation = AttackAnimations[0];
	}

	// 스킬 동작은 1초 안에 끝내고, 이동으로 끊지 않는다(돌진처럼 시전과 함께 움직이는 스킬이 있다).
	PlayOneShot(Animation, 1.0f, false);
}

void UMobaAnimationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Mesh)
	{
		return;
	}

	const bool bDead = MobaCombat::IsDead(GetOwner());
	if (bDead)
	{
		if (!bPlayingDeath && DeathAnimation)
		{
			bPlayingDeath = true;
			bPlayingOneShot = false;
			Mesh->PlayAnimation(DeathAnimation, false);
		}
		return;
	}

	if (bPlayingDeath)
	{
		// 되살아났다.
		bPlayingDeath = false;
		PlayLocomotion();
	}

	const float Speed = GetOwner()->GetVelocity().Size2D();

	if (bPlayingOneShot)
	{
		const bool bFinished = GetWorld()->GetTimeSeconds() >= OneShotEndTime;
		const bool bCancelled = bOneShotCancelsOnMove && Speed > MoveCancelSpeed;
		if (!bFinished && !bCancelled)
		{
			return;
		}
		PlayLocomotion();
	}

	if (UAnimSingleNodeInstance* Instance = Mesh->GetSingleNodeInstance())
	{
		if (Cast<UBlendSpace>(Locomotion))
		{
			const float Input = Speed * LocomotionSpeedScale;
			Instance->SetBlendSpacePosition(SpeedAxis == 1 ? FVector(0.0, Input, 0.0) : FVector(Input, 0.0, 0.0));
		}
	}
}
