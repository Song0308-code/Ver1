#include "Character/MobaCharacter.h"
#include "Vision/MobaVisionSubsystem.h"
#include "Game/MobaEffects.h"
#include "Game/MobaVisuals.h"
#include "AbilitySystem/Abilities/MobaAbilityArchetypes.h"
#include "AbilitySystem/MobaAbilityLibrary.h"
#include "AbilitySystem/MobaGameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaAnimationComponent.h"
#include "Character/MobaChampionData.h"
#include "Character/MobaPassive.h"
#include "AbilitySystem/Abilities/MobaSummonerSpells.h"
#include "Materials/MaterialInterface.h"
#include "Combat/MobaCombatComponent.h"
#include "Combat/MobaCombatEvents.h"
#include "Combat/MobaCombatLibrary.h"
#include "Game/MobaGameMode.h"
#include "Game/MobaGameplayTags.h"
#include "Game/MobaItems.h"
#include "GameFramework/Controller.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "Player/MobaPlayerState.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Vision/MobaWard.h"

AMobaCharacter::AMobaCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// 챔피언은 수가 적고 미니맵·전역 스킬에 필요하므로 거리와 무관하게 항상 복제한다.
	bAlwaysRelevant = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.0, 1440.0, 0.0);
	Movement->MaxWalkSpeed = 335.0f;

	// 가속·감속 없이 즉시 출발하고 멈춘다.
	Movement->MaxAcceleration = 8192.0f;
	Movement->BrakingDecelerationWalking = 8192.0f;
	Movement->GroundFriction = 8.0f;

	// 경로 추종이 이동 입력으로 들어가야 클라이언트 예측 이동이 그대로 동작한다.
	Movement->GetNavMovementProperties()->bUseAccelerationForPaths = true;

	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
	PlaceholderMesh->SetupAttachment(RootComponent);
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 엔진 기본 원기둥(지름 100, 높이 100)을 캡슐 크기에 맞춘다.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		PlaceholderMesh->SetStaticMesh(CylinderMesh.Object);
		MobaVisuals::UseColorableMaterial(PlaceholderMesh);
		const float Radius = GetCapsuleComponent()->GetUnscaledCapsuleRadius();
		const float HalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
		PlaceholderMesh->SetRelativeScale3D(FVector(Radius / 50.0f, Radius / 50.0f, HalfHeight / 50.0f));
	}

	CombatComponent = CreateDefaultSubobject<UMobaCombatComponent>(TEXT("CombatComponent"));
	CombatComponent->SetAutoAcquireWhenIdle(true);

	AnimationComponent = CreateDefaultSubobject<UMobaAnimationComponent>(TEXT("AnimationComponent"));

	// 메시는 보이기만 하고 충돌·클릭 판정은 캡슐이 맡는다.
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	DefaultAbilities.Add(EMobaAbilitySlot::Q, UMobaAbility_Skillshot::StaticClass());
	DefaultAbilities.Add(EMobaAbilitySlot::W, UMobaAbility_SelfBuff::StaticClass());
	DefaultAbilities.Add(EMobaAbilitySlot::E, UMobaAbility_Blink::StaticClass());
	DefaultAbilities.Add(EMobaAbilitySlot::R, UMobaAbility_GroundAoE::StaticClass());
}

void AMobaCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMobaCharacter, ChampionData);
	DOREPLIFETIME(AMobaCharacter, HomeLocation);
	DOREPLIFETIME(AMobaCharacter, RecallEndServerTime);
	DOREPLIFETIME(AMobaCharacter, PassiveStacks);
	DOREPLIFETIME(AMobaCharacter, PotionHealRemaining);
}

void AMobaCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyChampionAppearance();

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(RegenTimer, this, &AMobaCharacter::TickRegen, 0.25f, true);

		if (UMobaCombatEvents* Events = UMobaCombatEvents::Get(this))
		{
			DamageDealtHandle = Events->OnDamageDealt.AddUObject(this, &AMobaCharacter::HandleDamageDealt);
		}
	}
}

void AMobaCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMobaCombatEvents* Events = UMobaCombatEvents::Get(this))
	{
		Events->OnDamageDealt.Remove(DamageDealtHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AMobaCharacter::OnRep_ChampionData()
{
	ApplyChampionAppearance();
}

void AMobaCharacter::ApplyChampionAppearance()
{
	if (!ChampionData)
	{
		return;
	}

	const float Radius = GetCapsuleComponent()->GetUnscaledCapsuleRadius();
	const float HalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const FVector& Shape = ChampionData->PlaceholderScale;
	PlaceholderMesh->SetRelativeScale3D(FVector(Radius / 50.0f * Shape.X, Radius / 50.0f * Shape.Y, HalfHeight / 50.0f * Shape.Z));

	// 원기둥의 바닥이 캡슐 바닥에 닿도록 높이를 맞춘다.
	PlaceholderMesh->SetRelativeLocation(FVector(0.0, 0.0, HalfHeight * (Shape.Z - 1.0)));

	USkeletalMesh* ChampionMesh = ChampionData->Mesh.LoadSynchronous();
	if (!ChampionMesh)
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = GetMesh();
	MeshComponent->SetSkeletalMesh(ChampionMesh);
	MeshComponent->SetRelativeLocationAndRotation(ChampionData->MeshOffset, ChampionData->MeshRotation);
	MeshComponent->SetRelativeScale3D(FVector(ChampionData->MeshScale));
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (UMaterialInterface* Material = ChampionData->MeshMaterial.LoadSynchronous())
	{
		for (int32 Index = 0; Index < MeshComponent->GetNumMaterials(); ++Index)
		{
			MeshComponent->SetMaterial(Index, Material);
		}
	}

	if (UClass* AnimClass = ChampionData->AnimClass.LoadSynchronous())
	{
		MeshComponent->SetAnimInstanceClass(AnimClass);
	}
	else
	{
		AnimationComponent->Configure(MeshComponent, ChampionData->Animations);
	}

	// 임시 원기둥은 발밑의 납작한 팀 색 원판으로 바꿔 아군·적을 구분하는 표시로 쓴다.
	bUsingSkeletalMesh = true;
	PlaceholderMesh->SetRelativeScale3D(FVector(Radius * 1.6f / 50.0f, Radius * 1.6f / 50.0f, 0.02f));
	PlaceholderMesh->SetRelativeLocation(FVector(0.0, 0.0, -HalfHeight + 2.0f));
	PlaceholderMesh->SetCastShadow(false);
}

const FMobaBaseStats& AMobaCharacter::GetBaseStats() const
{
	return ChampionData ? ChampionData->BaseStats : BaseStats;
}

const FMobaStatGrowth& AMobaCharacter::GetStatGrowth() const
{
	return ChampionData ? ChampionData->StatGrowth : StatGrowth;
}

EMobaTeam AMobaCharacter::GetTeam() const
{
	const AMobaPlayerState* MobaPlayerState = GetPlayerState<AMobaPlayerState>();
	return MobaPlayerState ? MobaPlayerState->GetTeam() : EMobaTeam::None;
}

UAbilitySystemComponent* AMobaCharacter::GetAbilitySystemComponent() const
{
	const AMobaPlayerState* MobaPlayerState = GetPlayerState<AMobaPlayerState>();
	return MobaPlayerState ? MobaPlayerState->GetAbilitySystemComponent() : nullptr;
}

bool AMobaCharacter::IsDead() const
{
	const UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	return AbilitySystem && AbilitySystem->HasMatchingGameplayTag(MobaTags::State_Dead);
}

bool AMobaCharacter::IsRooted() const
{
	const UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	return AbilitySystem && AbilitySystem->HasMatchingGameplayTag(MobaTags::State_Rooted);
}

bool AMobaCharacter::IsStunned() const
{
	const UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	return AbilitySystem && AbilitySystem->HasMatchingGameplayTag(MobaTags::State_Stunned);
}

void AMobaCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	HomeLocation = GetActorLocation();

	if (!ChampionData)
	{
		// 접속할 때 정해 둔 챔피언을 쓰고, 없으면 기본 챔피언으로 한다.
		const AMobaPlayerState* MobaPlayerState = GetPlayerState<AMobaPlayerState>();
		const AMobaGameMode* GameMode = GetWorld()->GetAuthGameMode<AMobaGameMode>();
		if (MobaPlayerState && MobaPlayerState->GetChampion())
		{
			ChampionData = MobaPlayerState->GetChampion();
		}
		else if (GameMode)
		{
			ChampionData = GameMode->GetDefaultChampion();
		}

		ApplyChampionAppearance();
	}

	if (!PassiveInstance && ChampionData && ChampionData->Passive)
	{
		PassiveInstance = NewObject<UMobaPassive>(this, ChampionData->Passive);
		PassiveInstance->Initialize(this);
	}

	BindToPlayerStateTeam();
	InitAbilitySystem();
}

void AMobaCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	BindToPlayerStateTeam();
	InitAbilitySystem();
}

void AMobaCharacter::InitAbilitySystem()
{
	AMobaPlayerState* MobaPlayerState = GetPlayerState<AMobaPlayerState>();
	if (!MobaPlayerState)
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystem = MobaPlayerState->GetAbilitySystemComponent();
	UMobaAttributeSet* AttributeSet = MobaPlayerState->GetAttributeSet();
	AbilitySystem->InitAbilityActorInfo(MobaPlayerState, this);

	if (BoundAbilitySystem.Get() == AbilitySystem)
	{
		return;
	}
	BoundAbilitySystem = AbilitySystem;

	AbilitySystem->RegisterGameplayTagEvent(MobaTags::State_Dead, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AMobaCharacter::HandleDeadTagChanged);
	AbilitySystem->RegisterGameplayTagEvent(MobaTags::State_Stunned, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AMobaCharacter::HandleStunnedTagChanged);
	AbilitySystem->RegisterGameplayTagEvent(MobaTags::State_Rooted, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AMobaCharacter::HandleStunnedTagChanged);
	AttributeSet->OnMoveSpeedChanged.AddUObject(this, &AMobaCharacter::HandleMoveSpeedChanged);

	if (HasAuthority())
	{
		AttributeSet->OnOutOfHealth.AddUObject(this, &AMobaCharacter::HandleOutOfHealth);
		MobaPlayerState->OnLevelUp.AddUObject(this, &AMobaCharacter::HandleLevelUp);

		if (!MobaPlayerState->bStatsInitialized)
		{
			MobaPlayerState->bStatsInitialized = true;
			GrantChampionKit(MobaPlayerState);
		}
	}

	// 클라이언트에서는 스탯이 아직 복제되지 않았을 수 있다. 그 경우 복제 시점에 다시 반영된다.
	if (AttributeSet->GetMoveSpeed() > 0.0f)
	{
		HandleMoveSpeedChanged(AttributeSet->GetMoveSpeed());
	}

	ApplyDeadState(IsDead());
}

void AMobaCharacter::GrantChampionKit(AMobaPlayerState* MobaPlayerState)
{
	UAbilitySystemComponent* AbilitySystem = MobaPlayerState->GetAbilitySystemComponent();

	MobaPlayerState->GetAttributeSet()->InitFromBaseStats(GetBaseStats());

	if (ChampionData)
	{
		CombatComponent->ConfigureBasicAttack(ChampionData->bRangedBasicAttack, ChampionData->BasicAttackProjectileSpeed);
	}

	const TMap<EMobaAbilitySlot, TSubclassOf<UMobaGameplayAbility>>& Abilities = ChampionData ? ChampionData->Abilities : DefaultAbilities;
	for (const TPair<EMobaAbilitySlot, TSubclassOf<UMobaGameplayAbility>>& Entry : Abilities)
	{
		if (Entry.Value)
		{
			// 스킬은 0단계로 받고, 스킬 포인트로 찍어야 쓸 수 있다.
			FGameplayAbilitySpec Spec(Entry.Value, 0);
			Spec.GetDynamicSpecSourceTags().AddTag(MobaAbility::GetSlotTag(Entry.Key));
			AbilitySystem->GiveAbility(Spec);
		}
	}

	// 소환사 주문은 로비에서 고른 것을 처음부터 쓸 수 있는 상태(1단계)로 준다.
	const EMobaAbilitySlot SpellSlots[] = { EMobaAbilitySlot::D, EMobaAbilitySlot::F };
	for (int32 Index = 0; Index < 2; ++Index)
	{
		if (const TSubclassOf<UMobaGameplayAbility> SpellClass = MobaSummoner::GetAbilityClass(MobaPlayerState->GetSummonerSpell(Index)))
		{
			FGameplayAbilitySpec Spec(SpellClass, 1);
			Spec.GetDynamicSpecSourceTags().AddTag(MobaAbility::GetSlotTag(SpellSlots[Index]));
			AbilitySystem->GiveAbility(Spec);
		}
	}
}

