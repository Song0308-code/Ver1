#include "Combat/MobaProjectile.h"
#include "AbilitySystemComponent.h"
#include "Combat/MobaCombatLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AMobaProjectile::AMobaProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	bAlwaysRelevant = true;
	InitialLifeSpan = 10.0f;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.3f));

	// 실제 이펙트가 들어오기 전까지 쓰는 임시 외형.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
	}
}

void AMobaProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMobaProjectile, Target);
	DOREPLIFETIME(AMobaProjectile, Speed);
}

void AMobaProjectile::InitHoming(AActor* InTarget, const FGameplayEffectSpecHandle& InEffectSpec, float InSpeed)
{
	Target = InTarget;
	EffectSpec = InEffectSpec;
	Speed = InSpeed;
}

void AMobaProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsValid(Target) || MobaCombat::IsDead(Target))
	{
		// 클라이언트에서는 대상이 아직 복제되지 않았을 수 있으므로 서버만 정리한다.
		if (HasAuthority())
		{
			Destroy();
		}
		return;
	}

	const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	const float Distance = ToTarget.Size();
	const float Step = Speed * DeltaSeconds;

	if (Distance <= Step + HitRadius)
	{
		if (HasAuthority())
		{
			UAbilitySystemComponent* TargetAbilitySystem = MobaCombat::GetAbilitySystem(Target);
			if (TargetAbilitySystem && EffectSpec.IsValid())
			{
				TargetAbilitySystem->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
			}
			Destroy();
		}
		else
		{
			SetActorHiddenInGame(true);
			SetActorTickEnabled(false);
		}
		return;
	}

	SetActorLocation(GetActorLocation() + ToTarget / Distance * Step);
}
