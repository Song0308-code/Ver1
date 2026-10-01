#include "Map/MobaLanePath.h"
#include "Components/SplineComponent.h"

AMobaLanePath::AMobaLanePath()
{
	PrimaryActorTick.bCanEverTick = false;

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	RootComponent = Spline;
}

void AMobaLanePath::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RebuildSpline();
}

void AMobaLanePath::BeginPlay()
{
	Super::BeginPlay();

	RebuildSpline();
}

void AMobaLanePath::RebuildSpline()
{
	if (Waypoints.Num() < 2)
	{
		return;
	}

	Spline->SetSplinePoints(Waypoints, ESplineCoordinateSpace::World, false);
	for (int32 Index = 0; Index < Waypoints.Num(); ++Index)
	{
		Spline->SetSplinePointType(Index, ESplinePointType::Linear, false);
	}
	Spline->UpdateSpline();
}
