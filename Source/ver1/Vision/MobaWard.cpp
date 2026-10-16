#include "Vision/MobaWard.h"
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
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
}

void AMobaWard::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMobaWard, Team);
}

void AMobaWard::BeginPlay()
{
	Super::BeginPlay();

	ApplyTeamColor();
}

AMobaWard* AMobaWard::SpawnWard(UWorld* World, EMobaTeam InTeam, const FVector& Location, float Duration)
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
		Ward->FinishSpawning(SpawnTransform);
		Ward->SetLifeSpan(Duration);
	}
	return Ward;
}

void AMobaWard::OnRep_Team()
{
	ApplyTeamColor();
}

void AMobaWard::ApplyTeamColor()
{
	const FLinearColor Color = FMath::Lerp(MobaTeam::GetTeamColor(Team), FLinearColor::Green, 0.5f);
	Mesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(Color.R, Color.G, Color.B));
}

bool AMobaWard::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	return UMobaVisionSubsystem::IsRelevantToViewer(this, RealViewer) && Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
}
