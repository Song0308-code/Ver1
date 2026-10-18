#include "Vision/MobaWard.h"
#include "AbilitySystem/MobaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaCharacter.h"
#include "Combat/MobaCombatLibrary.h"
#include "Game/MobaGameMode.h"
#include "Game/MobaGameplayTags.h"
#include "Player/MobaPlayerState.h"
#include "Vision/MobaVisionSubsystem.h"
#include "Game/MobaVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AMobaWard::AMobaWard()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	// 커서로 고를 수만 있게 한다. 물리 물체 종류로 두어 이동을 막지 않고, 미니언·포탑의 대상 찾기(폰·동적 물체)에도 걸리지 않는다.
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionObjectType(ECC_PhysicsBody);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCastShadow(false);
	Mesh->SetCanEverAffectNavigation(false);

	// 실제 메시가 들어오기 전까지 쓰는 임시 외형: 가느다란 기둥.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CylinderMesh.Object);
		MobaVisuals::UseColorableMaterial(Mesh);
		Mesh->SetRelativeScale3D(FVector(0.25, 0.25, 0.9));
	}

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UMobaAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* AMobaWard::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AMobaWard::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMobaWard, Team);
	DOREPLIFETIME(AMobaWard, bControlWard);
}

void AMobaWard::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	ApplyTeamColor();

	if (HasAuthority())
	{
		FMobaBaseStats Stats;
		Stats.MaxHealth = bControlWard ? ControlWardHits : WardHits;
		Stats.MaxMana = 0.0f;
		Stats.AttackDamage = 0.0f;
		Stats.Armor = 0.0f;
		Stats.MagicResist = 0.0f;
		Stats.MoveSpeed = 0.0f;
		AttributeSet->InitFromBaseStats(Stats);
		AttributeSet->OnOutOfHealth.AddUObject(this, &AMobaWard::HandleOutOfHealth);
		AbilitySystemComponent->AddLooseGameplayTag(MobaTags::Unit_Ward, 1, EGameplayTagReplicationState::TagOnly);
	}
}

void AMobaWard::HandleOutOfHealth(AActor* DamageInstigator)
{
	const APawn* KillerPawn = Cast<APawn>(DamageInstigator);
	if (AMobaPlayerState* KillerState = KillerPawn ? KillerPawn->GetPlayerState<AMobaPlayerState>() : nullptr)
	{
		KillerState->AddGold(KillGold);
	}

	if (AMobaGameMode* GameMode = GetWorld()->GetAuthGameMode<AMobaGameMode>())
	{
		GameMode->Announce(FString::Printf(TEXT("%s 팀 %s 파괴 (+%dG)"), *MobaTeam::GetTeamDisplayName(Team), bControlWard ? TEXT("제어 와드") : TEXT("와드"), FMath::RoundToInt(KillGold)),
			MobaCombat::GetTeam(DamageInstigator));
	}

	Destroy();
}

AMobaWard* AMobaWard::SpawnWard(UWorld* World, EMobaTeam InTeam, const FVector& Location, float Duration, bool bInControlWard)
{
	if (!World || InTeam == EMobaTeam::None)
	{
		return nullptr;
	}

	// 기둥의 높이가 90이므로 절반만큼 올려 바닥에 세운다.
	const FTransform SpawnTransform(FRotator::ZeroRotator, Location + FVector(0.0, 0.0, 45.0));
	AMobaWard* Ward = World->SpawnActorDeferred<AMobaWard>(AMobaWard::StaticClass(), SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Ward)
	{
		Ward->Team = InTeam;
		Ward->bControlWard = bInControlWard;
		Ward->FinishSpawning(SpawnTransform);
		if (Duration > 0.0f)
		{
			Ward->SetLifeSpan(Duration);
		}
	}
	return Ward;
}

void AMobaWard::OnRep_Team()
{
	ApplyTeamColor();
}

void AMobaWard::ApplyTeamColor()
{
	// 일반 와드는 팀 색에 초록, 제어 와드는 팀 색에 빨강을 섞는다.
	const FLinearColor Color = FMath::Lerp(MobaTeam::GetTeamColor(Team), bControlWard ? FLinearColor::Red : FLinearColor::Green, 0.5f);
	Mesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(Color.R, Color.G, Color.B));
}

bool AMobaWard::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	return UMobaVisionSubsystem::IsRelevantToViewer(this, RealViewer) && Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
}