void AMobaCharacter::HandleMoveSpeedChanged(float NewMoveSpeed)
{
	GetCharacterMovement()->MaxWalkSpeed = NewMoveSpeed;
}

void AMobaCharacter::HandleOutOfHealth(AActor* DamageInstigator)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	if (!AbilitySystem || IsDead())
	{
		return;
	}

	AbilitySystem->AddLooseGameplayTag(MobaTags::State_Dead, 1, EGameplayTagReplicationState::TagOnly);
	CombatComponent->SetAttackTarget(nullptr);

	// 정글 버프는 죽으면 사라진다. 드래곤 보상만 남는다.
	FGameplayTagContainer LostBuffs;
	LostBuffs.AddTag(MobaTags::Buff_Blue);
	LostBuffs.AddTag(MobaTags::Buff_Red);
	LostBuffs.AddTag(MobaTags::Buff_Baron);
	AbilitySystem->RemoveActiveEffectsWithGrantedTags(LostBuffs);

	if (AMobaGameMode* GameMode = GetWorld()->GetAuthGameMode<AMobaGameMode>())
	{
		GameMode->HandleChampionKilled(this, DamageInstigator);
	}
}

void AMobaCharacter::HandleDeadTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		MobaEffects::Play(this, EMobaEffect::UnitDeath, GetActorLocation(), 1.0f, this);
	}

	ApplyDeadState(NewCount > 0);
}

void AMobaCharacter::HandleStunnedTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	UpdateMovementLock();
}

void AMobaCharacter::ApplyDeadState(bool bDead)
{
	GetWorldTimerManager().ClearTimer(DeathHideTimer);

	// 사망 애니메이션이 있으면 쓰러지는 모습을 잠깐 보여 준 뒤 숨긴다.
	if (bDead && bUsingSkeletalMesh && AnimationComponent->HasDeathAnimation())
	{
		GetWorldTimerManager().SetTimer(DeathHideTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (IsDead())
			{
				SetActorHiddenInGame(true);
			}
		}), DeathHideDelay, false);
	}
	else
	{
		SetActorHiddenInGame(bDead);
	}
	SetActorEnableCollision(!bDead);

	UpdateMovementLock();
}

void AMobaCharacter::UpdateMovementLock()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!CanMove())
	{
		if (IsLocallyControlled() && GetController())
		{
			GetController()->StopMovement();
		}
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	else if (Movement->MovementMode == MOVE_None)
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
}

void AMobaCharacter::Respawn(const FVector& Location, const FRotator& Rotation)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	if (!HasAuthority() || !AbilitySystem)
	{
		return;
	}

	TeleportChampion(Location, Rotation);

	AbilitySystem->SetNumericAttributeBase(UMobaAttributeSet::GetHealthAttribute(), AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxHealthAttribute()));
	AbilitySystem->SetNumericAttributeBase(UMobaAttributeSet::GetManaAttribute(), AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxManaAttribute()));
	AbilitySystem->RemoveLooseGameplayTag(MobaTags::State_Dead, 1, EGameplayTagReplicationState::TagOnly);
	PotionHealRemaining = 0.0f;

	if (PassiveInstance)
	{
		PassiveInstance->OnRespawned();
	}
}

void AMobaCharacter::TeleportChampion(const FVector& Location, const FRotator& Rotation)
{
	if (!HasAuthority())
	{
		return;
	}

	SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
	ClientTeleport(Location, Rotation);
}

void AMobaCharacter::ClientTeleport_Implementation(const FVector& Location, const FRotator& Rotation)
{
	// 걷던 중이었다면 이전 경로를 따라 되돌아가지 않도록 멈춘다.
	if (GetController())
	{
		GetController()->StopMovement();
	}

	SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
	GetCharacterMovement()->StopMovementImmediately();
}

void AMobaCharacter::CastAbility(EMobaAbilitySlot Slot, const FVector& TargetLocation)
{
	if (!CanAct())
	{
		return;
	}

	// 지연이 있어도 누르는 즉시 반응하도록, 조종하는 클라이언트는 시전 동작과 이펙트를 먼저 보여 준다.
	if (!HasAuthority() && IsLocallyControlled() && CanCastLocally(Slot))
	{
		PredictedCastTimes[static_cast<int32>(Slot)] = GetWorld()->GetTimeSeconds();
		PlayAbilityCastLocally(Slot);
	}

	ServerCastAbility(Slot, TargetLocation);
}

bool AMobaCharacter::CanCastLocally(EMobaAbilitySlot Slot) const
{
	const UMobaGameplayAbility* Ability = GetAbilityInSlot(Slot);
	const UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	float Remaining = 0.0f;
	float Duration = 0.0f;
	return Ability && AbilitySystem && GetAbilityLevel(Slot) >= 1
		&& !Ability->IsBlockedByCrowdControl(AbilitySystem)
		&& !GetCooldownRemaining(Slot, Remaining, Duration)
		&& AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetManaAttribute()) >= Ability->GetManaCost();
}

