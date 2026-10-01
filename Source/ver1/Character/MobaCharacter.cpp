#include "Character/MobaCharacter.h"
#include "AbilitySystem/Abilities/MobaAbilityArchetypes.h"
#include "AbilitySystem/MobaAbilityLibrary.h"
#include "AbilitySystem/MobaGameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaChampionData.h"
#include "Combat/MobaCombatComponent.h"
#include "Game/MobaGameMode.h"
#include "Game/MobaGameplayTags.h"
#include "GameFramework/Controller.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/MobaPlayerState.h"
#include "UObject/ConstructorHelpers.h"

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
		const float Radius = GetCapsuleComponent()->GetUnscaledCapsuleRadius();
		const float HalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
		PlaceholderMesh->SetRelativeScale3D(FVector(Radius / 50.0f, Radius / 50.0f, HalfHeight / 50.0f));
	}

	CombatComponent = CreateDefaultSubobject<UMobaCombatComponent>(TEXT("CombatComponent"));

	DefaultAbilities.Add(EMobaAbilitySlot::Q, UMobaAbility_Skillshot::StaticClass());
	DefaultAbilities.Add(EMobaAbilitySlot::W, UMobaAbility_SelfBuff::StaticClass());
	DefaultAbilities.Add(EMobaAbilitySlot::E, UMobaAbility_Blink::StaticClass());
	DefaultAbilities.Add(EMobaAbilitySlot::R, UMobaAbility_GroundAoE::StaticClass());
}

void AMobaCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMobaCharacter, ChampionData);
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

bool AMobaCharacter::IsStunned() const
{
	const UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	return AbilitySystem && AbilitySystem->HasMatchingGameplayTag(MobaTags::State_Stunned);
}

void AMobaCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!ChampionData)
	{
		if (const AMobaGameMode* GameMode = GetWorld()->GetAuthGameMode<AMobaGameMode>())
		{
			ChampionData = GameMode->GetDefaultChampion();
		}
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
	AttributeSet->OnMoveSpeedChanged.AddUObject(this, &AMobaCharacter::HandleMoveSpeedChanged);

	if (HasAuthority())
	{
		AttributeSet->OnOutOfHealth.AddUObject(this, &AMobaCharacter::HandleOutOfHealth);

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

	MobaPlayerState->GetAttributeSet()->InitFromBaseStats(ChampionData ? ChampionData->BaseStats : BaseStats);

	if (ChampionData)
	{
		CombatComponent->ConfigureBasicAttack(ChampionData->bRangedBasicAttack, ChampionData->BasicAttackProjectileSpeed);
	}

	const TMap<EMobaAbilitySlot, TSubclassOf<UMobaGameplayAbility>>& Abilities = ChampionData ? ChampionData->Abilities : DefaultAbilities;
	for (const TPair<EMobaAbilitySlot, TSubclassOf<UMobaGameplayAbility>>& Entry : Abilities)
	{
		if (Entry.Value)
		{
			FGameplayAbilitySpec Spec(Entry.Value, 1);
			Spec.GetDynamicSpecSourceTags().AddTag(MobaAbility::GetSlotTag(Entry.Key));
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

	if (AMobaGameMode* GameMode = GetWorld()->GetAuthGameMode<AMobaGameMode>())
	{
		GameMode->HandleChampionKilled(this, DamageInstigator);
	}
}

void AMobaCharacter::HandleDeadTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	ApplyDeadState(NewCount > 0);
}

void AMobaCharacter::HandleStunnedTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	UpdateMovementLock();
}

void AMobaCharacter::ApplyDeadState(bool bDead)
{
	SetActorHiddenInGame(bDead);
	SetActorEnableCollision(!bDead);

	UpdateMovementLock();
}

void AMobaCharacter::UpdateMovementLock()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!CanAct())
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
	if (CanAct())
	{
		ServerCastAbility(Slot, TargetLocation);
	}
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
