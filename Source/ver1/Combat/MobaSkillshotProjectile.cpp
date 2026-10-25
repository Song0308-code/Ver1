#include "Combat/MobaSkillshotProjectile.h"
#include "Game/MobaEffects.h"
#include "Game/MobaVisuals.h"
#include "AbilitySystem/MobaAbilityLibrary.h"
#include "Combat/MobaCombatLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AMobaSkillshotProjectile::AMobaSkillshotProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	bAlwaysRelevant = true;
	InitialLifeSpan = 10.0f;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);

	// 실제 이펙트가 들어오기 전까지 쓰는 임시 외형.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
		MobaVisuals::UseColorableMaterial(Mesh);
	}
}

void AMobaSkillshotProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMobaSkillshotProjectile, Direction);
	DOREPLIFETIME(AMobaSkillshotProjectile, Speed);
	DOREPLIFETIME(AMobaSkillshotProjectile, MaxRange);
	DOREPLIFETIME(AMobaSkillshotProjectile, Radius);
	DOREPLIFETIME(AMobaSkillshotProjectile, SourceTeam);
}

void AMobaSkillshotProjectile::BeginPlay()
{
	Super::BeginPlay();

	ApplyVisual();
}

void AMobaSkillshotProjectile::Launch(AActor* InSource, const FVector& InDirection, float InSpeed, float InMaxRange, float InRadius, const TArray<FGameplayEffectSpecHandle>& InHitEffects)
{
	Source = InSource;
	Direction = InDirection.GetSafeNormal2D();
	Speed = InSpeed;
	MaxRange = InMaxRange;
	Radius = InRadius;
	SourceTeam = MobaCombat::GetTeam(InSource);
	HitEffects = InHitEffects;

	ApplyVisual();
}

void AMobaSkillshotProjectile::OnRep_Visual()
{
	ApplyVisual();
}

void AMobaSkillshotProjectile::ApplyVisual()
{
	// 엔진 기본 구의 지름은 100이다.
	Mesh->SetWorldScale3D(FVector(Radius / 50.0f));

	const FLinearColor Color = FMath::Lerp(MobaTeam::GetTeamColor(SourceTeam), FLinearColor::White, 0.45f);
	Mesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(Color.R, Color.G, Color.B));
}

void AMobaSkillshotProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Speed <= 0.0f)
	{
		return;
	}

	const float Step = FMath::Min(Speed * DeltaSeconds, MaxRange - DistanceTravelled);
	const FVector Start = GetActorLocation();
	const FVector End = Start + FVector(Direction) * Step;

	if (HasAuthority() && Step > 0.0f)
	{
		TArray<FHitResult> Hits;
		GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Radius));

		for (const FHitResult& Hit : Hits)
		{
			AActor* HitActor = Hit.GetActor();
			if (MobaCombat::IsValidEnemyTarget(Source, HitActor))
			{
				for (const FGameplayEffectSpecHandle& Effect : HitEffects)
				{
					MobaAbility::ApplySpecToTarget(Effect, HitActor);
				}
				if (OnHitActor)
				{
					OnHitActor(HitActor);
				}
				Destroy();
				return;
			}
		}
	}

	SetActorLocation(End);
	DistanceTravelled += Step;

	if (DistanceTravelled >= MaxRange)
	{
		if (HasAuthority())
		{
			Destroy();
		}
		else
		{
			SetActorHiddenInGame(true);
			SetActorTickEnabled(false);
		}
	}
}

void AMobaSkillshotProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 사거리 끝까지 날아가 사라진 것이 아니라면 무언가에 맞은 것이다.
	if (EndPlayReason == EEndPlayReason::Destroyed && MaxRange > 0.0f && DistanceTravelled < MaxRange - 1.0f)
	{
		MobaEffects::Play(this, EMobaEffect::Hit, GetActorLocation(), Radius / 25.0f);
	}

	Super::EndPlay(EndPlayReason);
}