void AMobaCharacter::PlayAbilityCastLocally(EMobaAbilitySlot Slot)
{
	const FVector Feet = GetActorLocation() - FVector(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	MobaEffects::Play(this, EMobaEffect::Cast, Feet, 1.0f, this);
	OnAbilityCast.Broadcast(Slot);
}

void AMobaCharacter::ServerCastAbility_Implementation(EMobaAbilitySlot Slot, FVector_NetQuantize TargetLocation)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	if (!AbilitySystem)
	{
		return;
	}

	const FGameplayTag SlotTag = MobaAbility::GetSlotTag(Slot);
	for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
	{
		if (!Spec.GetDynamicSpecSourceTags().HasTagExact(SlotTag))
		{
			continue;
		}

		FGameplayAbilityTargetingLocationInfo SourceInfo;
		SourceInfo.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
		SourceInfo.LiteralTransform = FTransform(GetActorLocation());

		FGameplayAbilityTargetingLocationInfo TargetInfo;
		TargetInfo.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
		TargetInfo.LiteralTransform = FTransform(FVector(TargetLocation));

		FGameplayEventData Payload;
		Payload.Instigator = this;
		Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromLocations(SourceInfo, TargetInfo);

		AbilitySystem->TriggerAbilityFromGameplayEvent(Spec.Handle, AbilitySystem->AbilityActorInfo.Get(), SlotTag, &Payload, *AbilitySystem);
		return;
	}
}

void AMobaCharacter::HandleLevelUp(int32 NewLevel)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	if (!HasAuthority() || !AbilitySystem)
	{
		return;
	}

	const FMobaStatGrowth& Growth = GetStatGrowth();
	const auto AddToBase = [AbilitySystem](const FGameplayAttribute& Attribute, float Amount)
	{
		AbilitySystem->SetNumericAttributeBase(Attribute, AbilitySystem->GetNumericAttributeBase(Attribute) + Amount);
	};

	AddToBase(UMobaAttributeSet::GetMaxHealthAttribute(), Growth.MaxHealth);
	AddToBase(UMobaAttributeSet::GetMaxManaAttribute(), Growth.MaxMana);
	AddToBase(UMobaAttributeSet::GetAttackDamageAttribute(), Growth.AttackDamage);
	AddToBase(UMobaAttributeSet::GetArmorAttribute(), Growth.Armor);
	AddToBase(UMobaAttributeSet::GetMagicResistAttribute(), Growth.MagicResist);
	AddToBase(UMobaAttributeSet::GetAttackSpeedAttribute(), GetBaseStats().AttackSpeed * Growth.AttackSpeedFraction);

	// 늘어난 최대치만큼 현재 체력과 마나도 채워 준다.
	if (!IsDead())
	{
		AddToBase(UMobaAttributeSet::GetHealthAttribute(), Growth.MaxHealth);
		AddToBase(UMobaAttributeSet::GetManaAttribute(), Growth.MaxMana);
	}
}

void AMobaCharacter::TickRegen()
{
	UpdateShields();

	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	if (!AbilitySystem || IsDead() || MobaCombat::IsMatchOver(this))
	{
		return;
	}

	const float DeltaSeconds = 0.25f;
	const FMobaBaseStats& Stats = GetBaseStats();
	const bool bInFountain = FVector::DistSquared2D(GetActorLocation(), HomeLocation) <= FMath::Square(FountainRadius);

	const auto Regenerate = [AbilitySystem, DeltaSeconds, bInFountain, this](const FGameplayAttribute& Attribute, const FGameplayAttribute& MaxAttribute, float RegenPerSecond)
	{
		const float Current = AbilitySystem->GetNumericAttributeBase(Attribute);
		const float Max = AbilitySystem->GetNumericAttribute(MaxAttribute);
		if (Current >= Max)
		{
			return;
		}

		const float Gain = (RegenPerSecond + (bInFountain ? Max * FountainHealFractionPerSecond : 0.0f)) * DeltaSeconds;
		AbilitySystem->SetNumericAttributeBase(Attribute, FMath::Min(Current + Gain, Max));
	};

	const float MaxHealth = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxHealthAttribute());
	const float BonusHealthRegen = AbilitySystem->HasMatchingGameplayTag(MobaTags::Buff_Red) ? MaxHealth * RedBuffHealFractionPerSecond : 0.0f;
	const float BonusManaRegen = AbilitySystem->HasMatchingGameplayTag(MobaTags::Buff_Blue) ? BlueBuffManaRegenPerSecond : 0.0f;

	if (PassiveInstance)
	{
		PassiveInstance->Tick(DeltaSeconds);
	}

	// 물약 회복. 체력이 가득 차 있어도 시간은 흐른다.
	if (PotionHealRemaining > 0.0f)
	{
		const float Heal = FMath::Min(PotionHealPerSecond * DeltaSeconds, PotionHealRemaining);
		PotionHealRemaining -= Heal;
		const float Health = AbilitySystem->GetNumericAttributeBase(UMobaAttributeSet::GetHealthAttribute());
		AbilitySystem->SetNumericAttributeBase(UMobaAttributeSet::GetHealthAttribute(), FMath::Min(Health + Heal, MaxHealth));
		if (PotionHealRemaining < 0.01f)
		{
			PotionHealRemaining = 0.0f;
		}
	}

	Regenerate(UMobaAttributeSet::GetHealthAttribute(), UMobaAttributeSet::GetMaxHealthAttribute(), Stats.HealthRegenPerSecond + BonusHealthRegen);
	Regenerate(UMobaAttributeSet::GetManaAttribute(), UMobaAttributeSet::GetMaxManaAttribute(), Stats.ManaRegenPerSecond + BonusManaRegen);
}

float AMobaCharacter::GetWardCooldownRemaining() const
{
	const AMobaPlayerState* MobaPlayerState = GetPlayerState<AMobaPlayerState>();
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (!MobaPlayerState || !GameState)
	{
		return 0.0f;
	}

	return FMath::Max(MobaPlayerState->GetNextWardServerTime() - static_cast<float>(GameState->GetServerWorldTimeSeconds()), 0.0f);
}

