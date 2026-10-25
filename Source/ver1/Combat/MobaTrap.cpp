#include "Combat/MobaTrap.h"
#include "AbilitySystem/MobaAbilityLibrary.h"
#include "Character/MobaCharacter.h"
#include "Combat/MobaCombatLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Game/MobaEffects.h"
#include "Game/MobaGameplayTags.h"
#include "Game/MobaVisuals.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Player/MobaPlayerState.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AMobaTrap::AMobaTrap()
{
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);

	// 임시 외형: 납작한 원통.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CylinderMesh.Object);
		Mesh->SetRelativeScale3D(FVector(0.9, 0.9, 0.08));
		MobaVisuals::UseColorableMaterial(Mesh);
	}
}

void AMobaTrap::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMobaTrap, Team);
}

bool AMobaTrap::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	// 같은 팀 PC에만 보낸다.
	const APlayerController* Viewer = Cast<APlayerController>(RealViewer);
	const AMobaPlayerState* ViewerState = Viewer ? Viewer->GetPlayerState<AMobaPlayerState>() : nullptr;
	return ViewerState && ViewerState->GetTeam() == Team && Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
}

void AMobaTrap::BeginPlay()
{
	Super::BeginPlay();
	OnRep_Team();
}

void AMobaTrap::OnRep_Team()
{
	const FLinearColor Color = FMath::Lerp(MobaTeam::GetTeamColor(Team), FLinearColor::Black, 0.35f);
	Mesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(Color.R, Color.G, Color.B));

	// 서버를 겸하는 호스트 화면에서도 상대 팀의 덫은 숨긴다.
	const APlayerController* LocalController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const AMobaPlayerState* LocalState = LocalController ? LocalController->GetPlayerState<AMobaPlayerState>() : nullptr;
	SetActorHiddenInGame(LocalState && LocalState->GetTeam() != Team);
}

void AMobaTrap::Arm(AMobaCharacter* InOwner, float InDamage, float InArmDelay, float InLifetime)
{
	TrapOwner = InOwner;
	Team = InOwner ? InOwner->GetTeam() : EMobaTeam::None;
	Damage = InDamage;
	ArmTime = GetWorld()->GetTimeSeconds() + InArmDelay;
	SetLifeSpan(InLifetime);
	OnRep_Team();

	GetWorldTimerManager().SetTimer(CheckTimer, this, &AMobaTrap::CheckTrigger, 0.1f, true);
}

void AMobaTrap::CheckTrigger()
{
	AMobaCharacter* Placer = TrapOwner.Get();
	if (!Placer)
	{
		Destroy();
		return;
	}
	if (GetWorld()->GetTimeSeconds() < ArmTime)
	{
		return;
	}

	TArray<AActor*> Enemies;
	MobaAbility::FindEnemiesInRadius(Placer, GetActorLocation(), TriggerRadius, Enemies);
	for (AActor* Enemy : Enemies)
	{
		if (Enemy->IsA<AMobaCharacter>())
		{
			Explode();
			return;
		}
	}
}

void AMobaTrap::Explode()
{
	AMobaCharacter* Placer = TrapOwner.Get();
	UAbilitySystemComponent* AbilitySystem = Placer ? Placer->GetAbilitySystemComponent() : nullptr;
	if (AbilitySystem)
	{
		TArray<AActor*> Enemies;
		MobaAbility::FindEnemiesInRadius(Placer, GetActorLocation(), ExplosionRadius, Enemies);
		for (AActor* Enemy : Enemies)
		{
			MobaAbility::ApplySpecToTarget(MobaAbility::MakeDamageSpec(AbilitySystem, Placer, 0.0f, Damage, 0.0f), Enemy);
			if (MobaCombat::IsDead(Enemy))
			{
				continue;
			}
			MobaAbility::ApplySpecToTarget(MobaAbility::MakeStatModSpec(AbilitySystem, Placer, SlowDuration, SlowMultiplier, 1.0f, MobaTags::State_Slowed), Enemy);
			if (Enemy->IsA<AMobaCharacter>())
			{
				MobaAbility::ApplySpecToTarget(MobaAbility::MakeCrowdControlSpec(AbilitySystem, Placer, MarkDuration, MobaTags::State_Marked), Enemy);
				Placer->NotifyAbilityHit(Enemy);
			}
		}
	}

	MobaEffects::Play(this, EMobaEffect::AreaBlast, GetActorLocation(), ExplosionRadius / 300.0f);
	Destroy();
}
