#include "Combat/MobaProjectile.h"
#include "AbilitySystem/MobaAbilityLibrary.h"
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
	Mesh->SetCastShadow(false);

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
	DOREPLIFETIME(AMobaProjectile, VisualScale);
	DOREPLIFETIME(AMobaProjectile, SourceTeam);
}

void AMobaProjectile::BeginPlay()
{
	Super::BeginPlay();

	ApplyVisual();
}

void AMobaProjectile::InitHoming(AActor* InTarget, const FGameplayEffectSpecHandle& InEffectSpec, float InSpeed, float InVisualScale, EMobaTeam InSourceTeam)
{
	Target = InTarget;
	EffectSpec = InEffectSpec;
	Speed = InSpeed;
	VisualScale = InVisualScale;
	SourceTeam = InSourceTeam;

	ApplyVisual();
}

void AMobaProjectile::OnRep_Visual()
{
	ApplyVisual();
}

void AMobaProjectile::ApplyVisual()
{
	Mesh->SetWorldScale3D(FVector(VisualScale));

	// 어두운 바닥 위에서도 잘 보이도록 팀 색을 밝게 띄운다.
	const FLinearColor Color = FMath::Lerp(MobaTeam::GetTeamColor(SourceTeam), FLinearColor::White, 0.45f);
	Mesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(Color.R, Color.G, Color.B));
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

	const FVector ToTarget = MobaCombat::GetAimLocation(Target) - GetActorLocation();
	const float Distance = ToTarget.Size();
	const float Step = Speed * DeltaSeconds;

	if (Distance <= Step + HitRadius)
	{
		if (HasAuthority())
		{
			for (const FGameplayEffectSpecHandle& ExtraSpec : ExtraEffectSpecs)
			{
				MobaAbility::ApplySpecToTarget(ExtraSpec, Target);
			}
			MobaAbility::ApplySpecToTarget(EffectSpec, Target);
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