FVector AMobaCharacter::ClampWardLocation(const FVector& Location) const
{
	// 너무 먼 곳을 찍었으면 놓을 수 있는 거리까지만 나간다. 높이는 발밑에 맞춘다.
	const FVector Feet = GetActorLocation() - FVector(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	FVector Offset = Location - Feet;
	Offset.Z = 0.0;
	return Feet + Offset.GetClampedToMaxSize(WardPlaceRange);
}

void AMobaCharacter::UseHeraldEye()
{
	const AMobaPlayerState* MobaPlayerState = GetPlayerState<AMobaPlayerState>();
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (CanAct() && MobaPlayerState && GameState && MobaPlayerState->GetHeraldEyeExpireServerTime() > GameState->GetServerWorldTimeSeconds())
	{
		ServerUseHeraldEye();
	}
}

void AMobaCharacter::ServerUseHeraldEye_Implementation()
{
	if (AMobaGameMode* GameMode = GetWorld()->GetAuthGameMode<AMobaGameMode>())
	{
		if (CanAct())
		{
			GameMode->SummonHerald(this);
		}
	}
}

void AMobaCharacter::PlaceControlWard(const FVector& Location)
{
	const AMobaPlayerState* MobaPlayerState = GetPlayerState<AMobaPlayerState>();
	if (CanAct() && MobaPlayerState && MobaPlayerState->GetControlWards() > 0)
	{
		ServerPlaceControlWard(Location);
	}
}

void AMobaCharacter::ServerPlaceControlWard_Implementation(FVector_NetQuantize Location)
{
	AMobaPlayerState* MobaPlayerState = GetPlayerState<AMobaPlayerState>();
	if (!MobaPlayerState || MobaPlayerState->GetControlWards() <= 0 || !CanAct() || MobaCombat::IsMatchOver(this))
	{
		return;
	}

	AMobaWard* Ward = AMobaWard::SpawnWard(GetWorld(), GetTeam(), ClampWardLocation(Location), 0.0f, true);
	if (!Ward)
	{
		return;
	}

	MobaPlayerState->SetControlWards(MobaPlayerState->GetControlWards() - 1);

	// 한 사람당 하나만 남는다.
	if (AMobaWard* Previous = ActiveControlWard.Get())
	{
		Previous->Destroy();
	}
	ActiveControlWard = Ward;
}

void AMobaCharacter::PlaceWard(const FVector& Location)
{
	if (CanAct() && GetWardCooldownRemaining() <= 0.0f)
	{
		ServerPlaceWard(Location);
	}
}

void AMobaCharacter::ServerPlaceWard_Implementation(FVector_NetQuantize Location)
{
	AMobaPlayerState* MobaPlayerState = GetPlayerState<AMobaPlayerState>();
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (!MobaPlayerState || !GameState || !CanAct() || MobaCombat::IsMatchOver(this) || GetWardCooldownRemaining() > 0.0f)
	{
		return;
	}

	AMobaWard* Ward = AMobaWard::SpawnWard(GetWorld(), GetTeam(), ClampWardLocation(Location), WardDuration);
	if (!Ward)
	{
		return;
	}

	MobaPlayerState->SetNextWardServerTime(static_cast<float>(GameState->GetServerWorldTimeSeconds()) + WardCooldown);

	ActiveWards.RemoveAll([](const TWeakObjectPtr<AMobaWard>& Existing) { return !Existing.IsValid(); });
	ActiveWards.Add(Ward);
	while (ActiveWards.Num() > MaxActiveWards)
	{
		ActiveWards[0]->Destroy();
		ActiveWards.RemoveAt(0);
	}
}

int32 AMobaCharacter::GetAbilityLevel(EMobaAbilitySlot Slot) const
{
	const UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	if (!AbilitySystem)
	{
		return 0;
	}

	const FGameplayTag SlotTag = MobaAbility::GetSlotTag(Slot);
	for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(SlotTag))
		{
			return Spec.Level;
		}
	}
	return 0;
}

bool AMobaCharacter::CanLevelUpAbility(EMobaAbilitySlot Slot) const
{
	const AMobaPlayerState* MobaPlayerState = GetPlayerState<AMobaPlayerState>();
	const UMobaGameplayAbility* Ability = GetAbilityInSlot(Slot);
	return MobaPlayerState && Ability && MobaPlayerState->GetSkillPoints() > 0
		&& Ability->CanLevelUp(GetAbilityLevel(Slot), MobaPlayerState->GetChampionLevel());
}

void AMobaCharacter::LevelUpAbility(EMobaAbilitySlot Slot)
{
	if (CanLevelUpAbility(Slot))
	{
		ServerLevelUpAbility(Slot);
	}
}

void AMobaCharacter::ServerLevelUpAbility_Implementation(EMobaAbilitySlot Slot)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	AMobaPlayerState* MobaPlayerState = GetPlayerState<AMobaPlayerState>();
	if (!AbilitySystem || !MobaPlayerState || !CanLevelUpAbility(Slot))
	{
		return;
	}

	const FGameplayTag SlotTag = MobaAbility::GetSlotTag(Slot);
	for (FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(SlotTag) && MobaPlayerState->ConsumeSkillPoint())
		{
			++Spec.Level;

			// 서버 전용 스킬은 추가·삭제가 아니면 엔진이 복제 대상으로 표시하지 않는다.
			// 그러면 조종하는 클라이언트의 스킬 단계가 0에 머무르므로 추가로 표시해 강제로 보낸다.
			AbilitySystem->MarkAbilitySpecDirty(Spec, true);
			return;
		}
	}
}

const UMobaGameplayAbility* AMobaCharacter::GetAbilityInSlot(EMobaAbilitySlot Slot) const
{
	const UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	if (!AbilitySystem)
	{
		return nullptr;
	}

	const FGameplayTag SlotTag = MobaAbility::GetSlotTag(Slot);
	for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(SlotTag))
		{
			return Cast<UMobaGameplayAbility>(Spec.Ability);
		}
	}
	return nullptr;
}

bool AMobaCharacter::GetCooldownRemaining(EMobaAbilitySlot Slot, float& OutRemaining, float& OutDuration) const
{
	OutRemaining = 0.0f;
	OutDuration = 0.0f;

	const UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	if (!AbilitySystem)
	{
		return false;
	}

	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(MobaAbility::GetCooldownTag(Slot)));
	for (const TPair<float, float>& Time : AbilitySystem->GetActiveEffectsTimeRemainingAndDuration(Query))
	{
		if (Time.Key > OutRemaining)
		{
			OutRemaining = Time.Key;
			OutDuration = Time.Value;
		}
	}

	return OutRemaining > 0.0f;
}

