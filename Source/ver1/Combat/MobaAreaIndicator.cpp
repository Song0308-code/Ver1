#include "Combat/MobaAreaIndicator.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AMobaAreaIndicator::AMobaAreaIndicator()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CylinderMesh.Object);
	}
}

void AMobaAreaIndicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMobaAreaIndicator, Radius);
}

void AMobaAreaIndicator::BeginPlay()
{
	Super::BeginPlay();

	ApplyRadiusToMesh();
}

void AMobaAreaIndicator::Show(float InRadius, float Duration)
{
	Radius = InRadius;
	ApplyRadiusToMesh();
	SetLifeSpan(FMath::Max(Duration, 0.1f));
}

void AMobaAreaIndicator::OnRep_Radius()
{
	ApplyRadiusToMesh();
}

void AMobaAreaIndicator::ApplyRadiusToMesh()
{
	// 엔진 기본 원기둥은 지름 100, 높이 100이다. 바닥에 얇게 깐다.
	Mesh->SetWorldScale3D(FVector(Radius / 50.0f, Radius / 50.0f, 0.05f));
	Mesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(1.0, 0.25, 0.05));
}