void AMobaCharacter::BindToPlayerStateTeam()
{
	if (AMobaPlayerState* MobaPlayerState = GetPlayerState<AMobaPlayerState>())
	{
		MobaPlayerState->OnTeamChanged.RemoveAll(this);
		MobaPlayerState->OnTeamChanged.AddUObject(this, &AMobaCharacter::HandleTeamChanged);
		HandleTeamChanged(MobaPlayerState->GetTeam());
	}
}

void AMobaCharacter::HandleTeamChanged(EMobaTeam NewTeam)
{
	const FLinearColor TeamColor = MobaTeam::GetTeamColor(NewTeam);
	PlaceholderMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(TeamColor.R, TeamColor.G, TeamColor.B));
}

// ---------------------------------------------------------------------------------------------------------------------
// 스킬 시전 알림

void AMobaCharacter::NotifyAbilityCast(EMobaAbilitySlot Slot)
{
	if (!HasAuthority())
	{
		return;
	}

	// 스킬을 쓰면 귀환과 은신이 끊긴다(은신을 거는 스킬은 이 뒤에 다시 건다).
	CancelRecall();
	BreakStealth();
	MulticastAbilityCast(Slot);

	// 소환사 주문은 패시브의 "스킬을 쓰면" 효과를 일으키지 않는다.
	if (PassiveInstance && Slot != EMobaAbilitySlot::D && Slot != EMobaAbilitySlot::F)
	{
		PassiveInstance->OnAbilityCast(Slot);
	}
}

void AMobaCharacter::MulticastAbilityCast_Implementation(EMobaAbilitySlot Slot)
{
	// 이미 미리 보여 준 시전이면 다시 재생하지 않는다. 오래된 예측은 버린다(서버가 거절한 경우).
	double& PredictedTime = PredictedCastTimes[static_cast<int32>(Slot)];
	const bool bAlreadyPlayed = PredictedTime > 0.0 && GetWorld()->GetTimeSeconds() - PredictedTime < 1.5;
	PredictedTime = 0.0;
	if (bAlreadyPlayed)
	{
		return;
	}

	PlayAbilityCastLocally(Slot);
}

// ---------------------------------------------------------------------------------------------------------------------
// 상점

bool AMobaCharacter::IsInFountain() const
{
	return FVector::DistSquared2D(GetActorLocation(), HomeLocation) <= FMath::Square(FountainRadius);
}

bool AMobaCharacter::CanBuyItem(int32 ItemIndex) const
{
	const AMobaPlayerState* MobaPlayerState = GetPlayerState<AMobaPlayerState>();
	const FMobaItemDefinition* Item = MobaItems::GetItem(ItemIndex);
	if (!Item || !MobaPlayerState || !CanShop())
	{
		return false;
	}

	// 소모품은 가방 칸 대신 개수로 쌓인다.
	if (Item->bConsumable)
	{
		const bool bRoom = Item->bControlWard ? MobaPlayerState->GetControlWards() < MobaItems::MaxControlWards : MobaPlayerState->GetPotions() < MobaItems::MaxPotions;
		return MobaPlayerState->GetGold() >= Item->Cost && bRoom;
	}

	const FMobaPurchasePlan Plan = MobaItems::PlanPurchase(ItemIndex, MobaPlayerState->GetItems());
	return MobaPlayerState->GetGold() >= Plan.Cost && MobaItems::CanPurchase(ItemIndex, MobaPlayerState->GetItems(), Plan);
}

int32 AMobaCharacter::GetItemPurchaseCost(int32 ItemIndex) const
{
	const AMobaPlayerState* MobaPlayerState = GetPlayerState<AMobaPlayerState>();
	const FMobaItemDefinition* Item = MobaItems::GetItem(ItemIndex);
	if (!Item)
	{
		return 0;
	}
	return MobaPlayerState ? MobaItems::PlanPurchase(ItemIndex, MobaPlayerState->GetItems()).Cost : Item->Cost;
}

void AMobaCharacter::BuyItem(int32 ItemIndex)
{
	if (CanBuyItem(ItemIndex))
	{
		ServerBuyItem(ItemIndex);
	}
}

void AMobaCharacter::ServerBuyItem_Implementation(int32 ItemIndex)
{
	const FMobaItemDefinition* Item = MobaItems::GetItem(ItemIndex);
	AMobaPlayerState* MobaPlayerState = GetPlayerState<AMobaPlayerState>();
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	if (!Item || !MobaPlayerState || !AbilitySystem || !CanBuyItem(ItemIndex))
	{
		return;
	}

	if (Item->bConsumable)
	{
		if (MobaPlayerState->SpendGold(Item->Cost))
		{
			if (Item->bControlWard)
			{
				MobaPlayerState->SetControlWards(MobaPlayerState->GetControlWards() + 1);
			}
			else
			{
				MobaPlayerState->SetPotions(MobaPlayerState->GetPotions() + 1);
			}
		}
		return;
	}

	const FMobaPurchasePlan Plan = MobaItems::PlanPurchase(ItemIndex, MobaPlayerState->GetItems());
	if (!MobaPlayerState->SpendGold(Plan.Cost))
	{
		return;
	}

	// 재료를 빼고 완성품을 더하면 현재 체력·마나가 재료 몫까지 다시 채워지므로, 최대치가 늘어난 만큼만 채우도록 기억해 둔다.
	const float OldHealth = AbilitySystem->GetNumericAttributeBase(UMobaAttributeSet::GetHealthAttribute());
	const float OldMaxHealth = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxHealthAttribute());
	const float OldMana = AbilitySystem->GetNumericAttributeBase(UMobaAttributeSet::GetManaAttribute());
	const float OldMaxMana = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxManaAttribute());

	for (const int32 Slot : Plan.ConsumedSlots)
	{
		const FMobaItemDefinition* Component = MobaItems::GetItem(MobaPlayerState->GetItems()[Slot]);
		MobaPlayerState->RemoveItemAt(Slot);
		if (Component)
		{
			ApplyItemStats(*Component, -1.0f);
		}
	}

	MobaPlayerState->AddItem(static_cast<uint8>(ItemIndex));
	ApplyItemStats(*Item, 1.0f);

	if (Plan.ConsumedSlots.Num() > 0 && !IsDead())
	{
		const float NewMaxHealth = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxHealthAttribute());
		const float NewMaxMana = AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxManaAttribute());
		AbilitySystem->SetNumericAttributeBase(UMobaAttributeSet::GetHealthAttribute(), FMath::Clamp(OldHealth + FMath::Max(NewMaxHealth - OldMaxHealth, 0.0f), 1.0f, NewMaxHealth));
		AbilitySystem->SetNumericAttributeBase(UMobaAttributeSet::GetManaAttribute(), FMath::Clamp(OldMana + FMath::Max(NewMaxMana - OldMaxMana, 0.0f), 0.0f, NewMaxMana));
	}
}

void AMobaCharacter::UsePotion()
{
	const AMobaPlayerState* MobaPlayerState = GetPlayerState<AMobaPlayerState>();
	if (MobaPlayerState && MobaPlayerState->GetPotions() > 0 && !IsDead())
	{
		ServerUsePotion();
	}
}

void AMobaCharacter::ServerUsePotion_Implementation()
{
	AMobaPlayerState* MobaPlayerState = GetPlayerState<AMobaPlayerState>();
	const FMobaItemDefinition* Potion = MobaItems::GetItem(MobaItems::GetPotionItemIndex());
	if (!MobaPlayerState || !Potion || MobaPlayerState->GetPotions() <= 0 || IsDead() || MobaCombat::IsMatchOver(this))
	{
		return;
	}

	MobaPlayerState->SetPotions(MobaPlayerState->GetPotions() - 1);

	// 마시는 중에 또 마시면 남은 회복에 더해 같은 속도로 이어진다.
	PotionHealPerSecond = Potion->HealAmount / FMath::Max(Potion->HealDuration, 0.1f);
	PotionHealRemaining += Potion->HealAmount;
}

void AMobaCharacter::SellItem(int32 Slot)
{
	const AMobaPlayerState* MobaPlayerState = GetPlayerState<AMobaPlayerState>();
	if (MobaPlayerState && CanShop() && MobaPlayerState->GetItems().IsValidIndex(Slot))
	{
		ServerSellItem(Slot);
	}
}

void AMobaCharacter::ServerSellItem_Implementation(int32 Slot)
{
	AMobaPlayerState* MobaPlayerState = GetPlayerState<AMobaPlayerState>();
	if (!MobaPlayerState || !CanShop() || !MobaPlayerState->GetItems().IsValidIndex(Slot))
	{
		return;
	}

	const FMobaItemDefinition* Item = MobaItems::GetItem(MobaPlayerState->GetItems()[Slot]);
	MobaPlayerState->RemoveItemAt(Slot);
	if (Item)
	{
		MobaPlayerState->AddGold(FMath::FloorToFloat(Item->Cost * MobaItems::SellRefundFraction));
		ApplyItemStats(*Item, -1.0f);
	}
}

void AMobaCharacter::ApplyItemStats(const FMobaItemDefinition& Item, float Sign)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	if (!HasAuthority() || !AbilitySystem)
	{
		return;
	}

	const auto AddToBase = [AbilitySystem, Sign](const FGameplayAttribute& Attribute, float Amount)
	{
		if (Amount != 0.0f)
		{
			AbilitySystem->SetNumericAttributeBase(Attribute, AbilitySystem->GetNumericAttributeBase(Attribute) + Amount * Sign);
		}
	};

	AddToBase(UMobaAttributeSet::GetAttackDamageAttribute(), Item.AttackDamage);
	AddToBase(UMobaAttributeSet::GetAbilityPowerAttribute(), Item.AbilityPower);
	AddToBase(UMobaAttributeSet::GetArmorAttribute(), Item.Armor);
	AddToBase(UMobaAttributeSet::GetMagicResistAttribute(), Item.MagicResist);
	AddToBase(UMobaAttributeSet::GetMoveSpeedAttribute(), Item.MoveSpeed);
	AddToBase(UMobaAttributeSet::GetAttackSpeedAttribute(), GetBaseStats().AttackSpeed * Item.AttackSpeedFraction);
	AddToBase(UMobaAttributeSet::GetMaxHealthAttribute(), Item.MaxHealth);
	AddToBase(UMobaAttributeSet::GetMaxManaAttribute(), Item.MaxMana);
	AddToBase(UMobaAttributeSet::GetLifeStealAttribute(), Item.LifeSteal);
	AddToBase(UMobaAttributeSet::GetAbilityHasteAttribute(), Item.AbilityHaste);

	// 살 때는 늘어난 최대치만큼 현재 수치도 채워 주고, 팔 때는 줄어든 최대치를 넘지 않게만 한다.
	const auto AdjustCurrent = [AbilitySystem, Sign, this](const FGameplayAttribute& Attribute, const FGameplayAttribute& MaxAttribute, float Amount)
	{
		if (Amount == 0.0f || IsDead())
		{
			return;
		}

		const float Current = AbilitySystem->GetNumericAttributeBase(Attribute) + (Sign > 0.0f ? Amount : 0.0f);
		AbilitySystem->SetNumericAttributeBase(Attribute, FMath::Clamp(Current, 1.0f, AbilitySystem->GetNumericAttribute(MaxAttribute)));
	};

	AdjustCurrent(UMobaAttributeSet::GetHealthAttribute(), UMobaAttributeSet::GetMaxHealthAttribute(), Item.MaxHealth);
	if (Item.MaxMana != 0.0f)
	{
		const float Mana = AbilitySystem->GetNumericAttributeBase(UMobaAttributeSet::GetManaAttribute()) + (Sign > 0.0f ? Item.MaxMana : 0.0f);
		AbilitySystem->SetNumericAttributeBase(UMobaAttributeSet::GetManaAttribute(), FMath::Clamp(Mana, 0.0f, AbilitySystem->GetNumericAttribute(UMobaAttributeSet::GetMaxManaAttribute())));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// 귀환

bool AMobaCharacter::IsRecalling() const
{
	return RecallEndServerTime > 0.0f;
}

float AMobaCharacter::GetRecallRemaining() const
{
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (!GameState || RecallEndServerTime <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Max(RecallEndServerTime - static_cast<float>(GameState->GetServerWorldTimeSeconds()), 0.0f);
}

void AMobaCharacter::StartRecall()
{
	if (!CanAct() || IsRecalling())
	{
		return;
	}

	// 걷던 중이면 먼저 멈춘다. 움직이면 귀환이 취소되기 때문이다.
	if (AController* MyController = GetController())
	{
		MyController->StopMovement();
	}
	GetCharacterMovement()->StopMovementImmediately();
	CombatComponent->SetAttackTarget(nullptr);

	ServerStartRecall();
}

void AMobaCharacter::ServerStartRecall_Implementation()
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (!AbilitySystem || !GameState || !CanAct() || IsRecalling() || MobaCombat::IsMatchOver(this))
	{
		return;
	}

	CombatComponent->SetAttackTarget(nullptr);

	RecallStartTime = GetWorld()->GetTimeSeconds();
	RecallEndServerTime = static_cast<float>(GameState->GetServerWorldTimeSeconds()) + RecallDuration;
	AbilitySystem->SetLooseGameplayTagCount(MobaTags::State_Recalling, 1, EGameplayTagReplicationState::TagOnly);

	GetWorldTimerManager().SetTimer(RecallTimer, this, &AMobaCharacter::FinishRecall, RecallDuration, false);
	GetWorldTimerManager().SetTimer(RecallCheckTimer, this, &AMobaCharacter::TickRecall, 0.1f, true);
}

void AMobaCharacter::TickRecall()
{
	// 시작 직후에는 멈추는 중의 잔여 속도가 남아 있을 수 있어 잠깐은 속도를 보지 않는다.
	const bool bMoved = GetWorld()->GetTimeSeconds() - RecallStartTime > 0.4 && GetVelocity().Size2D() > 10.0f;
	if (!CanAct() || bMoved || CombatComponent->GetAttackTarget())
	{
		CancelRecall();
	}
}

void AMobaCharacter::CancelRecall()
{
	if (!HasAuthority() || !IsRecalling())
	{
		return;
	}

	RecallEndServerTime = 0.0f;
	GetWorldTimerManager().ClearTimer(RecallTimer);
	GetWorldTimerManager().ClearTimer(RecallCheckTimer);

	if (UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent())
	{
		AbilitySystem->SetLooseGameplayTagCount(MobaTags::State_Recalling, 0, EGameplayTagReplicationState::TagOnly);
	}
}

void AMobaCharacter::FinishRecall()
{
	if (!IsRecalling())
	{
		return;
	}

	CancelRecall();
	TeleportChampion(HomeLocation, GetActorRotation());
}

void AMobaCharacter::HandleDamageDealt(AActor* DamageInstigator, AActor* Victim, float Damage)
{
	// 맞으면 귀환이 끊긴다.
	if (Victim == this && Damage > 0.0f)
	{
		CancelRecall();

		if (PassiveInstance)
		{
			PassiveInstance->OnDamageTaken(Damage);
		}
	}
}

bool AMobaCharacter::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	return UMobaVisionSubsystem::IsRelevantToViewer(this, RealViewer) && Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
}

// ---------------------------------------------------------------------------------------------------------------------
// 은신

void AMobaCharacter::BreakStealth()
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	if (HasAuthority() && AbilitySystem && AbilitySystem->HasMatchingGameplayTag(MobaTags::State_Stealthed))
	{
		AbilitySystem->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(MobaTags::State_Stealthed));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// 보호막

void AMobaCharacter::AddShield(float Amount, float Duration)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	if (!HasAuthority() || !AbilitySystem || IsDead() || Amount <= 0.0f || Duration <= 0.0f)
	{
		return;
	}

	UpdateShields();
	Shields.Add({ Amount, GetWorld()->GetTimeSeconds() + Duration });

	float Total = 0.0f;
	for (const FShieldEntry& Entry : Shields)
	{
		Total += Entry.Amount;
	}
	AbilitySystem->SetNumericAttributeBase(UMobaAttributeSet::GetShieldAttribute(), Total);
}

void AMobaCharacter::UpdateShields()
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	if (!HasAuthority() || !AbilitySystem)
	{
		return;
	}

	// 죽으면 모두 사라진다.
	if (IsDead())
	{
		Shields.Reset();
	}

	// 피해로 줄어든 만큼을 먼저 사라질 보호막부터 깎는다.
	float Listed = 0.0f;
	for (const FShieldEntry& Entry : Shields)
	{
		Listed += Entry.Amount;
	}
	float Absorbed = FMath::Max(Listed - AbilitySystem->GetNumericAttributeBase(UMobaAttributeSet::GetShieldAttribute()), 0.0f);

	Shields.Sort([](const FShieldEntry& A, const FShieldEntry& B) { return A.ExpireTime < B.ExpireTime; });
	for (FShieldEntry& Entry : Shields)
	{
		const float Taken = FMath::Min(Entry.Amount, Absorbed);
		Entry.Amount -= Taken;
		Absorbed -= Taken;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	Shields.RemoveAll([Now](const FShieldEntry& Entry) { return Entry.Amount <= 0.01f || Entry.ExpireTime <= Now; });

	float Total = 0.0f;
	for (const FShieldEntry& Entry : Shields)
	{
		Total += Entry.Amount;
	}
	if (!FMath::IsNearlyEqual(Total, AbilitySystem->GetNumericAttributeBase(UMobaAttributeSet::GetShieldAttribute())))
	{
		AbilitySystem->SetNumericAttributeBase(UMobaAttributeSet::GetShieldAttribute(), Total);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// 패시브

FMobaAttackBonus AMobaCharacter::ApplyPassiveOnBasicAttack(AActor* Target)
{
	return PassiveInstance ? PassiveInstance->OnBasicAttack(Target) : FMobaAttackBonus();
}

const UMobaPassive* AMobaCharacter::GetPassiveDefaults() const
{
	return ChampionData && ChampionData->Passive ? ChampionData->Passive->GetDefaultObject<UMobaPassive>() : nullptr;
}
